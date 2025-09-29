.. _theory_parallelization:

===============
Parallelization
===============

This section describes the parallelization strategy used in the solver. The implementation is designed to scale from desktop computers to workstations and compute clusters. While many problems of interest are small enough to be executed on a desktop, larger cases (full plant integrated/coupled simulations, etc.) require the computational power of multi-node clusters. To support this range, a hybrid MPI+OpenMP approach is employed.

Domain Decomposition
====================
A model may consist of multiple flow circuits, where each circuit is defined in terms of its nodes and connecting faces at the basic level. The domain decomposition proceeds in two stages as described below:

#. **Model decomposition.**
   The total number of MPI ranks is first distributed among the circuits in the model. The allocation is weighted according to the size of each circuit, measured by a work metric that combines the number of nodes and faces. This ensures that larger circuits receive more ranks, while smaller circuits may be solved on less ranks. For each circuit, a dedicated MPI communicator is created via ``MPI_Comm_Split``.

#. **Circuit decomposition.**
   Within each circuit, the ranks belonging to its communicator are used to partition the circuit graph using METIS :cite:`metis2013`. The nodes are partitioned using ``METIS_PartGraphKway``, which aims to balance the workload while minimizing inter-process communication. After node partitioning, each face is assigned to the same rank as its upstream node. Ghost nodes and faces are identified at partition boundaries, and corresponding ghost data structures are created during initialization. These ghost entities provide the minimal boundary information needed for inter-rank communication during the simulation.

This hierarchical strategy allows the solver to adapt naturally to different problem sizes and processor counts. A single circuit may span multiple MPI ranks, while multiple circuits may be mapped onto different subsets of ranks or even share the same rank when the number of circuits exceeds the number of available processes.





.. _data-communication-computation:

Data Communication and Computation
----------------------------------

The steps of the algorithm in each time step for each circuit in the transient simulation in the parallel version are shown in :numref:`alg-transient-parallel`. All MPI ranks participate in the initialization phase, which includes reading the circuit geometry and simulation settings and allocating work. Steps 1, 2, and 3 are carried out by each rank for their owned nodes and faces. The node and face values are communicated whenever required during the steps described in the algorithm using standard PETSc calls (``VecGetArray``/``VecRestoreArray``, ``VecGetArrayRead``/``VecRestoreArrayRead``, ``VecGhostUpdateBegin``/``VecGhostUpdateEnd``, etc.).

In Step 4, each rank computes the preliminary flow rates and the coefficients only in their owned faces. Then their values are communicated to the ghost faces, as these values from the connecting ghost faces are required to compute pressure correction in the next Step 5. In Step 5, each rank first computes the elements of the global matrix :math:`A` and vector :math:`b` of Equation :eq:`eqn-pc`, populates and assembles them. Then, the assembled system is solved for pressure correction using the PETSc function ``KSPSolve``. The pressure correction values are then communicated to the ghost nodes as they are required in the next Step 6.

In Step 6, each rank updates the flow rates in their owned faces, then communicates these values to the ghost faces. Similarly, the values of pressure and velocity are updated in the owned nodes. The density value is computed (using the EOS) in the owned nodes. While pressure and velocity are communicated to the ghost nodes (as ghost node pressures are required in Step 4 of the next iteration), density is again computed in the ghost nodes using the updated static pressure and enthalpy values. Then the face quantities are computed in all owned faces, and the density value is communicated to the ghost faces.

In Step 7, each rank first checks convergence in their owned nodes and faces. Then, the rank convergence is communicated (using ``MPI_Allreduce``) across ranks to obtain global convergence. The whole process is repeated by all the ranks until global convergence is obtained. Once overall convergence is achieved, the converged values are updated for the next time step in both owned and ghost faces and nodes, and the program will be terminated.

.. _alg-transient-parallel:

.. rubric:: Algorithm: Transient calculation algorithm in parallel

Given a project with nodes and elements, partition them using METIS, and assign ownership of nodes and faces to each MPI rank.

* *Step 1.* Assume preliminary enthalpy :math:`\bar{h}` for all owned nodes.
* *Step 2.* Assume preliminary pressure :math:`\bar{p_0}` for all owned nodes.
* *Step 3.* Assume preliminary density :math:`\bar{\rho}` for all owned nodes.
* *Step 4.* Compute preliminary flow rate :math:`\bar{Q}` using Equation :eq:`eqn-momentum-disc`.

  a. Compute :math:`\bar{Q}`, :math:`a_j^{+}`, :math:`a_j^{-}` for all owned faces.
  b. Communicate :math:`\bar{Q}`, :math:`a_j^{+}`, :math:`a_j^{-}` for all ghost faces.

* *Step 5.* Compute pressure correction :math:`p'_0` using Equation :eq:`eqn-pc`.

  a. Compute global matrix and vector elements for the owned nodes and assemble.
  b. Solve the assembled system.
  c. Communicate :math:`p'_0` for all ghost nodes.

* *Step 6.* Update pressure :math:`p_0`, flow rate :math:`Q` using Equation :eq:`eqn-update` and compute density :math:`\rho` using the EOS.

  a. Update :math:`Q` in owned faces.
  b. Communicate :math:`Q` for all ghost faces.
  c. Update :math:`p_0`, :math:`V` for all owned nodes.
  d. Compute :math:`\rho` for all owned nodes.
  e. Communicate :math:`V`, :math:`p_0` for all ghost nodes.
  f. Compute :math:`\rho` for all ghost nodes.
  g. Compute quantities for all owned faces.
  h. Communicate :math:`\rho` for all ghost faces.

* *Step 7.* Check convergence of mass and momentum equations.

  a. Check for all owned nodes and faces.
  b. Communicate convergence for all ranks.

  **If converged:**

  * *Step 8.* Compute temperature :math:`T` in all solid nodes using Equation (18).
  * *Step 9.* Compute enthalpy :math:`h` in all fluid nodes using Equation (20).
  * *Step 10.* Check convergence of mass, momentum, and energy equations (fluid and solid).

    - **If converged:** **Stop.**

      i. Update quantities in owned and ghost nodes.
      ii. Update quantities in owned faces.
      iii. Update :math:`Q`, :math:`\rho` in ghost faces.

    - **Else:** repeat from Step 1.

  **Else:** repeat from Step 2.




.. bibliography::
   :style: unsrt
   :filter: docname in docnames
