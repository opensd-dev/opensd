# Greyvenstein (2002) problem 3 Simple He flow network transient

import opensd

#circuit inputs
circuit1 = opensd.Circuit(identifier="circuit1")
circuit1.assign_fluid("He")

#node inputs
node1 = circuit1.add_node("node1")
node2 = circuit1.add_node("node2")
node3 = circuit1.add_node("node3")
node4 = circuit1.add_node("node4")
node5 = circuit1.add_node("node5")
node6 = circuit1.add_node("node6")

#pipe inputs
pipe1=circuit1.add_pipe("pipe1",0.5,10.,"node1","node5",0.02,0.,10)
pipe2=circuit1.add_pipe("pipe2",0.5,10.,"node5","node6",0.02,0.,10)
pipe3=circuit1.add_pipe("pipe3",0.5,10.,"node6","node2",0.02,0.,10)
pipe4=circuit1.add_pipe("pipe4",0.5,10.,"node5","node3",0.02,0.,10)
pipe5=circuit1.add_pipe("pipe5",0.5,10.,"node6","node4",0.02,0.,10)

#boundary conditions
circuit1.add_BC("bc1","node1",'P',7.E5)
circuit1.add_BC("bc2","node1",'T',300.)
circuit1.add_BC("bc3","node2",'msource', -11.61)
circuit1.add_BC("bc4","node3",'msource', -12.37)
circuit1.add_BC("bc5","node4",'msource', -11.61)

geometry = opensd.Geometry([circuit1])
geometry.export_to_xml()

# def fun1(time,delt):
    # if time > 0.:
        # y = 0.
    # else:
        # y = -12.37
    # return y
# action_setup.Action("bc4","bval",fun1)
# def fun2(time,delt):
    # if time > 0.:
        # y = 0.
    # else:
        # y = -11.61
    # return y
# action_setup.Action("bc5","bval",fun2)


settings = opensd.Settings()
settings.verbosity = 3
settings.temp_solve = False
settings.run_mode = "steady"
settings.export_to_xml()

opensd.run(mpi_args=['mpiexec', '-n', '1'],opensd_exec='/mnt/c/codes/opensd/build/opensd')
