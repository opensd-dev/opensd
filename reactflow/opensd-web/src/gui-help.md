# OpenSD Web GUI Help

Press **F1** at any time to open this help page.

## Workspaces

- **Pre-processor** imports, edits, lays out, and exports OpenSD geometry.
- **Solver** edits settings and runs the OpenSD executable in an allowed WSL working directory.
- **Postprocessor** imports HDF5 results and plots circuit variables or component attributes.
- **Help** displays this document.
- **Requirements** displays the bundled GUI requirements.

## Geometry and layout

- **Import XML** loads an OpenSD geometry file. **Export XML** writes current attributes and connectivity.
- **Clear layout** removes all current components and starts an empty model.
- **Project information** opens live file, layout-status, component, connection, and per-circuit statistics.
- Layout JSON **Export layout/Import layout** shares component positions with another browser.
- **Fit view**, **+**, and **−** fit or zoom the model canvas. The minimap and React Flow controls remain available.
- Drag a component to move it. Drag empty canvas space to select. Left-to-right requires full enclosure; right-to-left accepts intersection.

## Selection and editing

- Hold **Ctrl** (or **Command**) and click to add components or connections to the selection.
- **Delete/Backspace** removes selected components or connections.
- Double-click a component to edit its attributes. Press **Escape** to close without applying changes.
- **Ctrl+C/Ctrl+V** copies and pastes selected graph components. **Ctrl+Z** undoes; **Ctrl+Y** or **Ctrl+Shift+Z** redoes.
- **Ctrl+C** places a clean PNG on the clipboard where supported and stores an internal graph copy. The selection context menu exports PNG for compatibility, compact SVG for scalable report graphics, or a tightly fitted high-resolution PDF for documents and printing. All exports include internal connections but omit unselected canvas content, selection outlines, selection rectangles, handles, and rotate controls.

## Alignment tools

- **Align vertical/horizontal** places selected component centers on one axis.
- **Transpose** rotates the selected layout and pipe-like component orientations 90° clockwise.
- **Move arrows** nudge the selection. **Closer/Farther H/V** changes spacing. **Dist H/V** distributes centers evenly.

## Components and connections

- Choose or drag **Node, Pipe, Pump, Valve, HSlab,** or **BC** from the component palette.
- Create a connection by dragging from one invisible connection port to another. A single click does not create a connection.
- Fluid nodes connect through pipes, pumps, or valves; direct node-to-node connections are rejected.
- Orange heat links connect hslabs to pipe/node thermal endpoints; direct hslab-to-hslab links are rejected.
- Circular BCs provide eight nearest-direction ports. Red BC links automatically move to the closest perimeter ports.
- Select a non-circular component to show its rotate control. Each click rotates it by 45°.

## Solver

- Select an existing absolute WSL working directory, edit/import settings, choose threads, and run.
- Working directories are allowed under `/mnt/c` by default. Administrators can configure `OPENSD_WORKING_ROOTS`.
- The run panel reports the command, exit code, standard output, and standard error.

## Postprocessor

- Import an `.h5`, `.hdf5`, or `output.res` result, then choose circuit, component type, filter, and attribute. Nodes, pipes, and faces use the same selection controls.
- Import validation `.txt`, `.csv`, or `.dat` tables from the same run folder to overlay reference series on the active plot.
- For regenerated transient results, pipe endpoint signals such as `pipe22_downstream` can be selected directly.
- Temperature-from-enthalpy plots also expose the Cp value used for conversion.

## Mouse and keyboard summary

- **F1:** Help
- **Escape:** Close attributes editor
- **Ctrl+C / Ctrl+V:** Copy/paste selection
- **Ctrl+Z / Ctrl+Y:** Undo/redo
- **Delete / Backspace:** Delete selection
- **Mouse wheel:** Zoom
- **Middle/right drag:** Pan
- **Left drag on canvas:** Selection rectangle
