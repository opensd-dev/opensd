# Two phase fluid simulation, Collier example 2.1

import opensd

circuit1 = opensd.Circuit(identifier="circuit1")
circuit1.assign_fluid("Water", fltype="two_phase")

circuit1.add_node("node1", elevation=0.)
circuit1.add_node("node2", elevation=3.66)

pipe1 = circuit1.add_pipe(
    "pipe1",
    0.01016,
    3.66,
    "node1",
    "node2",
    "BL",
    30.E-6,
    20,
    heat_input=100000.,
)

bc1 = circuit1.add_BC("bc1", "node1", "P", 68.9E5)
bc2 = circuit1.add_BC("bc2", "node1", "T", 477.15)
bc3 = circuit1.add_BC("bc3", "node2", "msource", -0.108)

geometry = opensd.Geometry([circuit1])
geometry.export_to_xml()

conditions = opensd.Conditions([bc1, bc2, bc3])
conditions.export_to_xml()

heat_input = opensd.Tabular([0., 400.], [100000., 100000.])
actions = opensd.Actions([
    opensd.Action("pipe1_heat_input", pipe1, "heat_input", heat_input),
])
actions.export_to_xml()

settings = opensd.Settings()
settings.verbosity = 3
settings.temp_solve = True
settings.run_mode = "transient"
settings.tim_slot = [[5., 400.]]
settings.export_to_xml()

opensd.run(mpi_args=["mpiexec", "-n", "1"], opensd_exec="/mnt/c/codes/opensd/build/opensd")
