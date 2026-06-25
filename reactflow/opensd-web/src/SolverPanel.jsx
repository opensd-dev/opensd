import { DEFAULT_SETTINGS, NUMERIC_FIELDS, settingsXml, useSolverPanelState } from "./solverState.js";

function downloadSettings(xml) {
  const url = URL.createObjectURL(new Blob([xml], { type: "application/xml;charset=utf-8" }));
  const anchor = document.createElement("a");
  anchor.href = url;
  anchor.download = "settings.xml";
  anchor.click();
  URL.revokeObjectURL(url);
}

export function SolverSidebarControls({ solver }) {
  const {
    settingsInput,
    settings,
    setSettings,
    inputDirectory,
    setInputDirectory,
    threads,
    setThreads,
    useCurrentGeometry,
    setUseCurrentGeometry,
    running,
    update,
    importSettings,
    run
  } = solver;

  return (
    <>
      <input
        ref={settingsInput}
        className="file-input"
        type="file"
        accept=".xml,application/xml,text/xml"
        onChange={(event) => importSettings(event.target.files?.[0])}
      />

      <section className="panel sidebar-panel">
        <div className="panel-title">Solver Run</div>
        <label className="sidebar-field">
          Working directory
          <input value={inputDirectory} onChange={(event) => setInputDirectory(event.target.value)} />
        </label>
        <label className="sidebar-field">
          Threads
          <input type="number" min="1" step="1" value={threads} onChange={(event) => setThreads(event.target.value)} />
        </label>
        <button className="secondary-button secondary-button--inline" onClick={() => settingsInput.current?.click()} disabled={running}>
          Import settings
        </button>
        <button className="secondary-button secondary-button--inline" onClick={() => downloadSettings(settingsXml(settings))} disabled={running}>
          Export settings
        </button>
        <button className="primary-button" onClick={run} disabled={running || !inputDirectory.trim()}>
          {running ? "Running..." : "Run solver"}
        </button>
      </section>

      <section className="panel sidebar-panel">
        <div className="panel-title">Solver Settings</div>
        <button type="button" className="secondary-button secondary-button--inline" onClick={() => setSettings(DEFAULT_SETTINGS)} disabled={running}>
          Defaults
        </button>

        <div className="solver-form-grid solver-form-grid--sidebar">
          <label>
            Run mode
            <select value={settings.run_mode} onChange={(event) => update("run_mode", event.target.value)}>
              {["steady", "design", "sensitivity", "optimize", "transient"].map((mode) => <option key={mode}>{mode}</option>)}
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

        <div className="solver-checks solver-checks--sidebar">
          <label><input type="checkbox" checked={settings.temp_solve} onChange={(event) => update("temp_solve", event.target.checked)} /> Solve temperature</label>
          <label><input type="checkbox" checked={settings.flag_write} onChange={(event) => update("flag_write", event.target.checked)} /> Write outputs</label>
          <label><input type="checkbox" checked={useCurrentGeometry} onChange={(event) => setUseCurrentGeometry(event.target.checked)} /> Write current Model geometry</label>
        </div>
      </section>
    </>
  );
}

export function SolverOutputPanel({ solver }) {
  return (
    <section className="workspace-pane solver-view">
      <div className="solver-output-card">
        <div className="solver-card-heading">
          <div>
            <h2>Run output</h2>
            <p>{solver.status}</p>
          </div>
        </div>
        <pre className="solver-output">{solver.output || "Solver output will appear here."}</pre>
      </div>
    </section>
  );
}

export default function SolverPanel({ currentGeometryXml }) {
  const solver = useSolverPanelState(currentGeometryXml);

  return (
    <section className="workspace-pane solver-view solver-view--legacy">
      <SolverSidebarControls solver={solver} />
      <SolverOutputPanel solver={solver} />
    </section>
  );
}
