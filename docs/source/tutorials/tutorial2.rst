Tutorial 2: Steady-State Flow Distribution in a Compressed Air Network
======================================================================

Reference
---------

PINET reference: ``Tutorial 2 - SS flow distribution in a compressed air network.docx``.

OpenSD files:

* :download:`Notebook <../../../tutorials/tutorial2/tutorial.ipynb>`
* :download:`Browser layout <../../../tutorials/tutorial2/geometry.layout.json>`

Problem description
-------------------

A flow circuit filled with air, as shown below, is considered. The flow distribution in
the network is required to be estimated. The geometrical details of various pipes in the circuit
are given in the table below. A constant friction factor of 0.03 shall be considered for all the pipes.
Supply pressure (node 1 and node 14) is 6 bar, and delivery pressures (nodes 4, 7, 9, 10, 12,
16, 19, 20, 22, 25, 27, 28, 29) is 3 bar. A uniform temperature of 15 degrees C shall be assumed
throughout the circuit.

.. figure:: ../_static/tutorials/tutorial2/figure1.png
   :alt: Tutorial 2 compressed air network schematic
   :align: center

   Schematic of Flow Circuit

.. list-table:: Table 1: Geometrical Details
   :widths: auto

   * - Elements
     - Diameter (m)
     - Length (m)
   * - 1, 12
     - 0.019
     - 200
   * - 2-3, 9, 11, 13, 15, 19, 21, 24, 26
     - 0.01588
     - 400
   * - 4-8, 10, 14, 16-18, 20, 22-23, 25, 27-29
     - 0.010
     - 100

Modeling steps
--------------

1. This problem is similar to that in tutorial 1. Here, air is the working fluid and shall be imported from thiravam fluid library. Note that a pressure-pressure boundary condition is used in this problem compared to the pressure-mass flow rate boundary condition used in Tutorial 1. The PINET code can handle different types of flow boundary conditions. However, it has to be ensured that pressure is specified in at least one node in each flow circuit.

Results
-------

Verify the nodal pressures and element flow rates from the code are same as that given in the tables below, respectively.

.. list-table:: Table 2: Nodal Pressure (bar) Results
   :widths: auto

   * - Node no.
     - PINET
   * - 1
     - 6.0000
   * - 2
     - 5.2150
   * - 3
     - 4.1131
   * - 5
     - 3.8547
   * - 6
     - 3.2057
   * - 8
     - 3.0423
   * - 11
     - 4.1132
   * - 13
     - 5.2149
   * - 14
     - 6.0000
   * - 15
     - 3.9848
   * - 17
     - 3.5975
   * - 18
     - 3.1287
   * - 21
     - 3.5478
   * - 23
     - 3.5975
   * - 24
     - 3.1286
   * - 26
     - 3.9847

.. list-table:: Table 3: Element Volumetric Flow Rates (g/s)
   :widths: auto

   * - Element No.
     - PINET
   * - 1
     - 16.461
   * - 2
     - 8.032
   * - 3
     - 3.596
   * - 4
     - 3.376
   * - 5
     - 1.782
   * - 6
     - 1.594
   * - 7
     - 0.797
   * - 8
     - 0.797
   * - 9
     - -3.597
   * - 10
     - 4.437
   * - 11
     - -8.034
   * - 12
     - 16.464
   * - 13
     - 8.430
   * - 14
     - 4.135
   * - 15
     - -4.294
   * - 16
     - 2.801
   * - 17
     - 1.400
   * - 18
     - 1.400
   * - 19
     - -1.494
   * - 20
     - 2.987
   * - 21
     - 1.493
   * - 22
     - 2.801
   * - 23
     - 1.400
   * - 24
     - 4.294
   * - 25
     - 4.135
   * - 26
     - 8.429
   * - 27
     - 4.436
   * - 28
     - 3.817
   * - 29
     - 1.400
