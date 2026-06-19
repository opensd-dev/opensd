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

### GUI-004 — Export OpenSD geometry XML (Should)

Users may export the current model as OpenSD `geometry.xml`, including edited component attributes and live connectivity.

**Acceptance:** Export the model as an XML browser download. The browser controls the destination and duplicate-file naming. An unchanged import/export preserves the original XML bytes.

**Implementation:** [src/App.jsx](../src/App.jsx) `exportGeometry`, matching the serialization model in `opensd/geometry.py` and component `to_xml_element()` methods.

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

### GUI-012 — Canvas undo/redo (Must)

The model canvas shall support undoing recent graph-edit actions with **Ctrl+Z** and redoing them with **Ctrl+Y** or **Ctrl+Shift+Z**.

**Acceptance:** Moving components, adding components, rotating components, connecting components, deleting components, and restoring default layout can be undone and redone. Sidebar Undo and Redo buttons shall provide the same actions.

**Implementation:** Undo/redo history in [src/App.jsx](../src/App.jsx).

---

### GUI-013 — Drag selection and delete (Must)

The model canvas shall allow left-click drag selection of components and deletion of selected components.

**Acceptance:** Dragging a selection rectangle selects multiple components; selected components can be moved together and deleted with Delete/Backspace.

**Implementation:** React Flow selection settings in [src/ModelFlowCanvas.jsx](../src/ModelFlowCanvas.jsx).

---

### GUI-014 — Scrollable sidebar (Must)

The left sidebar shall scroll vertically when controls exceed viewport height.

**Acceptance:** Bottom controls remain reachable without browser zoom or refresh.

**Implementation:** `.sidebar` in [src/App.css](../src/App.css).

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

### GUI-025 — Persistent component layout (Must)

When users move components on the model canvas, the GUI shall preserve component positions for the same geometry file only after the user explicitly saves the layout.

**Acceptance:** Re-importing the same XML file restores locally saved positions by stable component id after **Save layout**; users can export/import a `.layout.json` sidecar to share positions across computers; unmatched new components fall back to automatic layout; refreshing or closing with unsaved layout changes raises a browser warning.

**Implementation:** Local browser layout storage plus JSON sidecar import/export in [src/App.jsx](../src/App.jsx).

---

### GUI-026 — Circuit workspace controls (Must)

The model workspace shall provide explicit controls to start a blank circuit and restore the imported/generated default layout.

**Acceptance:** **New circuit** clears the model canvas without requiring browser refresh; **Default layout** restores the generated layout and clears the stored layout for the current geometry.

**Implementation:** Circuit controls in [src/App.jsx](../src/App.jsx), fit behavior in [src/ModelFlowCanvas.jsx](../src/ModelFlowCanvas.jsx).

---

### GUI-027 — Copy/export selected figure (Should)

The model workspace should allow selected components to be copied or exported as a figure for use in external applications such as PowerPoint, Word, and Paint.

**Acceptance:** Selecting one or more components and pressing **Ctrl+C**, or choosing **Copy figure**, places a PNG image of the selected canvas region on the system clipboard where supported. Choosing **Export PNG**, or right-clicking the selected components and choosing export, downloads a PNG. Connections between selected components are included where available.

**Implementation:** Clipboard image export in [src/App.jsx](../src/App.jsx).

---

### GUI-029 — Duplicate selected components (Should)

The model workspace should allow selected components to be copied and pasted within the same canvas.

**Acceptance:** Selecting components and pressing **Ctrl+C** stores an internal graph copy. Pressing **Ctrl+V** creates new copied components with preserved internal connections and a slight positional offset. The duplicated components become selected and the action participates in undo/redo.

**Implementation:** Internal graph clipboard in [src/App.jsx](../src/App.jsx).

---

### GUI-028 — Component alignment tools (Should)

The model workspace should provide diagramming alignment controls similar to PowerPoint or Flownex.

**Acceptance:** Selected components can be aligned left/right/center/top/bottom/middle. Three or more selected components can be distributed horizontally or vertically. Alignment actions participate in undo/redo.

**Implementation:** Selection layout tools in [src/App.jsx](../src/App.jsx), controls styled in [src/App.css](../src/App.css).

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

**Acceptance:** Fluid node identifiers appear below the node circle so the node symbol can remain compact.

**Implementation:** `shortLabel()` in [src/labelUtils.js](../src/labelUtils.js).

---

### GUI-034 — Details on hover (Must)

Full attributes shall appear in a **tooltip** on hover.

**Acceptance:** Tooltip lists identifier plus secondary fields from XML.

**Implementation:** [src/NodeTooltip.jsx](../src/NodeTooltip.jsx).

---

### GUI-035 — Selected component rotation (Must)

When a component is selected, the GUI shall show a nearby clockwise rotate-icon control that rotates the component in 45 degree steps.

**Acceptance:** Selecting a pipe or other non-circular component reveals the rotate button; circular fluid nodes do not show rotation controls; pipe flow handles follow the pipe orientation.

**Implementation:** Rotate controls in [src/PipeNode.jsx](../src/PipeNode.jsx), [src/BcNode.jsx](../src/BcNode.jsx); rotation state in [src/App.jsx](../src/App.jsx).

---

### GUI-036 — Component palette symbols (Must)

The component palette shall show compact symbols that match the component shape used on the canvas.

**Acceptance:** Fluid nodes use circle glyphs; pipes/pumps/valves/hslabs/BCs use rectangle glyphs matching their canvas styling.

**Implementation:** Palette glyphs in [src/App.jsx](../src/App.jsx), styles in [src/App.css](../src/App.css).

---

## 5. Fluid edges

### GUI-040 — Multi-pipe fluid node connectivity (Must)

Fluid nodes shall act as junctions and may connect to any number of pipes. Fluid nodes shall not display upstream/downstream arrow glyphs of their own. A fluid edge shall not connect a node to itself.

**Acceptance:** Multiple pipes can connect to the same fluid node; fluid flow connections use unobtrusive handles rather than visible arrow glyphs; attempts to connect from and to the same node are rejected.

**Implementation:** Invisible four-sided `flow-*` handles in [src/FlowNode.jsx](../src/FlowNode.jsx); pipe-side flow state in [src/App.jsx](../src/App.jsx).

---

### GUI-041 — Straight blue fluid edges (Must)

Fluid edges shall be **straight**, solid **blue** lines with direction arrowheads at the downstream end. They shall choose the nearest top/bottom/left/right fluid-node handles from current component positions. Pipe endpoints meaningfully offset left/right from a fluid node shall keep left/right handles even when vertical offset is larger; pipe endpoints nearly centered above/below the node shall use top/bottom. When exactly two fluid edges prefer the same node-side handle, they shall use different sides where possible. With three or more fluid edges, each edge shall keep its natural side, using offset handle positions on the same side when helpful.

**Implementation:** `flowEdge()` in [src/App.jsx](../src/App.jsx), `FLOW_MARKER` in [src/edgeUtils.js](../src/edgeUtils.js), [src/App.css](../src/App.css).

---

### GUI-042 — No pipe-attached fluid arrows (Must)

Pipes shall not show extra attached upstream/downstream arrow glyphs on the component body.

**Acceptance:** Pipe components expose only their normal short-edge flow handles; no dashed or solid arrow stubs are rendered on unconnected or connected pipes.

**Implementation:** [src/PipeNode.jsx](../src/PipeNode.jsx), flow handle styles in [src/App.css](../src/App.css).

---

### GUI-043 — No fluid edge labels (Must)

Fluid edges shall not display text labels (e.g. “up”, “down”) on the canvas.

**Implementation:** [src/App.jsx](../src/App.jsx) edge definitions.

---

## 6. Boundary condition edges

### GUI-050 — Nearest-side BC edges (Must)

BC-to-node connections shall be straight lines attached to the nearest side of the fluid node. The BC endpoint shall use its top or bottom side, whichever faces the node.

**Acceptance:** Moving either endpoint reroutes the connection to the nearest left, right, top, or bottom handle on the fluid node and to the facing top or bottom handle on the BC.

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

Heat connections to/from **pipes** and hslabs shall attach to the nearest **top** or **bottom** handle based on their relative canvas positions. Heat edges shall be straight lines without enforced minimum bend length. Fluid flow remains on pipe short sides.

**Implementation:** Dynamic heat handle selection in [src/App.jsx](../src/App.jsx); `ht-top-*` / `ht-bottom-*` handles on [src/PipeNode.jsx](../src/PipeNode.jsx) and [src/FlowNode.jsx](../src/FlowNode.jsx).

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

## 8. Requirements management

### GUI-080 — Browser requirements management (Should)

The GUI should provide a browser-based way to view project requirements. Requirement edits shall be made in the native Markdown files.

**Acceptance:** Users can browse requirement IDs and read requirement text from the bundled canonical requirements document; no browser edit or save controls are provided.

**Implementation:** Requirements workspace in [src/App.jsx](../src/App.jsx), styles in [src/App.css](../src/App.css).

---

## 9. Postprocess view

### GUI-070 — Circuit and pipe selection (Must)

Postprocess shall allow selecting **circuit**, **pipe filter**, and **node variable** for line plots.

**Implementation:** [src/App.jsx](../src/App.jsx) postprocess controls.

---

### GUI-071 — Derived temperature from enthalpy (Should)

When enthalpy is available, GUI may plot **temperature from total enthalpy** using user-supplied **Cp**.

**Implementation:** `getNodeValue`, `temperature_from_tenth` in [src/App.jsx](../src/App.jsx).

---

## 10. Solver workspace

### GUI-090 — Edit settings and run solver (Must)

The web app shall provide a Solver tab for editing the settings supported by `opensd.Settings`, importing and exporting `settings.xml`, and running `/mnt/c/codes/opensd/build/opensd` against a selected OpenSD working directory.

**Acceptance:** A run writes the edited `settings.xml`, optionally writes the current Model geometry, invokes the solver without a shell, and displays the command, exit status, standard output, and standard error.

**Implementation:** [src/SolverPanel.jsx](../src/SolverPanel.jsx), [solverServer.js](../solverServer.js), and the Vite plugin registration in [vite.config.js](../vite.config.js).

---

## 11. Non-goals

---

### GUI-091 — Layer graph (Must)

Internal hslab layer networks shall not be expanded on the main canvas (see GUI-063).

---

## Appendix A — Traceability (GUI)

| ID | Summary | Primary file(s) |
|----|---------|-------------------|
| GUI-001 | Model + Solver + Postprocess tabs | `App.jsx` |
| GUI-002 | XML import | `App.jsx` |
| GUI-010 | Fit/zoom toolbar | `ModelFlowCanvas.jsx` |
| GUI-012 | Canvas undo/redo | `App.jsx` |
| GUI-013 | Drag selection and delete | `ModelFlowCanvas.jsx` |
| GUI-014 | Scrollable sidebar | `App.css` |
| GUI-020 | Row per circuit | `circuitLayout.js` |
| GUI-021 | Horizontal chain | `circuitLayout.js` |
| GUI-024 | All pipe edges | `App.jsx` |
| GUI-025 | Persistent layout | `App.jsx` |
| GUI-026 | Circuit workspace controls | `App.jsx`, `ModelFlowCanvas.jsx` |
| GUI-027 | Copy/export selected figure | `App.jsx` |
| GUI-028 | Alignment tools | `App.jsx`, `App.css` |
| GUI-029 | Duplicate selected components | `App.jsx` |
| GUI-030 | Circle nodes | `FlowNode.jsx` |
| GUI-031 | Rectangle pipes | `PipeNode.jsx` |
| GUI-034 | Hover tooltips | `NodeTooltip.jsx` |
| GUI-035 | Rotate selected component | `PipeNode.jsx`, `BcNode.jsx`, `App.jsx` |
| GUI-036 | Component palette symbols | `App.jsx`, `App.css` |
| GUI-040 | Multi-pipe fluid nodes | `FlowNode.jsx`, `App.jsx` |
| GUI-042 | No pipe-attached fluid arrows | `PipeNode.jsx`, `App.css` |
| GUI-050 | Vertical BCs | `BcNode.jsx`, `App.jsx` |
| GUI-060 | Solid orange heat | `edgeUtils.js`, `App.css` |
| GUI-061 | Dynamic top/bottom heat handles | `App.jsx`, `PipeNode.jsx`, `FlowNode.jsx` |
| GUI-062 | Conditional heat handles | `edgeUtils.js` |
| GUI-063 | No layer nodes | `App.jsx` |
| GUI-064 | Layers in tooltip | `App.jsx` |
| GUI-080 | Browser requirements management | `App.jsx`, `App.css` |
| GUI-090 | Edit settings and run solver | `SolverPanel.jsx`, `solverServer.js`, `vite.config.js` |
