Tutorial 10: Natural Circulation in an open loop
================================================

Reference
---------

PINET reference: ``Tutorial 10 - Natural Circulation in an open loop.docx``.

OpenSD files:

* :download:`Input script <../../../tutorials/tutorial10/tutorial.py>`
* :download:`Browser layout <../../../tutorials/tutorial10/geometry.layout.json>`

Problem description
-------------------

Three pipes are connected to form an open loop in the shape of 'U'. The pipes have a diameter of
0.1 m and a length of 0.2 m. The loop is filled with water, and its ends are open to the
atmosphere (ambient temperature = 15 degrees C, pressure = 1 bar). The horizontal pipe at the
bottom is uniformly heated with a power of 20 kW. Assume the friction factor in the pipes as
0.05. It is required to estimate the loop flow rate and temperature difference.

Modeling steps
--------------

1. (Note the input file for this tutorial problem is already saved in /deck/example18.py)

2. Draw the layout and label the components for the problem as shown below:

.. figure:: ../_static/tutorials/tutorial10/figure1.png
   :alt: Tutorial 10 open natural-circulation loop layout
   :align: center

   Open natural-circulation loop layout.

3. Create the circuit in the GUI and assign the specified fluid.

4. Add the required nodes in the GUI and set their elevations and initial guesses.

5. Note that a guess value slightly higher than the initial temperature is specified in node 4 to have an initial temperature gradient in the circuit to initiate natural circulation in the first iteration

6. Add the pipe components in the GUI, connect the nodes, and enter the geometry, heat input, and loss data.

7. Note that the pipes are incremented (say, 10 nos.) to have more accurate results since cell average densities are used to calculate the driving head for natural circulation

8. Attach boundary conditions to the nodes

9. Note that these values correspond to the ambient conditions specified in the problem

10. Set convergence criteria

11. Note that for natural circulation, since it is difficult to get convergence with default convergence criteria (residue ~10-10), higher residue values (~10-7) are used in the convergence criteria. However, the results obtained are in general accurate enough for practical purposes.

Results
-------

Verify the mass flow rate (volume flow rate x density) (in any pipe) is equal to 0.8834 kg/s
from the output file. The temperature difference between the hot and cold leg should be 5.409
degrees C.
