# simple NC problem HHHC configuration for validation

import math

import opensd

# circuit inputs
circuit1 = opensd.Circuit(identifier="circuit1")
circuit1.assign_fluid("Water")

circuit2 = opensd.Circuit(identifier="circuit2")
circuit2.assign_fluid("Water")

# node inputs
circuit1.add_node("node1", elevation=0.)
circuit1.add_node("node2", elevation=2.2, ttemp_old=325.)
circuit1.add_node("node3", elevation=2.2)
circuit1.add_node("node4", elevation=0.)

circuit2.add_node("node5")
circuit2.add_node("node6")

# pipe inputs
circuit1.add_pipe(identifier="pipe1", diameter=0.026, length=2.2, unode="node1", dnode="node2", fricopt=0.05, roughness=30., ncell=5)
circuit1.add_pipe("pipe2", 0.026, 1.415, "node2", "node3", 0.05, 30., 5)
circuit1.add_pipe("pipe3", 0.026, 2.2, "node3", "node4", 0.05, 30., 5)
circuit1.add_pipe("pipe4", 0.026, 1.415, "node4", "node1", 0.05, 30., 5, heat_input=1000.)

circuit2.add_pipe(identifier="pipe5", diameter=0.004, length=1.415, unode="node5", dnode="node6", fricopt='DW', roughness=30.E-5, ncell=5, cfarea=0.000188)

# boundary conditions
bc1 = circuit2.add_BC("bc1", "node5", 'P', 2.E5)
bc2 = circuit2.add_BC("bc2", "node5", 'T', 283.)
bc3 = circuit2.add_BC("bc3", "node6", 'msource', -0.1666667)
bc4 = circuit1.add_BC("bc4", "node1", 'P', 70.E5, trans=False)

# heat slab material
glass = opensd.Solid(name="glass")
glass.rhomass = 2500.0
glass.cpmass = 840.0
glass.conductivity = 1.05
solids = opensd.Solids([glass])
solids.export_to_xml()

settings = opensd.Settings()
settings.T_ambient = 283.

# heat slabs
Au = math.pi*0.026*0.8
Ad = math.pi*0.028*0.8
hslab1 = opensd.HSlab("hslab1", ucomp="pipe2", uvar="pipe", uval=[1000.], dcomp="pipe5", dvar="pipe", dval=[1000.], uarea=Au, config="counter", solveSS=True)
hslab1.add_layer(thk_elem=0.002, thk_cros=1.415, nnodes=3, darea=Ad, solname='glass', sollib='User')

geometry = opensd.Geometry([circuit1, circuit2, hslab1])
geometry.export_to_xml()

conditions = opensd.Conditions([bc1, bc2, bc3, bc4])
conditions.export_to_xml()

settings.temp_solve = True
settings.run_mode = "steady"
settings.conv_crit_flow = 1.E-8
settings.conv_crit_temp_SS = 1.E-8
settings.conv_crit_temp_trans = 1.E-6
settings.tim_slot = [[0.0, 0.0]]
settings.export_to_xml()

opensd.run(mpi_args=['mpiexec', '-n', '1'], opensd_exec='/mnt/c/codes/opensd/build/opensd')

settings.run_mode = "transient"
settings.tim_slot = [[4.0, 10000.0]]

heat_input = opensd.Tabular([0.0, 4.0, 10000.0], [1000.0, 0.0, 0.0])
actions = opensd.Actions([
    opensd.Action("pipe4_heat_input", "pipe4", "heat_input", heat_input),
])
actions.export_to_xml()
settings.export_to_xml()

opensd.run(mpi_args=['mpiexec', '-n', '1'], opensd_exec='/mnt/c/codes/opensd/build/opensd')
