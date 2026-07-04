# Two phase tank energy source transient

import opensd

circuit1 = opensd.Circuit(identifier="circuit1")
circuit1.assign_fluid("Water", fltype="two_phase")

circuit1.add_node("node9")
circuit1.add_node("node12")
tank = circuit1.add_tptank(
    "LPH3shell",
    geom=("vertcyl", 3.6, 29.88),
    heat_input=-39776208.0996519,
)

circuit1.add_pipe(
    "LPH3extract",
    1.,
    20.,
    "node9",
    "LPH3shell",
    0.001,
    30.E-6,
    1,
    dfrac=1.,
    Kforward=10.,
)
circuit1.add_pipe(
    "LPH3drainout",
    1.,
    20.,
    "LPH3shell",
    "node12",
    0.001,
    30.E-6,
    1,
    ufrac=0.,
    Kforward=10.,
)

bc1 = circuit1.add_BC("BC8", "node9", "msource", 19.058)
bc2 = circuit1.add_BC("BC9", "node9", "H", 2556000.)
bc3 = circuit1.add_BC("BC10", "LPH3shell", "P", 152089.11814064477, trans=False)
bc4 = circuit1.add_BC("BC11", "LPH3shell", "H", 484058., trans=False)
bc5 = circuit1.add_BC("BC18", "node12", "msource", -19.058)

geometry = opensd.Geometry([circuit1])
geometry.export_to_xml()

conditions = opensd.Conditions([bc1, bc2, bc3, bc4, bc5])
conditions.export_to_xml()

heat_profile = opensd.Tabular(
    [0., 300., 1000.],
    [-39776208.0996519, -39776208.0996519, -39776208.0996519*0.8],
)
actions = opensd.Actions([
    opensd.Action("LPH3shell_heat_input", tank, "heat_input", heat_profile),
])
actions.export_to_xml()

post = opensd.Post([
    opensd.Calculate("tank_pressure", "LPH3shell", identifier="tank_pressure"),
    opensd.Calculate("tank_enthalpy", "LPH3shell", identifier="tank_enthalpy"),
])
post.export_to_xml()

settings = opensd.Settings()
settings.verbosity = 3
settings.temp_solve = True
settings.run_mode = "transient"
settings.tim_slot = [[1., 1000.]]
settings.conv_crit_temp_trans = 1.E-7
settings.export_to_xml()

opensd.run(mpi_args=["mpiexec", "-n", "1"], opensd_exec="/mnt/c/codes/opensd/build/opensd")
