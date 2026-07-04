Tutorial 15: Designer Functionality
===================================

Reference
---------

OpenSD files:

* :download:`Input script <../../../tutorials/tutorial15/tutorial.py>`

Problem description
-------------------

Consider three parallel pipes of diameter 0.025 m. The pipes have different lengths say, 100 m,
300 m and 500 m. The pipes carry water at 30 bar and 30 degrees C. The total flow rate through
the pipes should be 20 kg/s. In order to get equal flow through the pipes, find the secondary
loss to be added to the shorter pipes.

Modeling steps
--------------

1. The circuit is built following the usual procedure.

2. Add the ``Kforward`` values of the shorter pipes as designer parameters.

3. Add two designer result equations for the mass-flow differences between the reference branch and the two shorter branches.

4. Ensure that the number of designer parameters equals the number of result equations; this is required for design functionality, unlike sensitivity or optimizer workflows.

5. Run the designer to get the solution.

Results
-------

The "Kforward" values for the shorter pipes should be obtained as 20.2629 and 40.5259.
