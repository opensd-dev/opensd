# Dementev blowdown experiment

from pathlib import Path
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

tank = circuit1.add_tptank("node1", elevation=0., geom=("vertcyl", 0.309, 2.13))
circuit1.add_node("node2", elevation=0.)

circuit1.add_orifice("orifice1", "node1", "node2", 0.025, 1., opening=1., ufrac=0.8)

bc1 = circuit1.add_BC("bc1", "node1", "P", 123.E5, trans=False)
bc2 = circuit1.add_BC("bc2", "node1", "T", 535., trans=False)
bc3 = circuit1.add_BC("bc3", "node2", "P", 1.E5)

geometry = opensd.Geometry([circuit1])
geometry.export_to_xml()

conditions = opensd.Conditions([bc1, bc2, bc3])
conditions.export_to_xml()

post = opensd.Post([
    opensd.Calculate("tank_pressure", tank.identifier, identifier="tank_pressure"),
])
post.export_to_xml()

settings = opensd.Settings()
settings.verbosity = 1
settings.temp_solve = True
settings.conv_crit_flow = 1.E-4
settings.conv_crit_temp_trans = 1.E-7
settings.conv_crit_temp_SS = 1.E-7
settings.no_flow_iter = 10000
settings.run_mode = "steady"
settings.tim_slot = [[0.0, 0.0]]
settings.export_to_xml()

opensd.run(mpi_args=["mpiexec", "-n", "1"], opensd_exec="/mnt/c/codes/opensd/build/opensd")

settings.run_mode = "transient"
settings.tim_slot = [[0.0001, 0.1], [0.001, 3.0], [0.01, 10.0]]
settings.export_to_xml()

opensd.run(mpi_args=["mpiexec", "-n", "1"], opensd_exec="/mnt/c/codes/opensd/build/opensd")
