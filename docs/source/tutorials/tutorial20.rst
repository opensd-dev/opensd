Tutorial 20: Dementev Blowdown Pipe Variant
===========================================

Reference
---------

OpenSD file:

* :download:`Input script <../../../tutorials/tutorial20/tutorial.py>`

Problem description
-------------------

This PINET-derived tutorial exercises a Dementev blowdown pipe variant. The
case is included as an OpenSD tutorial deck for transient compressible-flow
regression checks.

Modeling steps
--------------

1. Create the pipe, inlet volume or source condition, outlet boundary, and fluid setup in the browser pre-processor. Enter the transient pressure or flow action data from the reference deck, then export the geometry and settings files into ``tutorials/tutorial20``.

Results
-------

Run the case from the browser **Solver** workspace and load the result in
**Postprocessor**. Plot pressure and flow histories at the key pipe locations.

Expected checks
^^^^^^^^^^^^^^^

The case should complete without solver failure and produce a monotonic
blowdown response consistent with the boundary-condition setup.
