# Optimiser base problem

import json
import math

import numpy as np
from scipy import optimize

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


def total_cost(diameter):
    pump_rate = 4000.0
    pipe_rate = 0.1667
    tarif = 0.4
    duration = 60000.0

    rhomass = 996.9835
    viscosity = 7.975e-4
    mflow = 20.0
    length = 300.0
    roughness = 30.0e-5
    vflow = mflow / rhomass
    area = math.pi * diameter**2 / 4.0
    velocity = vflow / area
    reynolds = rhomass * velocity * diameter / viscosity
    fricfact = 0.25 / math.log10(
        roughness / (3.7 * diameter) + 5.74 / reynolds**0.9
    )**2
    dp = fricfact * length * rhomass * velocity**2 / (2.0 * diameter)
    pump_power = vflow * dp
    pump_cost = pump_power * pump_rate / 100.0
    pump_running_cost = pump_cost + tarif * duration
    pipe_cost = pipe_rate * length * diameter * 1000.0
    return pump_cost + pump_running_cost + pipe_cost


root = optimize.minimize(
    lambda x: total_cost(float(np.atleast_1d(x)[0])),
    np.array([0.1]),
    bounds=[(float(min(design["parameters"]["pipe1.diameter"])), float(max(design["parameters"]["pipe1.diameter"])))],
)

with open("design_result.json", "w", encoding="utf-8") as f:
    json.dump(
        {
            "mode": "optimize",
            "parameters": {
                "pipe1": {
                    "diameter": root.x.tolist(),
                },
            },
            "objective": float(root.fun),
            "success": bool(root.success),
        },
        f,
        indent=2,
    )
