import { spawn } from "node:child_process";
import { timingSafeEqual } from "node:crypto";
import { readFileSync } from "node:fs";
import { access, realpath, writeFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const SERVER_DIRECTORY = path.dirname(fileURLToPath(import.meta.url));

function loadLocalEnv() {
  try {
    const envText = readFileSync(path.join(SERVER_DIRECTORY, ".env"), "utf8");
    for (const line of envText.split(/\r?\n/)) {
      const trimmed = line.trim();
      if (!trimmed || trimmed.startsWith("#")) continue;

      const separatorIndex = trimmed.indexOf("=");
      if (separatorIndex < 1) continue;

      const key = trimmed.slice(0, separatorIndex).trim();
      const value = trimmed.slice(separatorIndex + 1).trim().replace(/^(['"])(.*)\1$/, "$2");
      if (process.env[key] === undefined) process.env[key] = value;
    }
  } catch (error) {
    if (error.code !== "ENOENT") throw error;
  }
}

loadLocalEnv();

const OPENSD_ROOT = "/mnt/c/codes/opensd";
const OPENSD_EXECUTABLE = `${OPENSD_ROOT}/build/opensd`;
const WORKING_DIRECTORY_ROOTS = (process.env.OPENSD_WORKING_ROOTS ?? "/mnt/c")
  .split(",")
  .map((root) => root.trim())
  .filter(Boolean);
const MAX_REQUEST_BYTES = 12 * 1024 * 1024;
const MAX_OUTPUT_BYTES = 4 * 1024 * 1024;
const RUN_TIMEOUT_MS = 10 * 60 * 1000;
const AUTH_REALM = "OpenSD Web";
const AUTH_DISABLED = process.env.OPENSD_WEB_AUTH === "off";
const AUTH_USERNAME = process.env.OPENSD_WEB_USERNAME ?? "";
const AUTH_PASSWORD = process.env.OPENSD_WEB_PASSWORD ?? "";

function sendJson(response, status, body) {
  response.statusCode = status;
  response.setHeader("Content-Type", "application/json; charset=utf-8");
  response.end(JSON.stringify(body));
}

function sendText(response, status, body) {
  response.statusCode = status;
  response.setHeader("Content-Type", "text/plain; charset=utf-8");
  response.end(body);
}

function unauthorized(response) {
  response.statusCode = 401;
  response.setHeader("WWW-Authenticate", `Basic realm="${AUTH_REALM}", charset="UTF-8"`);
  response.setHeader("Content-Type", "text/plain; charset=utf-8");
  response.end("Authentication required.");
}

function isAuthConfigured() {
  return AUTH_USERNAME.length > 0 && AUTH_PASSWORD.length > 0;
}

function constantTimeEqual(left, right) {
  const leftBuffer = Buffer.from(left);
  const rightBuffer = Buffer.from(right);
  if (leftBuffer.length !== rightBuffer.length) return false;
  return timingSafeEqual(leftBuffer, rightBuffer);
}

function parseBasicCredentials(header) {
  if (typeof header !== "string" || !header.startsWith("Basic ")) return null;

  const decoded = Buffer.from(header.slice(6), "base64").toString("utf8");
  const separatorIndex = decoded.indexOf(":");
  if (separatorIndex < 0) return null;

  return {
    username: decoded.slice(0, separatorIndex),
    password: decoded.slice(separatorIndex + 1)
  };
}

function isAuthorized(request) {
  const credentials = parseBasicCredentials(request.headers.authorization);
  if (!credentials) return false;

  return (
    constantTimeEqual(credentials.username, AUTH_USERNAME) &&
    constantTimeEqual(credentials.password, AUTH_PASSWORD)
  );
}

function authMiddleware(request, response, next) {
  if (AUTH_DISABLED) {
    next();
    return;
  }

  if (!isAuthConfigured()) {
    sendText(
      response,
      503,
      "OpenSD web access control is enabled, but OPENSD_WEB_USERNAME and OPENSD_WEB_PASSWORD are not set."
    );
    return;
  }

  if (!isAuthorized(request)) {
    unauthorized(response);
    return;
  }

  next();
}

function readJson(request) {
  return new Promise((resolve, reject) => {
    const chunks = [];
    let size = 0;

    request.on("data", (chunk) => {
      size += chunk.length;
      if (size > MAX_REQUEST_BYTES) {
        reject(new Error("Solver request is too large."));
        request.destroy();
        return;
      }
      chunks.push(chunk);
    });
    request.on("end", () => {
      try {
        resolve(JSON.parse(Buffer.concat(chunks).toString("utf8")));
      } catch {
        reject(new Error("Invalid solver request."));
      }
    });
    request.on("error", reject);
  });
}

async function checkedWorkingDirectory(inputDirectory) {
  if (typeof inputDirectory !== "string" || !path.posix.isAbsolute(inputDirectory)) {
    throw new Error("Working directory must be an absolute WSL path.");
  }

  const directory = await realpath(inputDirectory);
  const roots = await Promise.all(WORKING_DIRECTORY_ROOTS.map((root) => realpath(root)));
  const isAllowed = roots.some((root) => directory === root || directory.startsWith(`${root}/`));
  if (!isAllowed) {
    throw new Error(`Working directory must be inside one of: ${WORKING_DIRECTORY_ROOTS.join(", ")}.`);
  }
  return directory;
}

function appendOutput(current, chunk) {
  if (current.length >= MAX_OUTPUT_BYTES) return current;
  return current + chunk.toString("utf8").slice(0, MAX_OUTPUT_BYTES - current.length);
}

function runSolver(directory, threads) {
  return new Promise((resolve, reject) => {
    const args = [];
    if (threads > 0) args.push("--threads", String(threads));
    args.push(`${directory}/`);

    const child = spawn(OPENSD_EXECUTABLE, args, {
      cwd: directory,
      env: process.env,
      shell: false
    });
    let stdout = "";
    let stderr = "";
    let timedOut = false;
    const timeout = setTimeout(() => {
      timedOut = true;
      child.kill("SIGTERM");
    }, RUN_TIMEOUT_MS);

    child.stdout.on("data", (chunk) => { stdout = appendOutput(stdout, chunk); });
    child.stderr.on("data", (chunk) => { stderr = appendOutput(stderr, chunk); });
    child.on("error", (error) => {
      clearTimeout(timeout);
      reject(error);
    });
    child.on("close", (exitCode, signal) => {
      clearTimeout(timeout);
      resolve({
        command: [OPENSD_EXECUTABLE, ...args].join(" "),
        exitCode,
        signal,
        timedOut,
        stdout,
        stderr
      });
    });
  });
}

async function handleSolverRun(request, response) {
  try {
    await access(OPENSD_EXECUTABLE);
    const body = await readJson(request);
    const directory = await checkedWorkingDirectory(body.inputDirectory);
    if (typeof body.settingsXml !== "string" || !body.settingsXml.includes("<settings>")) {
      throw new Error("A valid settings XML document is required.");
    }

    await writeFile(path.join(directory, "settings.xml"), body.settingsXml, "utf8");
    if (typeof body.geometryXml === "string" && body.geometryXml.includes("<geometry")) {
      await writeFile(path.join(directory, "geometry.xml"), body.geometryXml, "utf8");
    }

    const threads = Number.isInteger(body.threads) && body.threads > 0 ? body.threads : 0;
    const result = await runSolver(directory, threads);
    sendJson(response, result.exitCode === 0 ? 200 : 500, result);
  } catch (error) {
    sendJson(response, 400, { error: error.message });
  }
}

function solverMiddleware(request, response, next) {
  if (request.method === "POST" && request.url === "/api/solver/run") {
    handleSolverRun(request, response);
    return;
  }
  next();
}

export function solverServerPlugin() {
  return {
    name: "opensd-solver-server",
    configureServer(server) {
      server.middlewares.use(authMiddleware);
      server.middlewares.use(solverMiddleware);
    },
    configurePreviewServer(server) {
      server.middlewares.use(authMiddleware);
      server.middlewares.use(solverMiddleware);
    }
  };
}
