.. _theory_parallelization:

=======================
Parallel Implementation
=======================

This section describes the parallelization strategy used in the solver. The implementation is designed to scale from desktop computers to workstations and compute clusters. While many problems of interest are small enough to be executed on a desktop, larger cases (full plant integrated/coupled simulations, etc.) require the computational power of multi-node clusters. To support this range, a hybrid MPI+OpenMP approach is employed.

Domain Decomposition
====================
A model may consist of multiple flow circuits, where each circuit is defined in terms of its nodes and connecting faces at the basic level. The domain decomposition proceeds in two stages as described below:

#. **Model decomposition.**
   The total number of MPI ranks is first distributed among the circuits in the model. The allocation is weighted according to the size of each circuit, measured by a work metric that combines the number of nodes and faces. This ensures that larger circuits receive more ranks, while smaller circuits may be solved on less ranks. For each circuit, a dedicated MPI communicator is created via ``MPI_Comm_Split``.

#. **Circuit decomposition.**
   Within each circuit, the ranks belonging to its communicator are used to partition the circuit graph using METIS :cite:`metis2013`. The nodes are partitioned using ``METIS_PartGraphKway``, which aims to balance the workload while minimizing inter-process communication. After node partitioning, each face is assigned to the same rank as its upstream node. Ghost nodes and faces are identified at partition boundaries, and corresponding ghost data structures are created during initialization. These ghost entities provide the minimal boundary information needed for inter-rank communication during the simulation.

This hierarchical strategy allows the solver to adapt naturally to different problem sizes and processor counts. A single circuit may span multiple MPI ranks, while multiple circuits may be mapped onto different subsets of ranks or even share the same rank when the number of circuits exceeds the number of available processes.







.. bibliography::
   :style: unsrt
   :filter: docname in docnames
