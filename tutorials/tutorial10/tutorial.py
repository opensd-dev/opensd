# simple NC problem U section (open loop) for validation

import opensd

#circuit inputs
circuit1 = opensd.Circuit(identifier="circuit1")
circuit1.assign_fluid("Water")

#node inputs
node1 = circuit1.add_node("node1",elevation=0.2)
node2 = circuit1.add_node("node2",elevation=0.)
node3 = circuit1.add_node("node3",elevation=0.,ttemp_old=289.)
node4 = circuit1.add_node("node4",elevation=0.2,ttemp_old=289.)

#pipe inputs
pipe1=circuit1.add_pipe(identifier="pipe1",diameter=0.1,length=0.2,unode="node1",dnode="node2",fricopt=0.05,roughness=30.E-6,ncell=10)
pipe2=circuit1.add_pipe(identifier="pipe2",diameter=0.1,length=0.2,unode="node2",dnode="node3",fricopt=0.05,roughness=30.E-6,ncell=10,heat_input=2.E4)
pipe3=circuit1.add_pipe(identifier="pipe3",diameter=0.1,length=0.2,unode="node3",dnode="node4",fricopt=0.05,roughness=30.E-6,ncell=10)

#boundary conditions
circuit1.add_BC("bc1","node1",'P',1.E5)
circuit1.add_BC("bc2","node1",'T',288.)
circuit1.add_BC("bc3","node4",'P',1.E5)
# bc4 = comp.BC("bc4","node4",'T',289.)

geometry = opensd.Geometry([circuit1])
geometry.export_to_xml()

settings = opensd.Settings()
settings.verbosity = 3
settings.temp_solve = True
settings.run_mode = "steady"
settings.conv_crit_flow = 1.E-7
settings.conv_crit_temp_SS = 1.E-7
settings.export_to_xml()

opensd.run(mpi_args=['mpiexec', '-n', '1'],opensd_exec='/mnt/c/codes/opensd/build/opensd')
