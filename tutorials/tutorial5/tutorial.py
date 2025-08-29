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

# def fun1(t,dt):
#     # y = 700.*1000.
#     import math
#     y = (650. + 50.*math.exp(-0.004*t))*1000.
#     # print t
#     return y
# action_setup.Action("bc1","bval",fun1)

# val1 = post.Monitor("node1","msource")

# from PINET import scheduler
# scheduler.etime = 1800.
# scheduler.delt = 10.

settings = opensd.Settings()
settings.verbosity = 3
settings.temp_solve = False
settings.run_mode = "steady"
settings.export_to_xml()

opensd.run(mpi_args=['mpiexec', '-n', '1'],opensd_exec='/home/vikram/Codes/opensd/build/opensd')

