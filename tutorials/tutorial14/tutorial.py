# Optimiser base problem

import json

import numpy as np

import opensd

circuit1 = opensd.Circuit(identifier="circuit1")
circuit1.assign_fluid("Water")

circuit1.add_node("node1")
circuit1.add_node("node2")

circuit1.add_pipe("pipe1", 0.1, 300., "node1", "node2", "DW", 30.E-5, 5)

bc1 = circuit1.add_BC("bc1", "node1", "P", 30.E5)
bc2 = circuit1.add_BC("bc2", "node1", "T", 303.)
bc3 = circuit1.add_BC("bc3", "node2", "msource", -20.)

geometry = opensd.Geometry([circuit1])
geometry.export_to_xml()

conditions = opensd.Conditions([bc1, bc2, bc3])
conditions.export_to_xml()

design = {
    "mode": "optimize",
    "objective": "total pumping and pipe cost",
    "parameters": {
        "pipe1.diameter": np.linspace(0.08, 0.35, num=10).tolist(),
    },
}
with open("design.json", "w", encoding="utf-8") as f:
    json.dump(design, f, indent=2)

settings = opensd.Settings()
settings.verbosity = 3
settings.temp_solve = False
settings.run_mode = "optimize"
settings.export_to_xml()

opensd.run(mpi_args=["mpiexec", "-n", "1"], opensd_exec="/mnt/c/codes/opensd/build/opensd")
