# OpenSD web GUI requirements

**Product:** `opensd-web` — browser UI for OpenSD geometry visualization and HDF5 postprocessing.  
**Stack:** React, Vite, React Flow v11, h5wasm.  
**Solver I/O:** Geometry XML from [opensd/geometry.py](../../../opensd/geometry.py); results HDF5 from C++ [SOL-070](../../../requirements/opensd-solver.md).

---

## 1. Application scope

### GUI-001 — Application workspaces (Must)

The application shall provide tabs in this order: **Pre-processor** (geometry graph), **Solver**, **Postprocessor** (HDF5 plots), **Help**, and **Requirements**.

**Acceptance:** Tab switching preserves loaded data; Pre-processor shows the React Flow canvas; Solver shows solver controls; Postprocessor shows result import controls and line plots; Help displays the bundled GUI guide and opens with **F1**.

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

**Acceptance:** Moving components, adding components, rotating components, connecting components, and deleting components can be undone and redone. A continuous mouse drag records one undo step at drag start, so Ctrl+Z returns directly to the previous position instead of traversing intermediate pointer positions.

**Implementation:** Undo/redo history in [src/App.jsx](../src/App.jsx).

---

### GUI-013 — Drag selection and delete (Must)

The model canvas shall support directional drag selection, additive Ctrl/Command-click selection, visible selection highlighting, and deletion of selected components or connections.

**Acceptance:** Dragging left-to-right selects components fully enclosed by the rectangle. Dragging right-to-left selects components that are fully or partially enclosed. Ctrl/Command-click adds individual components to the selection. Selected components and connections are clearly highlighted; either can be deleted with Delete/Backspace.

**Implementation:** React Flow selection settings in [src/ModelFlowCanvas.jsx](../src/ModelFlowCanvas.jsx).

---

### GUI-014 — Scrollable sidebar (Must)

The left sidebar shall scroll vertically when controls exceed viewport height.

**Acceptance:** Bottom controls remain reachable without browser zoom or refresh.

**Implementation:** `.sidebar` in [src/App.css](../src/App.css).

---

## 3. Layout

### GUI-020 — Separate layout region per circuit (Must)

Each **circuit** shall be laid out in its own non-overlapping horizontal region, with circuits placed side by side.

**Acceptance:** Components and BC stacks from different circuits do not overlap. All circuits use one shared elevation-to-y mapping, so nodes from different circuits with the same `elevation` appear on the same horizontal level.

**Implementation:** Dynamic circuit bounds in [src/circuitLayout.js](../src/circuitLayout.js).

---

### GUI-021 — Layered fluid topology (Must)

Within a circuit, connected **nodes, pipes, and pumps** shall use topology- and elevation-derived placement. Connections between equal-elevation nodes advance horizontally; connections whose node elevations differ remain in the same x-column and advance vertically, with higher elevations placed above lower elevations.

**Acceptance:** Node `elevation` values are sorted and used only to establish relative vertical order; numeric elevation differences do not scale canvas distance. Adjacent elevation ranks use compact minimum spacing expanded only when needed for BC stacks. Vertical pipes receive a 90-degree default orientation. Branches spread into collision-free lanes where needed, component boxes do not overlap, and dense networks avoid coincident connector paths where practical.

**Implementation:** `buildHorizontalSequence()` in [src/circuitLayout.js](../src/circuitLayout.js).

---

### GUI-022 — Circuit label at row start (Must)

The circuit identifier shall be positioned immediately to the left of the actual first connected component rather than at an unrelated region corner.

**Acceptance:** Rectangle labeled with circuit id remains close to the first node, pipe, or pump; one gray dotted membership edge links them and remains visually distinct from physical blue fluid-flow and orange heat-flow edges.

**Implementation:** [src/App.jsx](../src/App.jsx) `parseGeometryXml`.

---

### GUI-023 — Spacing between elements (Must)

Horizontal and vertical spacing shall be large enough for component shapes and labels to remain readable.

**Acceptance:** Layer and lane spacing account for branches, BC stacks, and component collision clearance.

**Implementation:** [src/circuitLayout.js](../src/circuitLayout.js).

---

### GUI-024 — Complete pipe connectivity (Must)

Every pipe **upstream** and **downstream** fluid edge shall be drawn after all endpoint nodes exist.

**Acceptance:** No missing edges such as pipe → downstream node when that node appears later in layout order.

**Implementation:** Two-phase node creation then edge pass over full `pipes` list in [src/App.jsx](../src/App.jsx).

---

### GUI-025 — Persistent component layout (Must)

The GUI shall support explicit layout JSON export/import for preserving and sharing component positions.

**Acceptance:** Users can export/import a `.layout.json` sidecar to share positions across computers; imported positions are matched by stable component id; unmatched components retain automatic positions; refreshing or closing with unsaved layout changes raises a browser warning.

**Implementation:** Local browser layout storage plus JSON sidecar import/export in [src/App.jsx](../src/App.jsx).

---

### GUI-026 — Circuit workspace controls (Must)

The model workspace shall provide a **Clear layout** control to remove all current components and start a blank circuit.

**Acceptance:** **Clear layout** clears the model canvas without requiring browser refresh.

**Implementation:** Circuit controls in [src/App.jsx](../src/App.jsx), fit behavior in [src/ModelFlowCanvas.jsx](../src/ModelFlowCanvas.jsx).

---

### GUI-027 — Copy/export selected figure (Should)

The model workspace should allow selected components to be copied or exported as a figure for use in external applications such as PowerPoint, Word, and Paint.

**Acceptance:** Selecting one or more components and pressing **Ctrl+C** places a PNG image of the selected canvas region on the system clipboard where supported. The selection context menu offers PNG, scalable SVG, and tightly fitted high-resolution PDF downloads. Connections whose endpoints are both selected are included with their normal visual style. Rotate controls, connection handles, selection highlighting, selected-edge coloring, and selection rectangles are omitted so the result resembles a clean browser-canvas screenshot. SVG export removes unselected graph elements and embeds shared stylesheet rules once, avoiding impractically large files caused by repeated computed styles. PDF export uses a dedicated high-resolution render for clearer zooming and print output.

**Implementation:** Clipboard image export in [src/App.jsx](../src/App.jsx).

---

### GUI-029 — Duplicate selected components (Should)

The model workspace should allow selected components to be copied and pasted within the same canvas.

**Acceptance:** Selecting components and pressing **Ctrl+C** stores an internal graph copy. Pressing **Ctrl+V** creates new copied components with preserved internal connections and a slight positional offset. The duplicated components become selected and the action participates in undo/redo.

**Implementation:** Internal graph clipboard in [src/App.jsx](../src/App.jsx).

---

### GUI-028 — Component alignment tools (Should)

The model workspace should provide diagramming alignment controls similar to PowerPoint or Flownex.

**Acceptance:** Selected components can be aligned on a common vertical or horizontal axis, rotated 90 degrees clockwise as a group around the selection center to exchange horizontal and vertical layout, moved left/right/up/down in fixed increments, moved closer together or farther apart along either axis, and distributed horizontally or vertically. Transpose also advances each selected pipe-like component's own rotation by 90 degrees and swaps its occupied width/height when calculating the new position, preserving center alignment and producing a geometric rotation of the selection. The Transpose button appears last in the alignment-control grid. These actions participate in undo/redo.

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

**Boundary conditions** shall render as circles larger than fluid nodes **above** the attached flow node, with a visible `BC` mark and identifier label so they remain distinct from pipes.

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

**Acceptance:** Fluid-node tooltips include the current `elevation` value from the node attributes.

**Visibility:** The hovered component and tooltip render above all other canvas components and edges.

**Editor interaction:** Pressing **Escape** while the component attribute editor is open closes the editor without applying changes.

**Acceptance:** Tooltip lists identifier plus secondary fields from XML.

**Implementation:** [src/NodeTooltip.jsx](../src/NodeTooltip.jsx).

---

### GUI-035 — Selected component rotation (Must)

When a component is selected, the GUI shall show a nearby clockwise rotate-icon control that rotates the component in 45 degree steps.

**Acceptance:** Selecting a pipe or other non-circular component reveals the rotate button; the button remains at a fixed screen-relative offset while the component rotates so repeated clicks do not require pointer movement and renders above any hover tooltip; clicking or double-clicking the rotate button does not open the component attribute editor; circular fluid nodes and BCs do not show rotation controls; pipe flow handles follow the pipe orientation.

**Implementation:** Rotate controls in [src/PipeNode.jsx](../src/PipeNode.jsx); rotation state in [src/App.jsx](../src/App.jsx).

---

### GUI-036 — Component palette symbols (Must)

The component palette shall show compact symbols that match the component shape used on the canvas.

**Acceptance:** Fluid nodes use small circle glyphs; BCs use larger circle glyphs; pumps use a circular impeller/flow glyph; pipes, valves, and hslabs use rectangle glyphs matching their canvas styling.

**Implementation:** Palette glyphs in [src/App.jsx](../src/App.jsx), styles in [src/App.css](../src/App.css).

---

### GUI-037 — Pump component (Must)

The pre-processor shall provide a distinct pump component representing a two-node pressure-raising flow element.

**Acceptance:** The palette and canvas show a recognizable pump symbol with inlet/outlet flow handles. Imported `<vspump>` and `<hpump>` elements retain their model type and attributes; newly added pumps default to `<vspump>` attributes including `Nop`, `curve_speed`, and `curve_file`.

**Implementation:** Pump parsing and serialization in [src/App.jsx](../src/App.jsx), pump symbol in [src/PipeNode.jsx](../src/PipeNode.jsx) and [src/App.css](../src/App.css), based on `VSPump`/`HPump` in [opensd/turbo.py](../../../opensd/turbo.py).

---

### GUI-038 — Project statistics and information (Should)

The Pre-processor sidebar shall provide an on-demand **Project information** dialog without permanently occupying sidebar space.

**Acceptance:** The dialog displays geometry filename, layout key/status, live totals for component and connection types, and component counts grouped by circuit. It closes by button, backdrop click, or Escape.

**Implementation:** Project information dialog and live graph aggregation in [src/App.jsx](../src/App.jsx), styled in [src/App.css](../src/App.css).

---

## 5. Fluid edges

### GUI-040 — Multi-pipe fluid node connectivity (Must)

Fluid nodes shall act as junctions and may connect to any number of pipes. Fluid nodes shall not display upstream/downstream arrow glyphs of their own. A fluid edge shall not connect a node to itself, and direct fluid-node-to-fluid-node connections shall be rejected.

**Acceptance:** Multiple pipes can connect to the same fluid node; fluid flow connections use unobtrusive handles rather than visible arrow glyphs; self-connections and direct node-to-node connections are rejected. A single click never creates a connection; users must drag from one connection handle to another.

**Implementation:** Invisible four-sided `flow-*` handles in [src/FlowNode.jsx](../src/FlowNode.jsx); pipe-side flow state in [src/App.jsx](../src/App.jsx).

---

### GUI-041 — Straight blue fluid edges (Must)

Fluid edges shall be **straight**, solid **blue** lines with direction arrowheads at the downstream end. They shall choose the nearest top/bottom/left/right fluid-node handles from current component positions. Pipe endpoints meaningfully offset left/right from a fluid node shall keep left/right handles even when vertical offset is larger; pipe endpoints nearly centered above/below the node shall use top/bottom. When exactly two fluid edges prefer the same node-side handle, they shall use different sides where possible. With three or more fluid edges, each edge shall keep its natural side, using offset handle positions on the same side when helpful.

**Implementation:** `flowEdge()` in [src/App.jsx](../src/App.jsx), `FLOW_MARKER` in [src/edgeUtils.js](../src/edgeUtils.js), [src/App.css](../src/App.css).

**Occlusion:** A straight connection intersecting a non-endpoint component is elevated and made dashed so its path remains legible through the component. Its original connection style—including color, thickness, marker, and opacity—is preserved.

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

BC-to-node connections shall be straight lines attached to the nearest side of the fluid node. Circular BCs shall expose eight evenly distributed invisible perimeter ports and use the port nearest the connected node.

**Acceptance:** Moving either endpoint reroutes the connection to the nearest left, right, top, or bottom handle on the fluid node and to the nearest of the BC's north, northeast, east, southeast, south, southwest, west, or northwest ports. BC handles retain their connection hit area but are not visibly rendered as dots.

**Implementation:** `bcEdge()` in [src/App.jsx](../src/App.jsx).

---

### GUI-051 — BC styling (Must)

BC edges shall use distinct **red** styling.

**Implementation:** `.bc-edge` in [src/App.css](../src/App.css).

---

## 7. Heat transfer / hslab

### GUI-060 — Hslab heat edges (Must)

Heat transfer connections involving hslabs shall use **solid orange** lines with **open** arrowheads indicating direction.

**Acceptance:** Not dashed; not animated. Direct hslab-to-hslab connections are rejected; heat connections must involve an hslab and a pipe or node-side thermal endpoint.

**Implementation:** `hslabEdge()`, `.hslab-edge` in [src/App.css](../src/App.css), [src/edgeUtils.js](../src/edgeUtils.js).

**Default layout:** An hslab with exactly one unique connected pipe shall be placed close to that pipe; if the pipe is vertical, the hslab is also vertical and placed parallel beside it. An hslab connected to two pipes begins at the mean of their horizontal centers and displayed elevations, and is vertical when both pipes are vertical. If two hslabs have the same mean position, deterministic perpendicular offsets prevent exact overlap. Hslabs involving non-pipe components use the packed fallback area.

---

### GUI-061 — Pipe heat from long sides (Must)

Heat connections to/from **pipes** and hslabs shall attach to the nearest rotated **long edge** at each endpoint based on relative canvas positions. Heat edges shall be straight lines without enforced minimum bend length. Fluid flow remains on pipe short sides.

**Acceptance:** Moving a pipe or hslab continuously reroutes the visible edge to the nearest inward-facing long sides without making the connection disappear.

**Implementation:** Dynamic heat handle selection in [src/App.jsx](../src/App.jsx); both invisible long-side handle pairs remain mounted on participating components in [src/PipeNode.jsx](../src/PipeNode.jsx) and [src/FlowNode.jsx](../src/FlowNode.jsx).

---

### GUI-062 — Conditional heat handles (Must)

Heat connector handles shall retain an invisible connection hit area only on the exact long edges used by hslab heat connections.

**Acceptance:** No orange heat-port dots or stray orange segments are visible on the unused long edge of pipes, hslabs, or nodes; existing and newly created heat connections can still attach to the appropriate nearest handles.

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

**Acceptance:** A run accepts an existing absolute WSL working directory under an allowed root (default `/mnt/c`, configurable with comma-separated `OPENSD_WORKING_ROOTS`), writes the edited `settings.xml`, optionally writes the current Pre-processor geometry, invokes the solver without a shell, and displays the command, exit status, standard output, and standard error. Realpath validation prevents path traversal and symlink escape outside configured roots.

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
| GUI-001 | Pre-processor + Solver + Postprocessor tabs | `App.jsx` |
| GUI-002 | XML import | `App.jsx` |
| GUI-010 | Fit/zoom toolbar | `ModelFlowCanvas.jsx` |
| GUI-012 | Canvas undo/redo | `App.jsx` |
| GUI-013 | Directional/additive selection and delete | `ModelFlowCanvas.jsx`, `App.css` |
| GUI-014 | Scrollable sidebar | `App.css` |
| GUI-020 | Side-by-side circuit regions with shared elevation levels | `App.jsx`, `circuitLayout.js` |
| GUI-021 | Layered fluid topology | `circuitLayout.js` |
| GUI-024 | All pipe edges | `App.jsx` |
| GUI-025 | Persistent layout | `App.jsx` |
| GUI-026 | Circuit workspace controls | `App.jsx`, `ModelFlowCanvas.jsx` |
| GUI-027 | Copy/export selected figure | `App.jsx` |
| GUI-028 | Alignment, nudge, and spacing tools | `App.jsx`, `App.css` |
| GUI-029 | Duplicate selected components | `App.jsx` |
| GUI-030 | Circle nodes | `FlowNode.jsx` |
| GUI-031 | Rectangle pipes | `PipeNode.jsx` |
| GUI-032 | Circular BCs above target nodes | `BcNode.jsx`, `circuitLayout.js`, `App.css` |
| GUI-034 | Hover tooltips | `NodeTooltip.jsx` |
| GUI-035 | Rotate selected non-circular component | `PipeNode.jsx`, `App.jsx` |
| GUI-036 | Component palette symbols | `App.jsx`, `App.css` |
| GUI-037 | Pump component and XML | `App.jsx`, `PipeNode.jsx`, `App.css` |
| GUI-038 | Project statistics and information | `App.jsx`, `App.css` |
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
