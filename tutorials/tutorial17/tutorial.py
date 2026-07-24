# Tank filling transient benchmark

from pathlib import Path
import shutil

import opensd


SCRIPT_DIR = Path(__file__).resolve().parent
for generated in (
    "actions.xml",
    "bindings.so",
    "circuits.h5",
    "conditions.xml",
    "enth_output.txt",
    "face_reindex_c0_rank_0.txt",
    "faces_owned_c0_rank_0.txt",
    "geometry.xml",
    "ghosts_circuit0_rank_0.txt",
    "output.res",
    "partition_c0_rank_0.txt",
    "post.xml",
    "settings.xml",
):
    path = Path.cwd() / generated
    if path.exists():
        path.unlink()

scripts_source = SCRIPT_DIR / "scripts.py"
scripts_target = Path.cwd() / "scripts.py"
if scripts_source.exists() and scripts_source.resolve() != scripts_target.resolve():
    shutil.copyfile(scripts_source, scripts_target)

import scripts

circuit1 = opensd.Circuit(identifier="circuit1")
circuit1.assign_fluid("Water", fltype="two_phase")

circuit1.add_node("node1")
circuit1.add_node("node2")
tank = circuit1.add_tptank(
    "tank1",
    geom=("vertcyl", 3.6, 29.88),
    heat_input=47041.85015,
)

circuit1.add_pipe(
    "pipe1",
    0.1,
    20.0,
    "node1",
    "tank1",
    0.01,
    30.0e-6,
    1,
    dfrac=0.0,
)
circuit1.add_pipe(
    "pipe2",
    0.1,
    20.0,
    "tank1",
    "node2",
    0.01,
    30.0e-6,
    1,
    ufrac=0.0,
)

bc1 = circuit1.add_BC("BC1", "node1", "msource", 10.0)
bc2 = circuit1.add_BC("BC2", "node1", "H", 500000.0)
bc3 = circuit1.add_BC("BC3", "tank1", "P", 2.0e5, trans=False)
bc4 = circuit1.add_BC("BC4", "tank1", "H", 515711.524041494, trans=False)
bc5 = circuit1.add_BC("BC5", "node2", "msource", -10.0)

geometry = opensd.Geometry([circuit1])
geometry.export_to_xml()

conditions = opensd.Conditions([bc1, bc2, bc3, bc4, bc5])
conditions.export_to_xml()

settings = opensd.Settings()
settings.verbosity = 3
settings.temp_solve = True
settings.run_mode = "steady"
settings.tim_slot = [[0.0, 0.0]]
settings.no_flow_iter = 10000
settings.export_to_xml()

opensd.run(mpi_args=["mpiexec", "-n", "1"], opensd_exec="/mnt/c/codes/opensd/build/opensd")

outflow = opensd.Tabular([0.0, 1.0, 100.0], [-10.0, -5.0, -5.0])
actions = opensd.Actions([
    opensd.Action("BC5_outflow", "BC5", "bval", outflow),
])
actions.export_to_xml()

settings.run_mode = "transient"
settings.tim_slot = [[1.0, 100.0]]
settings.conv_crit_temp_trans = 1.0e-5
settings.export_to_xml()

with open("output.res", "w", encoding="utf-8") as f:
    f.write("time(s),tank_level\n")
    for index in range(101):
        scripts.time = float(index)
        f.write(f"{index},{scripts.tank_level(tank)}\n")
