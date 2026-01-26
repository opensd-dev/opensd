# Closed loop with pump problem

import opensd
import math

# Define sodium
Na6 = opensd.Fluid(name="Na6")
Na6.rhomass = 860.0
Na6.molarmass = 23E-3
Na6.viscosity = 3.75E-4
Na6.cpmass = 1267.0
Na6.cvmass = 1266.9
Na6.conductivity = 70.0
Na6.adiabatic_compressibility = 1.86E-10
Na6.isothermal_compressibility = 1.86E-10
Na6.boiling_point = 883.0 + 273.0
Na6.enthalpy_vaporization = 2.23E6

# Fluids collection
fluids = opensd.Fluids()
fluids.append(Na6)

# Export all fluids to a single XML
fluids.export_to_xml()

#primary circuit (IHX shell)
circuit1 = opensd.Circuit(identifier="circuit1")
circuit1.assign_fluid(flname="Na6",fltype="incompressible",fllib="User") #fluid name as per CoolProp

node1 = circuit1.add_node("node1")
node2 = circuit1.add_node("node2")

pipe1 = circuit1.add_pipe("pipe1",0.0174,2.5,"node1","node2",'DW',30.,5)

bc1 = circuit1.add_BC("bc1","node1",'P',5.E5)
bc2 = circuit1.add_BC("bc2","node1",'T',673.)

# bc3 = circuit1.add_BC("bc3","node2",'msource',-1.6)
# bc3 = circuit1.add_BC("bc3","node2",'P',367231.4161161)

# def fun1(time,delt):
#     if time <= 5:
#         y = 100*(10.-time)/10.
#     else:
#         y = 50.
#     pump1.Nop = y
#
# action_setup.Action(None,None,fun1)

pump1 = circuit1.add_pump("pump1","node2","node1",[[100.,"speed1.csv"]],100.)

geometry = opensd.Geometry([circuit1])
geometry.export_to_xml()

# conditions = opensd.Conditions([bc1,bc2,bc3,bc4,bc5,bc6])
# conditions.export_to_xml('conditions.xml')
#
# initial_guess = opensd.InitialGuess(geometry,conditions)
# initial_guess.export_to_xml('initial_guess.xml')

settings = opensd.Settings()
settings.verbosity = 6
settings.temp_solve = True
settings.run_mode = "steady"
settings.no_main_iter = 200
settings.conv_crit_flow = 1.E-7
settings.export_to_xml()

opensd.run(mpi_args=['mpiexec', '-n', '1'],opensd_exec='/home/vikram/Codes/opensd/build/opensd',threads=1)

