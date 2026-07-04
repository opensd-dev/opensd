Tutorial 18: Two-Phase Tank Energy-Source Transient
===================================================

Reference
---------

OpenSD file:

* :download:`Input script <../../../tutorials/tutorial18/tutorial.py>`

Problem description
-------------------

This PINET-derived tutorial exercises a two-phase tank transient with an
energy source. The case is included as an OpenSD tutorial deck for regression
and postprocessing checks.

Modeling steps
--------------

1. Create the tank model in the browser pre-processor, assign the two-phase fluid state, and enter the initial pressure, inventory, and thermal state values used by the reference deck.

2. Add the energy source and any boundary-condition components needed by the transient. Set the transient schedule in the solver settings, then export the geometry and settings files into ``tutorials/tutorial18``.

Results
-------

Run the case from the browser **Solver** workspace and load the result in
**Postprocessor**. Plot pressure, enthalpy or temperature, and any
inventory-related result channels produced by the case.

Expected checks
^^^^^^^^^^^^^^^

The transient should remain numerically stable and preserve physically
consistent tank pressure and energy trends.
