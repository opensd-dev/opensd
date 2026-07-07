# Closed loop with pump problem

import opensd

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

bc1 = circuit1.add_BC("bc1","node1",'P',5.E5,trans=False)
bc2 = circuit1.add_BC("bc2","node1",'T',673.,trans=False)

pump1 = circuit1.add_pump(
    "pump1",
    "node2",
    "node1",
    [[50., "speed2.csv"], [100., "speed1.csv"]],
    100.,
)

geometry = opensd.Geometry([circuit1])
geometry.export_to_xml()

conditions = opensd.Conditions([bc1, bc2])
conditions.export_to_xml()

settings = opensd.Settings()
settings.temp_solve = False
settings.relax_pres = 0.2
settings.run_mode = "steady"
settings.conv_crit_flow = 1.0e-8
settings.export_to_xml()

opensd.run(mpi_args=['mpiexec', '-n', '1'],opensd_exec='/mnt/c/codes/opensd/build/opensd',threads=1)

pump_speed = opensd.Tabular([0.0, 5.0, 20.0], [100.0, 50.0, 50.0])
actions = opensd.Actions([
    opensd.Action("pump1_speed", "pump1", "Nop", pump_speed),
])
actions.export_to_xml()
settings.run_mode = "transient"
settings.tim_slot = [[0.1, 20.0]]
settings.export_to_xml()

opensd.run(mpi_args=['mpiexec', '-n', '1'],opensd_exec='/mnt/c/codes/opensd/build/opensd',threads=1)

