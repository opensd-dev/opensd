# SFR IHX

import opensd
import math

# Define sodium (shell side)
Na6 = opensd.Fluid(name="Na6")
Na6.rhomass = 860.0
Na6.molarmass = 23E-3
Na6.viscosity = 3.75E-4
Na6.cpmass = 1260.0
Na6.cvmass = 1270.0
Na6.conductivity = 70.0
Na6.adiabatic_compressibility = 1.86E-10
Na6.isothermal_compressibility = 1.86E-10
Na6.boiling_point = 883.0 + 273.0
Na6.enthalpy_vaporization = 2.23E6

# Define sodium (tube side)
Na7 = opensd.Fluid(name="Na7")
Na7.rhomass = 860.0
Na7.molarmass = 23E-3
Na7.viscosity = 3.75E-4
Na7.cpmass = 1260.0
Na7.cvmass = 1270.0
Na7.conductivity = 70.0
Na7.adiabatic_compressibility = 1.86E-10
Na7.isothermal_compressibility = 1.86E-10
Na7.boiling_point = 883.0 + 273.0
Na7.enthalpy_vaporization = 2.23E6

# Fluids collection
fluids = opensd.Fluids()
fluids.append(Na6)
fluids += [Na7]

# Export all fluids to a single XML
fluids.export_to_xml()

#primary circuit (IHX shell)
# circuit1 = opensd.Circuit(identifier="circuit1")
# circuit1.assign_fluid(flname="LiqNa",fltype="incompressible") #fluid name as per CoolProp

# node1 = circuit1.add_node("node1")
# node2 = circuit1.add_node("node2")

# Af = 0.25*math.pi*1.831**2 - 3600*0.25*math.pi*0.019**2
# Pw = math.pi*(1.831+3600*0.019)
# dh = 4.*Af/(Pw)

# pipe1 = circuit1.add_pipe("pipe1",dh,7.5,"node1","node2",'DW',30.,20,cfarea=Af) #npar=1

# circuit1.add_BC("bc1","node1",'P',5.E5)
# circuit1.add_BC("bc2","node1",'T',817.)
# circuit1.add_BC("bc3","node2",'msource',-1644.)


#secondary circuit (IHX tube)
circuit2 = opensd.Circuit(identifier="circuit2")
circuit2.assign_fluid(flname="LiqNa",fltype="incompressible",fllib="User")

node3 = circuit2.add_node("node3")
node4 = circuit2.add_node("node4")

pipe2=circuit2.add_pipe("pipe2",0.0174,7.5,"node3","node4",'DW',30.,10,npar=3600,heat_input=308.E6)

circuit2.add_BC("bc4","node3",'P',5.E5)
circuit2.add_BC("bc5","node3",'T',628.)
circuit2.add_BC("bc6","node4",'msource',-1461.)



geometry = opensd.Geometry([circuit2])
geometry.export_to_xml()

settings = opensd.Settings()
settings.verbosity = 6
settings.temp_solve = True
settings.run_mode = "steady"
settings.no_main_iter = 200
settings.export_to_xml()

opensd.run(mpi_args=['mpiexec', '-n', '1'],opensd_exec='/home/vikram/Codes/opensd/build/opensd')

