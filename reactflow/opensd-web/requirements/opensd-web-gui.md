# OpenSD web GUI requirements

**Product:** `opensd-web` — browser UI for OpenSD geometry visualization and HDF5 postprocessing.  
**Stack:** React, Vite, React Flow v11, h5wasm.  
**Solver I/O:** Geometry XML from [opensd/geometry.py](../../../opensd/geometry.py); results HDF5 from C++ [SOL-070](../../../requirements/opensd-solver.md).

---

## 1. Application scope

### GUI-001 — Dual workspaces (Must)

The application shall provide two tabs: **Model** (geometry graph) and **Postprocess** (HDF5 plots).

**Acceptance:** Tab switch preserves loaded data; Model shows React Flow canvas; Postprocess shows import controls and line plot.

**Implementation:** [src/App.jsx](../src/App.jsx).

---

### GUI-002 — Import geometry XML (Must)

Users shall import OpenSD **geometry.xml** (or equivalent) and render the network graph.

**Acceptance:** File picker accepts `.xml`; parse errors surface in status text; model stats shown in sidebar.

**Implementation:** `parseGeometryXml()` in [src/App.jsx](../src/App.jsx).

---

### GUI-003 — Import HDF5 results (Must)

Users shall import **HDF5** result files for postprocessing.

**Acceptance:** File picker accepts `.h5`/`.hdf5`; circuit/node series plotted when variables exist.

**Implementation:** `parseHdf5Results()` in [src/App.jsx](../src/App.jsx).

---

### GUI-004 — Export graph JSON (Should)

Users may export the current React Flow node/edge list as JSON.

**Acceptance:** Download `opensd_flow.json` from sidebar control.

**Implementation:** [src/App.jsx](../src/App.jsx) `exportJSON`.

---

## 2. Canvas navigation

### GUI-010 — View toolbar (Must)

The model canvas shall provide **Fit view**, **zoom in**, and **zoom out** controls.

**Acceptance:** Toolbar visible on Model tab; fit runs after successful XML import.

**Implementation:** [src/ModelFlowCanvas.jsx](../src/ModelFlowCanvas.jsx).

---

### GUI-011 — Standard pan/zoom aids (Should)

React Flow **Controls** and **MiniMap** shall remain available.

**Implementation:** [src/ModelFlowCanvas.jsx](../src/ModelFlowCanvas.jsx).

---

## 3. Layout

### GUI-020 — One horizontal row per circuit (Must)

Each **circuit** shall be laid out on its own horizontal **row** (distinct vertical position).

**Acceptance:** Multiple circuits do not share the same baseline Y.

**Implementation:** [src/circuitLayout.js](../src/circuitLayout.js) `CIRCUIT_ROW_GAP`.

---

### GUI-021 — In-line fluid chain (Must)

Within a circuit, connected **nodes and pipes** shall appear in a single left-to-right **chain** (node → pipe → node → …), not in separate columns by type.

**Acceptance:** Order derived from pipe `unode`/`dnode` topology walk.

**Implementation:** `buildHorizontalSequence()` in [src/circuitLayout.js](../src/circuitLayout.js).

---

### GUI-022 — Circuit label at row start (Must)

The circuit identifier shall appear as the first element on the row, linked to the first component in the chain.

**Acceptance:** Rectangle labeled with circuit id; one edge to first node or pipe.

**Implementation:** [src/App.jsx](../src/App.jsx) `parseGeometryXml`.

---

### GUI-023 — Spacing between elements (Must)

Horizontal spacing between node and pipe slots shall be large enough for large networks to remain readable.

**Acceptance:** `NODE_STEP` and `PIPE_STEP` in layout (currently 64 and 96 px).

**Implementation:** [src/circuitLayout.js](../src/circuitLayout.js).

---

### GUI-024 — Complete pipe connectivity (Must)

Every pipe **upstream** and **downstream** fluid edge shall be drawn after all endpoint nodes exist.

**Acceptance:** No missing edges such as pipe → downstream node when that node appears later in layout order.

**Implementation:** Two-phase node creation then edge pass over full `pipes` list in [src/App.jsx](../src/App.jsx).

---

## 4. Node appearance

### GUI-030 — Fluid nodes as circles (Must)

**Fluid nodes** (`<node>`) shall render as **circles**.

**Implementation:** [src/FlowNode.jsx](../src/FlowNode.jsx), `type: "flow"`.

---

### GUI-031 — Pipes as rectangles (Must)

**Pipes** shall render as horizontal **rectangles**. Circuit labels and **hslabs** use the same node component with distinct styling.

**Implementation:** [src/PipeNode.jsx](../src/PipeNode.jsx), `type: "pipe"`.

---

### GUI-032 — BCs above target node (Must)

**Boundary conditions** shall render as small rectangles **above** the attached flow node.

**Implementation:** [src/BcNode.jsx](../src/BcNode.jsx), vertical layout in [src/circuitLayout.js](../src/circuitLayout.js).

---

### GUI-033 — Identifier on canvas (Must)

On-canvas text shall show **identifiers only** (truncated when long).

**Acceptance:** No ncell, diameter, volume, etc. on the shape itself.

**Implementation:** `shortLabel()` in [src/NodeTooltip.jsx](../src/NodeTooltip.jsx).

---

### GUI-034 — Details on hover (Must)

Full attributes shall appear in a **tooltip** on hover.

**Acceptance:** Tooltip lists identifier plus secondary fields from XML.

**Implementation:** [src/NodeTooltip.jsx](../src/NodeTooltip.jsx).

---

## 5. Fluid edges

### GUI-040 — Left/right fluid handles (Must)

Fluid connections shall use **left** (in) and **right** (out) handles on nodes and pipes (short sides of pipe rectangles).

**Implementation:** `flow-in` / `flow-out` handles in [src/FlowNode.jsx](../src/FlowNode.jsx), [src/PipeNode.jsx](../src/PipeNode.jsx).

---

### GUI-041 — Straight blue fluid edges (Must)

Fluid edges shall be **straight**, solid **blue** lines.

**Implementation:** `flowEdge()`, [src/App.css](../src/App.css).

---

### GUI-042 — Fluid direction arrows (Must)

Fluid edges shall show **direction** with small **open** (unfilled) arrowheads at the downstream end.

**Acceptance:** `MarkerType.Arrow`, reduced size (~10px).

**Implementation:** [src/edgeUtils.js](../src/edgeUtils.js) `FLOW_MARKER`.

---

### GUI-043 — No fluid edge labels (Must)

Fluid edges shall not display text labels (e.g. “up”, “down”) on the canvas.

**Implementation:** [src/App.jsx](../src/App.jsx) edge definitions.

---

## 6. Boundary condition edges

### GUI-050 — Vertical BC edges (Must)

BC-to-node connections shall be **vertical** straight lines.

**Acceptance:** BC source handle bottom; node target handle top (`bc-in`).

**Implementation:** `bcEdge()` in [src/App.jsx](../src/App.jsx).

---

### GUI-051 — BC styling (Must)

BC edges shall use distinct **red** styling.

**Implementation:** `.bc-edge` in [src/App.css](../src/App.css).

---

## 7. Heat transfer / hslab

### GUI-060 — Hslab heat edges (Must)

Heat transfer connections involving hslabs shall use **solid orange** lines with **open** arrowheads indicating direction.

**Acceptance:** Not dashed; not animated.

**Implementation:** `hslabEdge()`, `.hslab-edge` in [src/App.css](../src/App.css), [src/edgeUtils.js](../src/edgeUtils.js).

---

### GUI-061 — Pipe heat from long sides (Must)

Heat connections to/from **pipes** shall attach to **top** and **bottom** handles (long sides of the rectangle). Fluid flow remains on left/right.

**Implementation:** `ht-in` / `ht-out` on [src/PipeNode.jsx](../src/PipeNode.jsx); `resolveHeatHandles()` in [src/edgeUtils.js](../src/edgeUtils.js).

---

### GUI-062 — Conditional heat handles (Must)

Orange **heat connector** handles shall appear **only** on components that participate in at least one hslab heat edge.

**Acceptance:** Components with no hslab link show no `ht-in`/`ht-out` dots.

**Implementation:** `buildHeatHandleMap()` in [src/edgeUtils.js](../src/edgeUtils.js).

---

### GUI-063 — No layer nodes on canvas (Must)

**Hslab layers** shall not be drawn as separate graph nodes or edges.

**Acceptance:** Only hslab rectangle plus upstream/downstream heat links to fluid network.

**Implementation:** [src/App.jsx](../src/App.jsx) (layer loop removed).

---

### GUI-064 — Layers in hslab tooltip (Must)

Layer information shall be listed in the **hslab hover tooltip** (count, layer number, solid name, radial node count).

**Acceptance:** `layerTooltipLines()` appended to hslab details.

**Implementation:** [src/App.jsx](../src/App.jsx) `layerTooltipLines`.

---

## 8. Postprocess view

### GUI-070 — Circuit and pipe selection (Must)

Postprocess shall allow selecting **circuit**, **pipe filter**, and **node variable** for line plots.

**Implementation:** [src/App.jsx](../src/App.jsx) postprocess controls.

---

### GUI-071 — Derived temperature from enthalpy (Should)

When enthalpy is available, GUI may plot **temperature from total enthalpy** using user-supplied **Cp**.

**Implementation:** `getNodeValue`, `temperature_from_tenth` in [src/App.jsx](../src/App.jsx).

---

## 9. Non-goals

### GUI-090 — Not a solver (Must)

The web app shall **not** run or replace the OpenSD C++ solver; it only visualizes inputs and HDF5 outputs.

---

### GUI-091 — Layer graph (Must)

Internal hslab layer networks shall not be expanded on the main canvas (see GUI-063).

---

## Appendix A — Traceability (GUI)

| ID | Summary | Primary file(s) |
|----|---------|-------------------|
| GUI-001 | Model + Postprocess tabs | `App.jsx` |
| GUI-002 | XML import | `App.jsx` |
| GUI-010 | Fit/zoom toolbar | `ModelFlowCanvas.jsx` |
| GUI-020 | Row per circuit | `circuitLayout.js` |
| GUI-021 | Horizontal chain | `circuitLayout.js` |
| GUI-024 | All pipe edges | `App.jsx` |
| GUI-030 | Circle nodes | `FlowNode.jsx` |
| GUI-031 | Rectangle pipes | `PipeNode.jsx` |
| GUI-034 | Hover tooltips | `NodeTooltip.jsx` |
| GUI-042 | Fluid arrows | `edgeUtils.js` |
| GUI-050 | Vertical BCs | `BcNode.jsx`, `App.jsx` |
| GUI-060 | Solid orange heat | `edgeUtils.js`, `App.css` |
| GUI-061 | Top/bottom heat on pipes | `PipeNode.jsx` |
| GUI-062 | Conditional heat handles | `edgeUtils.js` |
| GUI-063 | No layer nodes | `App.jsx` |
| GUI-064 | Layers in tooltip | `App.jsx` |
