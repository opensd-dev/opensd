# FFTF SA problem

import opensd

# Define sodium (shell side)
Na13 = opensd.Fluid(name="Na6")
Na13.rhomass = 860.0
Na13.molarmass = 23E-3
Na13.viscosity = 3.75E-4
Na13.cpmass = 1267.0
Na13.cvmass = 1266.9
Na13.conductivity = 70.0
Na13.adiabatic_compressibility = 1.86E-10
Na13.isothermal_compressibility = 1.86E-10
Na13.boiling_point = 883.0 + 273.0
Na13.enthalpy_vaporization = 2.23E6

# Fluids collection
fluids = opensd.Fluids()
fluids.append(Na13)

# Export all fluids to a single XML
fluids.export_to_xml()

#primary circuit (IHX shell)
circuit1 = opensd.Circuit(identifier="circuit1")
circuit1.assign_fluid(flname="Na13",fltype="incompressible",fllib="User") #fluid name as per CoolProp

node1 = circuit1.add_node("node1")
node2 = circuit1.add_node("node2")
# global node2

pipe1 = circuit1.add_pipe("pipe1",length=0.9144,diameter=0.003238,unode="node1",dnode="node2",fricopt=0.02,roughness=0.,ncell=10,heat_input=0.0,cfarea=0.00433)

bc1 = circuit1.add_BC("bc1","node1",'P',5.E5)
bc2 = circuit1.add_BC("bc2","node1",'T',598.899)
bc3 = circuit1.add_BC("bc3","node2",'msource',-25.3928)

# action_setup.Action("bc3","bval",fun1)

SS13 = opensd.Solid(name="SS13")
SS13.rhomass = 7600.0
SS13.cpmass = 540.0
SS13.conductivity = 20.0

gap13 = opensd.Solid(name="gap13")
gap13.rhomass = 1.0
gap13.cpmass = 0.0
gap13.conductivity = 1.12

MOX13 = opensd.Solid(name="MOX13")
MOX13.rhomass = 10600.0
MOX13.cpmass = 406.0
MOX13.conductivity = 3.0

# Solids collection
solids = opensd.Solids()
solids.append(SS13)
solids += [gap13]
solids += [MOX13]

# Export all solids to a single XML
solids.export_to_xml()

snode1 = opensd.SNode("snode1")

hslab1 = opensd.HSlab("hslab1",ucomp="pipe1",uvar="pipe",uval=[script1],dcomp="snode1",dvar="hflux",dval=0.0,uarea=3.642,nlayers=3)
hslab1.add_layer(thk_elem=3.81E-4,thk_cros=0.9144,nnodes=2,darea=3.167,solname='SS13',sollib="User")
hslab1.add_layer(thk_elem=7.5E-5,thk_cros=0.9144,nnodes=2,darea=3.079,solname='gap13',sollib="User",heat_input=0.)
hslab1.add_layer(thk_elem=0.00247,thk_cros=0.9144,nnodes=2,darea=3.079,solname='MOX13',sollib="User",heat_input=3174806.,AFF=[0.0740, 0.0937, 0.1107, 0.1222, 0.1271, 0.1248,0.1156, 0.0999, 0.0786, 0.0534])

geometry = opensd.Geometry([circuit1,hslab1])
geometry.export_to_xml()

conditions = opensd.Conditions([bc1,bc2,bc3,bc4,bc5,bc6])
conditions.export_to_xml('conditions.xml')

initial_guess = opensd.InitialGuess(geometry,conditions)
initial_guess.export_to_xml('initial_guess.xml')

settings = opensd.Settings()
settings.verbosity = 6
settings.temp_solve = True
settings.run_mode = "steady"
settings.no_main_iter = 200
settings.export_to_xml()

opensd.run(mpi_args=['mpiexec', '-n', '1'],opensd_exec='/mnt/c/codes/opensd/build/opensd',threads=1)

