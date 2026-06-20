import { spawn } from "node:child_process";
import { access, realpath, writeFile } from "node:fs/promises";
import path from "node:path";

const OPENSD_ROOT = "/mnt/c/codes/opensd";
const OPENSD_EXECUTABLE = `${OPENSD_ROOT}/build/opensd`;
const MAX_REQUEST_BYTES = 12 * 1024 * 1024;
const MAX_OUTPUT_BYTES = 4 * 1024 * 1024;
const RUN_TIMEOUT_MS = 10 * 60 * 1000;

function sendJson(response, status, body) {
  response.statusCode = status;
  response.setHeader("Content-Type", "application/json; charset=utf-8");
  response.end(JSON.stringify(body));
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

  const root = await realpath(OPENSD_ROOT);
  const directory = await realpath(inputDirectory);
  if (directory !== root && !directory.startsWith(`${root}/`)) {
    throw new Error(`Working directory must be inside ${OPENSD_ROOT}.`);
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
      server.middlewares.use(solverMiddleware);
    },
    configurePreviewServer(server) {
      server.middlewares.use(solverMiddleware);
    }
  };
}
