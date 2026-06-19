import { useRef, useState } from "react";

const DEFAULT_SETTINGS = {
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

const NUMERIC_FIELDS = [
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

function settingsXml(settings) {
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

function downloadSettings(xml) {
  const url = URL.createObjectURL(new Blob([xml], { type: "application/xml;charset=utf-8" }));
  const anchor = document.createElement("a");
  anchor.href = url;
  anchor.download = "settings.xml";
  anchor.click();
  URL.revokeObjectURL(url);
}

export default function SolverPanel({ currentGeometryXml }) {
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
    setStatus("Solver running…");

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
      const result = await response.json();
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

  return (
    <section className="workspace-pane solver-view">
      <input
        ref={settingsInput}
        className="file-input"
        type="file"
        accept=".xml,application/xml,text/xml"
        onChange={(event) => importSettings(event.target.files?.[0])}
      />

      <div className="solver-toolbar">
        <label className="solver-directory">
          Working directory
          <input value={inputDirectory} onChange={(event) => setInputDirectory(event.target.value)} />
        </label>
        <label>
          Threads
          <input type="number" min="1" step="1" value={threads} onChange={(event) => setThreads(event.target.value)} />
        </label>
        <button className="secondary-button" onClick={() => settingsInput.current?.click()} disabled={running}>
          Import settings
        </button>
        <button className="secondary-button" onClick={() => downloadSettings(settingsXml(settings))} disabled={running}>
          Export settings
        </button>
        <button className="primary-button" onClick={run} disabled={running || !inputDirectory.trim()}>
          {running ? "Running…" : "Run solver"}
        </button>
      </div>

      <div className="solver-content">
        <div className="solver-settings-card">
          <div className="solver-card-heading">
            <div>
              <h2>Solver settings</h2>
              <p>Written to settings.xml before each run.</p>
            </div>
            <button type="button" className="secondary-button" onClick={() => setSettings(DEFAULT_SETTINGS)} disabled={running}>
              Defaults
            </button>
          </div>

          <div className="solver-form-grid">
            <label>
              Run mode
              <select value={settings.run_mode} onChange={(event) => update("run_mode", event.target.value)}>
                {['steady', 'design', 'sensitivity', 'optimize', 'transient'].map((mode) => <option key={mode}>{mode}</option>)}
              </select>
            </label>
            <label>
              Time slots (dt t_end pairs)
              <input value={settings.tim_slot} onChange={(event) => update("tim_slot", event.target.value)} />
            </label>
            {NUMERIC_FIELDS.map(([name, label, step]) => (
              <label key={name}>
                {label}
                <input type="number" step={step} value={settings[name]} onChange={(event) => update(name, event.target.value)} />
              </label>
            ))}
          </div>

          <div className="solver-checks">
            <label><input type="checkbox" checked={settings.temp_solve} onChange={(event) => update("temp_solve", event.target.checked)} /> Solve temperature</label>
            <label><input type="checkbox" checked={settings.flag_write} onChange={(event) => update("flag_write", event.target.checked)} /> Write outputs</label>
            <label><input type="checkbox" checked={useCurrentGeometry} onChange={(event) => setUseCurrentGeometry(event.target.checked)} /> Write current Model geometry</label>
          </div>
        </div>

        <div className="solver-output-card">
          <div className="solver-card-heading">
            <div>
              <h2>Run output</h2>
              <p>{status}</p>
            </div>
          </div>
          <pre className="solver-output">{output || "Solver output will appear here."}</pre>
        </div>
      </div>
    </section>
  );
}
