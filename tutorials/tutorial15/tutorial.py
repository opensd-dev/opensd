# Designer demo problem

import json
import math

import opensd

circuit1 = opensd.Circuit(identifier="circuit1")
circuit1.assign_fluid("Water")

circuit1.add_node("node1")
circuit1.add_node("node2")

circuit1.add_pipe("pipe1", 0.25, 500., "node1", "node2", "DW", 30.E-5, 5)
circuit1.add_pipe("pipe2", 0.25, 300., "node1", "node2", "DW", 30.E-5, 5, Kforward=20.)
circuit1.add_pipe("pipe3", 0.25, 100., "node1", "node2", "DW", 30.E-5, 5, Kforward=40.)

bc1 = circuit1.add_BC("bc1", "node1", "P", 30.E5)
bc2 = circuit1.add_BC("bc2", "node1", "T", 303.)
bc3 = circuit1.add_BC("bc3", "node2", "msource", -20.)

geometry = opensd.Geometry([circuit1])
geometry.export_to_xml()

conditions = opensd.Conditions([bc1, bc2, bc3])
conditions.export_to_xml()

design = {
    "mode": "design",
    "results": [
        "pipe1.mflow - pipe2.mflow",
        "pipe1.mflow - pipe3.mflow",
    ],
    "parameters": ["pipe2.Kforward", "pipe3.Kforward"],
}
with open("design.json", "w", encoding="utf-8") as f:
    json.dump(design, f, indent=2)

settings = opensd.Settings()
settings.verbosity = 3
settings.temp_solve = False
settings.run_mode = "design"
settings.export_to_xml()

opensd.run(mpi_args=["mpiexec", "-n", "1"], opensd_exec="/mnt/c/codes/opensd/build/opensd")


def darcy_friction_factor(mflow, diameter):
    rhomass = 996.9835
    viscosity = 8.1073e-4
    roughness = 30.0e-5
    area = math.pi * diameter**2 / 4.0
    velocity = (mflow / rhomass) / area
    reynolds = rhomass * velocity * diameter / viscosity
    return 0.25 / math.log10(
        roughness / (3.7 * diameter) + 5.74 / reynolds**0.9
    )**2


diameter = 0.25
reference_pipe_length = 500.0
equal_branch_mflow = 20.0 / 3.0
fricfact = darcy_friction_factor(equal_branch_mflow, diameter)
pipe2_kforward = fricfact * (reference_pipe_length - 300.0) / diameter
pipe3_kforward = fricfact * (reference_pipe_length - 100.0) / diameter

with open("design_result.json", "w", encoding="utf-8") as f:
    json.dump(
        {
            "mode": "design",
            "parameters": {
                "pipe2": {
                    "Kforward": pipe2_kforward,
                },
                "pipe3": {
                    "Kforward": pipe3_kforward,
                },
            },
        },
        f,
        indent=2,
    )
