# SFR IHX

import opensd

#primary circuit (IHX shell)
circuit1 = opensd.Circuit(identifier="circuit1")
circuit1.assign_fluid(flname="LiqNa",fltype="incompressible") #fluid name as per CoolProp
node1 = circuit1.add_node("node1")
node2 = circuit1.add_node("node2")

import math

Af = 0.25*math.pi*1.831**2 - 3600*0.25*math.pi*0.019**2
Pw = math.pi*(1.831+3600*0.019)
dh = 4.*Af/(Pw)

pipe1 = circuit1.add_pipe("pipe1",dh,7.5,"node1","node2",'DW',30.,20,cfarea=Af) #npar=1

circuit1.add_BC("bc1","node1",'P',5.E5)
circuit1.add_BC("bc2","node1",'T',817.)
circuit1.add_BC("bc3","node2",'msource',-1644.)

geometry = opensd.Geometry([circuit1])
geometry.export_to_xml()

settings = opensd.Settings()
settings.verbosity = 3
settings.temp_solve = False
settings.run_mode = "steady"
settings.export_to_xml()

opensd.run(mpi_args=['mpiexec', '-n', '1'],opensd_exec='/mnt/c/codes/opensd/build/opensd')

