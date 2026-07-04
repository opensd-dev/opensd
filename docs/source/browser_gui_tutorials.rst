=====================
Browser GUI Tutorials
=====================

The OpenSD web GUI provides a browser workflow for inspecting tutorial models,
editing geometry, running the solver, and plotting results. The in-app Help
workspace contains the most up-to-date click-by-click instructions and can be
opened with ``F1``.

Starting the GUI
================

From ``reactflow/opensd-web``:

.. code-block:: bash

   npm install
   cp .env.example .env
   npm run dev

Open the URL shown by Vite, normally ``http://localhost:5173``. If
authentication is enabled, use the username and password configured in
``.env``.

Inspect an existing tutorial
============================

1. Open the **Pre-processor** workspace.
2. Select **Import XML** and choose a tutorial ``geometry.xml`` file.
3. Use **Fit view** to frame the imported graph.
4. Open **Project information** to review component counts and circuit groups.
5. Double-click components to inspect or edit their XML attributes.
6. Export ``geometry.xml`` and ``geometry.layout.json`` after edits.

Build a simple browser model
============================

1. Clear the layout.
2. Add two nodes, a pipe, and boundary conditions from the component palette.
3. Connect node-to-pipe and pipe-to-node links to define the flow path.
4. Connect each boundary condition to the correct node.
5. Edit pipe and boundary-condition attributes in the component editor.
6. Use alignment tools to clean up the diagram.
7. Export geometry and layout files.

Run and postprocess
===================

1. Open the **Solver** workspace.
2. Set the working directory to a folder containing ``geometry.xml``.
3. Import or edit solver settings and run the executable.
4. Open **Postprocessor**.
5. Import an HDF5 result file or legacy ``output.res``.
6. Select circuit, component type, component filter, and attribute.
7. Overlay validation tables when available.
8. Export plot SVGs for reports.

Tutorial decks
==============

The Python tutorial decks under ``tutorials/`` can be used to generate
``geometry.xml`` inputs, then opened in the browser GUI. The browser workflow is
especially useful for inspecting PINET-derived decks before running them.
