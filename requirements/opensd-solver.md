# OpenSD solver requirements

**Product:** OpenSD — open-source system dynamics code for thermal–fluid networks.  
**Stack:** Python model setup (`opensd/`), C++ solver (`src/`, `include/opensd/`), XML input, HDF5 results.  
**Theory reference:** [Governing equations](../docs/source/theory/governing_equations.rst), [Discretized equations](../docs/source/theory/discretized_equations.rst).

---

## 1. Scope and architecture

### SOL-001 — System purpose (Must)

OpenSD shall simulate **one-dimensional thermal–fluid networks** (pipes, nodes, boundaries) with optional **solid / heat-slab** thermal coupling.

**Acceptance:** User can define geometry in Python, export XML, run the `opensd` executable, and obtain solution fields for postprocessing.

**Implementation:** [opensd/](../opensd/), [src/](../src/), [include/opensd/](../include/opensd/).

---

### SOL-002 — Python front end and C++ solver (Must)

Model setup, XML generation, and run orchestration shall be provided in **Python**; time-consuming solution shall run in a **compiled C++** executable invoked via subprocess.

**Acceptance:** `opensd.executor.run()` launches `opensd` with optional MPI/PETSc/OpenMP thread flags.

**Implementation:** [opensd/executor.py](../opensd/executor.py), C++ main in `src/`.

---

### SOL-003 — Documentation alignment (Should)

User-facing theory and tutorials shall remain in Sphinx under `docs/source/`; this file shall not duplicate full equation derivations.

**Acceptance:** Requirements reference theory docs where physics is specified.

---

## 2. Governing physics (fluid)

### SOL-010 — 1D fluid equations (Must)

The flow solver shall solve **mass, momentum, and energy** conservation for 1D flow in pipe-like components.

**Acceptance:** Documented in theory guide; pipe discretization uses cell/face structure consistent with 1D formulation.

**Implementation:** [docs/source/theory/governing_equations.rst](../docs/source/theory/governing_equations.rst), [src/pipe.cpp](../src/pipe.cpp), [opensd/pipe.py](../opensd/pipe.py).

---

### SOL-011 — Homogeneous two-phase (Must)

For two-phase fluid type, a **homogeneous flow** mixture model shall be used.

**Acceptance:** Circuit `fltype` includes `two_phase`; property calls use appropriate CoolProp backend.

**Implementation:** [opensd/circuit.py](../opensd/circuit.py) `FluidType`, CoolProp integration.

---

### SOL-012 — Total pressure formulation (Must)

Momentum shall support a **total pressure** form for improved numerical treatment of acceleration terms in pipes.

**Acceptance:** Theory documents \(p_0 = p + \frac{1}{2}\rho V^2\) and momentum in total-pressure form.

**Implementation:** [docs/source/theory/governing_equations.rst](../docs/source/theory/governing_equations.rst).

---

### SOL-013 — Fluid property library (Must)

Fluid thermodynamic properties shall be obtained via **CoolProp** by default, with optional **user-defined** fluid modules.

**Acceptance:** `Circuit.assign_fluid(flname, fltype, fllib)` supports `CoolProp` and `User` libraries.

**Implementation:** [opensd/circuit.py](../opensd/circuit.py).

---

## 3. Thermal / solid physics

### SOL-020 — Heat slab (HSlab) (Must)

The solver shall support **heat slabs** (`hslab`) with layered solid conduction and coupling to fluid nodes or pipes.

**Acceptance:** Python `HSlab` / `SNode` / layers; XML `<hslab>` and `<layer>` elements; C++ heat transfer solver.

**Implementation:** [opensd/hslab.py](../opensd/hslab.py), [src/ht_solver.cpp](../src/ht_solver.cpp), [include/opensd/ht_solver.h](../include/opensd/ht_solver.h).

---

### SOL-021 — Fluid–solid coupling (Must)

Heat slabs shall couple to fluid components via declared **upstream/downstream** attachment (`uvar`/`dvar`, `ucomp`/`dcomp`).

**Acceptance:** Geometry XML and Python API specify pipe or node connections; energy exchange appears in slab/node equations.

**Implementation:** [opensd/hslab.py](../opensd/hslab.py), [src/sfacether.cpp](../src/sfacether.cpp).

---

### SOL-022 — Solid materials (Must)

Layer materials shall be definable via configured solid libraries (e.g. user modules, external libraries where integrated).

**Acceptance:** Layer `solname` / `sollib` resolved when building solid state objects.

**Implementation:** [opensd/hslab.py](../opensd/hslab.py) `SNode`.

---

## 4. Network components

### SOL-030 — Flow circuit container (Must)

A **circuit** shall group fluid nodes, pipes, boundary conditions, and related flow elements under one fluid definition.

**Acceptance:** Python `Circuit`; XML `<circuit identifier="..." flname="..." fltype="...">`; registry of circuits for export.

**Implementation:** [opensd/circuit.py](../opensd/circuit.py), [src/circuit.cpp](../src/circuit.cpp).

---

### SOL-031 — Fluid nodes (Must)

**Nodes** shall represent control volumes with compatible primary variables (pressure, enthalpy/temperature, etc.) and optional fixed-variable constraints.

**Acceptance:** Python `Node`; XML `<node>`; HDF5 node groups under circuit.

**Implementation:** [opensd/node.py](../opensd/node.py), [src/node.cpp](../src/node.cpp).

---

### SOL-032 — Pipes (Must)

**Pipes** shall connect two nodes (`unode`, `dnode`), with diameter, length, cell count `ncell`, friction options, and elevation consistency checks.

**Acceptance:** Length not less than elevation difference; internal faces/cells created for discretization.

**Implementation:** [opensd/pipe.py](../opensd/pipe.py), [src/pipe.cpp](../src/pipe.cpp).

---

### SOL-033 — Boundary conditions (Must)

**Boundary conditions** shall assign constrained variables (`var`, `val`) to a target node.

**Acceptance:** Python `BC`; XML `<bc>`; applied in solver BC handling.

**Implementation:** [opensd/bc.py](../opensd/bc.py), [src/bc.cpp](../src/bc.cpp).

---

### SOL-034 — Additional flow elements (Should)

The codebase shall support extended flow components where implemented (e.g. pumps, orifices, branches, turbo machinery) as registered on the circuit.

**Acceptance:** Circuit lists `pumps`, `orifices`, `gers`, `branches`, `faces`; corresponding C++ types exist.

**Implementation:** [opensd/circuit.py](../opensd/circuit.py), [include/opensd/pump.h](../include/opensd/pump.h), related `src/` units.

---

## 5. Geometry and input I/O

### SOL-040 — Geometry XML export (Must)

The Python **Geometry** API shall export a root `<geometry>` document containing circuits and heat slabs.

**Acceptance:** `Geometry.export_to_xml()` writes valid XML consumed by the C++ reader.

**Implementation:** [opensd/geometry.py](../opensd/geometry.py), [src/geometry.cpp](../src/geometry.cpp).

---

### SOL-041 — Settings XML (Must)

Simulation controls shall be read from a **settings** XML file (run mode, time slots, iteration limits, convergence tolerances, relaxation factors, output flags).

**Acceptance:** Python `Settings` mirrors C++ `settings::` namespace; XML read in `read_settings_xml`.

**Implementation:** [opensd/settings.py](../opensd/settings.py), [include/opensd/settings.h](../include/opensd/settings.h).

---

### SOL-042 — Run modes (Must)

The solver shall support configured **run modes**: at minimum steady, transient; design, sensitivity, and optimize enumerated in API.

**Acceptance:** `RunMode` enum in Python and C++; `run_mode` drives solution driver.

**Implementation:** [opensd/settings.py](../opensd/settings.py) `RunMode`, [include/opensd/settings.h](../include/opensd/settings.h).

---

### SOL-043 — Transient time slots (Must)

Transient runs shall use **time slot** definitions (`delt`, end time) from settings.

**Acceptance:** `tim_slot` array in Python `Settings` and C++ `settings::tim_slot`.

**Implementation:** [opensd/settings.py](../opensd/settings.py).

---

## 6. Numerics and convergence

### SOL-050 — Nonlinear iteration controls (Must)

Settings shall expose **maximum iteration counts** and **convergence criteria** for flow, temperature, and heat transfer.

**Acceptance:** Defaults include `no_main_iter`, `no_flow_iter`, `conv_crit_flow`, `conv_crit_temp_SS`, `conv_crit_ht`, etc.

**Implementation:** [opensd/settings.py](../opensd/settings.py), [src/convergence.cpp](../src/convergence.cpp).

---

### SOL-051 — Relaxation factors (Must)

Under-relaxation (`alpha_mom`, `alpha_ener`, `alpha_heat`) shall be configurable for stability.

**Acceptance:** Values read from settings and used in C++ solution updates.

**Implementation:** [opensd/settings.py](../opensd/settings.py), [include/opensd/settings.h](../include/opensd/settings.h).

---

### SOL-052 — Steady-state circuit flag (Must)

Each circuit may enable or disable steady-state solution via `solveSS`.

**Acceptance:** XML attribute `solveSS` on `<circuit>`; Python `Circuit.solveSS`.

**Implementation:** [opensd/circuit.py](../opensd/circuit.py).

---

## 7. Execution and parallelism

### SOL-060 — Local executable launch (Must)

Python shall launch the OpenSD binary with working directory and optional capture of stdout.

**Acceptance:** `opensd.executor.run(output=..., cwd=..., opensd_exec=...)`.

**Implementation:** [opensd/executor.py](../opensd/executor.py).

---

### SOL-061 — MPI and threading (Should)

The runner shall accept **MPI** command prefixes and **OpenMP** thread count (`-s N`) for parallel execution when the binary is built with those features.

**Acceptance:** `mpi_args`, `petsc_args`, `threads` passed to CLI builder.

**Implementation:** [opensd/executor.py](../opensd/executor.py).

---

### SOL-062 — Parallel theory (Should)

Parallel decomposition approach shall be documented for developers.

**Acceptance:** Theory page on parallelization exists.

**Implementation:** [docs/source/theory/parallelization.rst](../docs/source/theory/parallelization.rst).

---

## 8. Output and results

### SOL-070 — HDF5 results (Must)

The C++ solver shall write **HDF5** results with hierarchical groups for circuits, nodes, pipes, faces, and BCs.

**Acceptance:** `save_to_hdf5` / `load_from_hdf5` on circuit and child objects; GUI postprocess reads `circuits/.../nodes`.

**Implementation:** [src/circuit.cpp](../src/circuit.cpp), [include/opensd/hdf5_interface.h](../include/opensd/hdf5_interface.h).

---

### SOL-071 — Solution fields on nodes (Must)

Node HDF5 output shall include primary solution guesses (e.g. temperature, pressure, enthalpy fields) suitable for plotting along pipes.

**Acceptance:** Web GUI reads `ttemp_gues`, `tenth_gues`, etc. from node records.

**Implementation:** [src/node.cpp](../src/node.cpp), [reactflow/opensd-web/src/App.jsx](../reactflow/opensd-web/src/App.jsx) `parseHdf5Results`.

---

### SOL-072 � Transient output.res fields (Must)

Transient runs shall write a fresh `output.res` for the latest run only, rather than appending across runs.

**Acceptance:** `output.res` contains time, node solution fields, pipe-level `vflow`/`velocity`/`mflow`, face-level `vflow`/`velocity`/`mflow`, and pipe upstream/downstream endpoint fields such as `ttemp_gues:<pipe>_downstream` for postprocessing. `opensd.result.Result` can read the columnar output.

**Implementation:** [src/post.cpp](../src/post.cpp), [opensd/result.py](../opensd/result.py).

---

## 9. Python API ergonomics

### SOL-080 — Public package surface (Must)

`import opensd` shall expose geometry, settings, executor, result, fluid/solid helpers, conditions, and initial guess utilities.

**Acceptance:** Exports listed in `opensd/__init__.py`.

**Implementation:** [opensd/__init__.py](../opensd/__init__.py).

---

### SOL-081 — Component lookup (Must)

Project-level **get_comp** shall resolve string identifiers to live Python component objects within registered circuits.

**Acceptance:** `opensd.project.get_comp(name)` returns pipe/node/bc instance when found.

**Implementation:** [opensd/project.py](../opensd/project.py).

---

### SOL-082 — Input validation (Should)

Python APIs shall validate types and ranges for critical settings (e.g. positive iteration counts) before export/run.

**Acceptance:** `opensd.checkvalue` used in settings setters.

**Implementation:** [opensd/checkvalue.py](../opensd/checkvalue.py), [opensd/settings.py](../opensd/settings.py).

---

## 10. Non-goals and known limits

### SOL-090 — Not a 3D CFD code (Must)

OpenSD shall **not** be required to solve general 3D Navier–Stokes; scope remains network/system dynamics.

---

### SOL-091 — GUI is separate (Must)

Interactive graph editing is **not** part of the solver executable; the web GUI is specified in `GUI-###` requirements.

**Implementation:** [reactflow/opensd-web/requirements/opensd-web-gui.md](../reactflow/opensd-web/requirements/opensd-web-gui.md).

---

## Appendix A — Traceability (solver)

| ID | Summary | Primary implementation |
|----|---------|------------------------|
| SOL-001 | 1D thermal–fluid networks | `opensd/`, `src/` |
| SOL-002 | Python + C++ split | `executor.py`, `src/` |
| SOL-010 | Mass/momentum/energy | theory docs, `pipe.cpp` |
| SOL-011 | Homogeneous two-phase | `circuit.py` |
| SOL-013 | CoolProp / user fluids | `circuit.py` |
| SOL-020 | Heat slabs | `hslab.py`, `ht_solver.cpp` |
| SOL-030 | Circuits | `circuit.py`, `circuit.cpp` |
| SOL-031 | Nodes | `node.py`, `node.cpp` |
| SOL-032 | Pipes | `pipe.py`, `pipe.cpp` |
| SOL-033 | BCs | `bc.py`, `bc.cpp` |
| SOL-040 | Geometry XML | `geometry.py` |
| SOL-041 | Settings XML | `settings.py`, `settings.h` |
| SOL-042 | Run modes | `settings.py` |
| SOL-050 | Convergence controls | `settings.py`, `convergence.cpp` |
| SOL-060 | Run executable | `executor.py` |
| SOL-070 | HDF5 output | `circuit.cpp`, `hdf5_interface.h` |
| SOL-080 | Python exports | `__init__.py` |
