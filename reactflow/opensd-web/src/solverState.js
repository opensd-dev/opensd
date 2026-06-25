import { useRef, useState } from "react";

export const DEFAULT_SETTINGS = {
  run_mode: "steady",
  tim_slot: "0.0 0.0",
  verbosity: "3",
  alpha_mom: "1.0",
  alpha_ener: "1.0",
  alpha_heat: "1.0",
  no_main_iter: "500",
  no_flow_iter: "300",
  temp_solve: true,
  flag_write: true,
  T_ambient: "300.0",
  conv_crit_flow: "1e-10",
  conv_crit_temp_SS: "1e-10"
};

export const NUMERIC_FIELDS = [
  ["verbosity", "Verbosity", "1"],
  ["alpha_mom", "Momentum relaxation", "any"],
  ["alpha_ener", "Energy relaxation", "any"],
  ["alpha_heat", "Heat relaxation", "any"],
  ["no_main_iter", "Main iterations", "1"],
  ["no_flow_iter", "Flow iterations", "1"],
  ["T_ambient", "Ambient temperature", "any"],
  ["conv_crit_flow", "Flow convergence", "any"],
  ["conv_crit_temp_SS", "Temperature convergence", "any"]
];

export function settingsXml(settings) {
  const value = (name) => String(settings[name]);
  return `<?xml version='1.0' encoding='UTF-8'?>
<settings>
  <run_mode>${value("run_mode")}</run_mode>
  <tim_slot>${value("tim_slot").trim()}</tim_slot>
  <verbosity>${value("verbosity")}</verbosity>
  <alpha_mom>${value("alpha_mom")}</alpha_mom>
  <alpha_ener>${value("alpha_ener")}</alpha_ener>
  <alpha_heat>${value("alpha_heat")}</alpha_heat>
  <no_main_iter>${value("no_main_iter")}</no_main_iter>
  <no_flow_iter>${value("no_flow_iter")}</no_flow_iter>
  <temp_solve>${settings.temp_solve ? "True" : "False"}</temp_solve>
  <flag_write>${settings.flag_write ? "True" : "False"}</flag_write>
  <T_ambient>${value("T_ambient")}</T_ambient>
  <conv_crit_flow>${value("conv_crit_flow")}</conv_crit_flow>
  <conv_crit_temp_SS>${value("conv_crit_temp_SS")}</conv_crit_temp_SS>
</settings>
`;
}

function settingsFromXml(xmlText) {
  const document = new DOMParser().parseFromString(xmlText, "application/xml");
  if (document.querySelector("parsererror") || !document.querySelector("settings")) {
    throw new Error("The selected file is not a valid settings.xml document.");
  }

  const text = (name, fallback) => document.querySelector(name)?.textContent?.trim() || fallback;
  const boolean = (name, fallback) => {
    const raw = text(name, fallback ? "True" : "False").toLowerCase();
    return raw === "true" || raw === "1";
  };

  return {
    ...DEFAULT_SETTINGS,
    ...Object.fromEntries(Object.keys(DEFAULT_SETTINGS)
      .filter((name) => !["temp_solve", "flag_write"].includes(name))
      .map((name) => [name, text(name, DEFAULT_SETTINGS[name])])),
    temp_solve: boolean("temp_solve", DEFAULT_SETTINGS.temp_solve),
    flag_write: boolean("flag_write", DEFAULT_SETTINGS.flag_write)
  };
}

async function solverResponseJson(response) {
  const text = await response.text();
  try {
    return text ? JSON.parse(text) : {};
  } catch {
    const contentType = response.headers.get("content-type") || "unknown content type";
    const preview = text.trim().slice(0, 240);
    throw new Error(`Solver server did not return JSON (${response.status} ${response.statusText}, ${contentType}). ${preview || "Empty response."}`);
  }
}

export function useSolverPanelState(currentGeometryXml) {
  const settingsInput = useRef(null);
  const [settings, setSettings] = useState(DEFAULT_SETTINGS);
  const [inputDirectory, setInputDirectory] = useState("/mnt/c/codes/opensd/tutorials/tutorial1");
  const [threads, setThreads] = useState("1");
  const [useCurrentGeometry, setUseCurrentGeometry] = useState(true);
  const [status, setStatus] = useState("Ready");
  const [output, setOutput] = useState("");
  const [running, setRunning] = useState(false);

  const update = (name, value) => setSettings((current) => ({ ...current, [name]: value }));

  const importSettings = async (file) => {
    if (!file) return;
    try {
      setSettings(settingsFromXml(await file.text()));
      setStatus(`Loaded ${file.name}`);
    } catch (error) {
      setStatus(error.message);
    } finally {
      if (settingsInput.current) settingsInput.current.value = "";
    }
  };

  const run = async () => {
    setRunning(true);
    setOutput("");
    setStatus("Solver running...");

    try {
      const geometryXml = useCurrentGeometry ? currentGeometryXml() : null;
      const response = await fetch("/api/solver/run", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          inputDirectory: inputDirectory.trim(),
          settingsXml: settingsXml(settings),
          geometryXml,
          threads: Number.parseInt(threads, 10)
        })
      });
      const result = await solverResponseJson(response);
      const runOutput = [result.command, result.stdout, result.stderr].filter(Boolean).join("\n\n");
      if (runOutput) setOutput(runOutput);
      if (!response.ok) {
        setStatus(`Solver failed${Number.isInteger(result.exitCode) ? ` (exit ${result.exitCode})` : ""}`);
        if (!runOutput) setOutput(result.error || `Solver exited with code ${result.exitCode}`);
        return;
      }

      setStatus(`Completed successfully (exit ${result.exitCode})`);
    } catch (error) {
      setOutput(error.message);
      setStatus("Solver failed");
    } finally {
      setRunning(false);
    }
  };

  return {
    settingsInput,
    settings,
    setSettings,
    inputDirectory,
    setInputDirectory,
    threads,
    setThreads,
    useCurrentGeometry,
    setUseCurrentGeometry,
    status,
    output,
    running,
    update,
    importSettings,
    run
  };
}
