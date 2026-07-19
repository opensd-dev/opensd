# Greyvenstein (2002) problem 4

import opensd

#circuit inputs
circuit1 = opensd.Circuit(identifier="circuit1")
circuit1.assign_fluid("He")

#node inputs
node1 = circuit1.add_node("node1")
node2 = circuit1.add_node("node2")

#pipe inputs
pipe1 = circuit1.add_pipe("pipe1",0.1,10.,"node1","node2",0.02,0.,5)

#boundary conditions

bc1 = circuit1.add_BC("bc1","node1",'P',700000.)
bc2 = circuit1.add_BC("bc2","node1",'T',300.)
bc3 = circuit1.add_BC("bc3","node2",'P',650000.)
# bc3 = circuit1.add_BC("bc3","node2",'msource',-1.75)

geometry = opensd.Geometry([circuit1])
geometry.export_to_xml()

settings = opensd.Settings()
settings.verbosity = 3
settings.temp_solve = False
settings.run_mode = "steady"
settings.export_to_xml()

opensd.run(mpi_args=['mpiexec', '-n', '1'],opensd_exec='/mnt/c/codes/opensd/build/opensd')

a1 = opensd.Action("ramp_bc1", "bc1", "bval", "fun1")

actions = opensd.Actions([a1])
actions.export_to_xml()

# settings.no_main_iter = 500
settings.verbosity = 1
settings.temp_solve = False
settings.run_mode = "transient"
settings.tim_slot = [[10.0, 1800.]]
settings.flag_write = True
settings.export_to_xml()

opensd.run(mpi_args=['mpiexec', '-n', '1'],opensd_exec='/mnt/c/codes/opensd/build/opensd')
