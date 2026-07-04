# Two phase natural circulation problem

import opensd

circuit1 = opensd.Circuit(identifier="circuit1")
circuit1.assign_fluid("Water", fltype="two_phase")

circuit1.add_node("node1", elevation=2.445)
circuit1.add_node("node2", elevation=0.)
circuit1.add_node("node3", elevation=0.)
circuit1.add_node("node4", elevation=0.575)
circuit1.add_node("node5", elevation=2.21)
circuit1.add_node("node6", elevation=2.445, ttemp_old=555.)

circuit1.add_pipe("pipe1", 0.0199, 2.445, "node1", "node2", "BL", 30.E-6, 10)
circuit1.add_pipe("pipe2", 0.0199, 2.02, "node2", "node3", "BL", 30.E-6, 10)
circuit1.add_pipe("pipe3", 0.0199, 0.575, "node3", "node4", "BL", 30.E-6, 10, heat_input=25000.)
circuit1.add_pipe("pipe4", 0.0199, 1.635, "node4", "node5", "BL", 30.E-6, 10)
circuit1.add_pipe("pipe5", 0.0199, 2.036, "node5", "node6", "BL", 30.E-6, 10)

bc1 = circuit1.add_BC("bc1", "node1", "P", 70.E5)
bc2 = circuit1.add_BC("bc2", "node1", "T", 549.)
bc3 = circuit1.add_BC("bc3", "node6", "P", 70.E5)

geometry = opensd.Geometry([circuit1])
geometry.export_to_xml()

conditions = opensd.Conditions([bc1, bc2, bc3])
conditions.export_to_xml()

settings = opensd.Settings()
settings.verbosity = 3
settings.temp_solve = True
settings.run_mode = "steady"
settings.conv_crit_temp_SS = 1.E-7
settings.conv_crit_flow = 1.E-7
settings.export_to_xml()

opensd.run(mpi_args=["mpiexec", "-n", "1"], opensd_exec="/mnt/c/codes/opensd/build/opensd")
