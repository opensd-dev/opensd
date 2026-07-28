# CEP trip with feedwater heater coupling

from pathlib import Path
import math
import shutil

import opensd


SCRIPT_DIR = Path(__file__).resolve().parent
scripts_source = SCRIPT_DIR / "scripts.py"
scripts_target = Path.cwd() / "scripts.py"
if scripts_source.exists() and scripts_source.resolve() != scripts_target.resolve():
    shutil.copyfile(scripts_source, scripts_target)

bindings_source = SCRIPT_DIR.parents[1] / "build" / "bindings.so"
bindings_target = Path.cwd() / "bindings.so"
if bindings_source.exists() and bindings_source.resolve() != bindings_target.resolve():
    shutil.copyfile(bindings_source, bindings_target)

circuit1 = opensd.Circuit(identifier="circuit1")
circuit1.assign_fluid("Water", fltype="two_phase")

circuit1.add_node("node2")
circuit1.add_node("node3")

bc1 = circuit1.add_BC("BC1", "node2", "P", 20.0e5)
bc2 = circuit1.add_BC("BC2", "node2", "T", 338.15)
bc3 = circuit1.add_BC("BC3", "node3", "msource", -412.0)

circuit2 = opensd.Circuit(identifier="circuit2")
circuit2.assign_fluid("Water", fltype="two_phase")

circuit2.add_node("node10")
lph3shell = circuit2.add_tptank("LPH3shell", geom=("vertcyl", 3.6, 29.88))
circuit2.add_node("node12")

bc16 = circuit2.add_BC("BC16", "node10", "msource", 22.5828)
bc17 = circuit2.add_BC("BC17", "node10", "H", 2054000.0)

length = 25.9
inner_diameter = 17.05e-3
outer_diameter = 19.05e-3
tube_count = 917

lph2tube = circuit1.add_pipe(
    "LPH2tube",
    inner_diameter,
    length,
    "node2",
    "node3",
    0.001,
    30.0e-5,
    5,
    npar=tube_count,
    qcrit=1.0e8,
    Kforward=0.0,
    ufrac=0.0,
    dfrac=0.0,
)
lph2shell = circuit2.add_tptank("LPH2shell", geom=("vertcyl", 3.6, 29.88), heat_input=0.0)
circuit2.add_pipe(
    "LPH2extract",
    1.0,
    20.0,
    "node10",
    "LPH2shell",
    0.001,
    30.0e-6,
    1,
    dfrac=1.0,
)
circuit2.add_pipe(
    "LPH2drainout",
    1.0,
    20.0,
    "LPH2shell",
    "node12",
    0.001,
    30.0e-6,
    1,
    ufrac=0.0,
    dfrac=0.0,
    Kforward=1000.0,
)

Au = math.pi * outer_diameter * length * tube_count
Ad = math.pi * inner_diameter * length * tube_count
hslab = opensd.HSlab(
    "LPH2hslab",
    ucomp="LPH2shell",
    uvar="node",
    uval=[20000.0],
    dcomp="LPH2tube",
    dvar="pipe",
    dval=[10000.0],
    uarea=Au,
    config="parallel",
    nlayers=1,
)
hslab.add_layer(
    thk_elem=(outer_diameter - inner_diameter) / 2.0,
    thk_cros=length,
    nnodes=3,
    darea=Ad,
    solname="SS6",
    sollib="User",
)

circuit2.add_pipe(
    "LPH3drainout",
    1.0,
    20.0,
    "LPH3shell",
    "LPH2shell",
    0.001,
    30.0e-6,
    1,
    ufrac=0.0,
    dfrac=0.0,
    Kforward=389104.51453613,
)

bc10 = circuit2.add_BC("BC10", "LPH3shell", "P", 1.57e5)
bc11 = circuit2.add_BC("BC11", "LPH3shell", "H", 484058.0)
bc12 = circuit2.add_BC("BC12", "LPH2shell", "P", 66583.81764724, trans=False)
bc13 = circuit2.add_BC("BC13", "LPH2shell", "H", 388165.0, trans=False)
bc18 = circuit2.add_BC("BC18", "node12", "P", 80143.2800554)

SS6 = opensd.Solid(name="SS6")
SS6.rhomass = 7600.0
SS6.cpmass = 540.0
SS6.conductivity = 20.0
opensd.Solids([SS6]).export_to_xml()

geometry = opensd.Geometry([circuit1, circuit2, hslab])
geometry.export_to_xml()

conditions = opensd.Conditions([bc1, bc2, bc3, bc16, bc17, bc10, bc11, bc12, bc13, bc18])
conditions.export_to_xml()

post = opensd.Post([
    opensd.Calculate("lph2shell_msource", "LPH2shell", identifier="lph2shell_msource"),
    opensd.Calculate("lph2shell_esource", "LPH2shell", identifier="lph2shell_esource"),
    opensd.Calculate("node10_pressure", "node10", identifier="node10_pressure"),
    opensd.Calculate("lph2shell_pressure", "LPH2shell", identifier="lph2shell_pressure"),
    opensd.Calculate("lph2shell_enthalpy", "LPH2shell", identifier="lph2shell_enthalpy"),
    opensd.Calculate("node12_msource", "node12", identifier="node12_msource"),
    opensd.Calculate("lph3shell_msource", "LPH3shell", identifier="lph3shell_msource"),
])
post.export_to_xml()

settings = opensd.Settings()
settings.verbosity = 0
settings.temp_solve = True
settings.no_flow_iter = 1000
settings.conv_crit_temp_trans = 1.0e-5
settings.conv_crit_ht = 1.0e-5
settings.run_mode = "steady"
settings.tim_slot = [[0.0, 0.0]]
settings.export_to_xml()

opensd.run(mpi_args=["mpiexec", "-n", "1"], opensd_exec="/mnt/c/codes/opensd/build/opensd")

bc12.enabled = False
bc13.enabled = False
for fixed_var in ("P", "H"):
    if fixed_var in lph2shell.fixed_var:
        lph2shell.fixed_var.remove(fixed_var)
geometry.export_to_xml()
conditions.export_to_xml()

actions = opensd.Actions([
    opensd.Action("BC3_msource", "BC3", "bval", "cep_trip_msource"),
])
actions.export_to_xml()

settings.run_mode = "transient"
settings.tim_slot = [[1.0, 10.0], [10.0, 100.0], [1.0, 110.0]]
settings.export_to_xml()

opensd.run(mpi_args=["mpiexec", "-n", "1"], opensd_exec="/mnt/c/codes/opensd/build/opensd")
