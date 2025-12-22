# SFR SG

import opensd
import math

# Define sodium (shell side)
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

#primary circuit (SG shell)
circuit1 = opensd.Circuit(identifier="circuit1")
circuit1.assign_fluid(flname="Na6",fltype="incompressible",fllib="User") #fluid name as per CoolProp

node1 = circuit1.add_node("node1")
node2 = circuit1.add_node("node2")

#geometry inputs
Do = 0.855
Di = 0.831
do = 0.0172
di = 0.0126
N = 547
L = 22.
E = 0.0322
ninc = 50

Af = math.pi*Di**2/4.-N*math.pi*do**2/4.
Pw = math.pi*Di+N*math.pi*do
dh = 4.*Af/Pw

pipe1=circuit1.add_pipe(identifier="pipe1",diameter=dh,length=L,unode="node2",dnode="node1",cfarea=Af,fricopt='DW',roughness=30E-6,ncell=ninc,heat_input=0.)

bc1 = circuit1.add_BC("bc1","node1",'P',5.E5)
# def fun1(time):
#     if time <=900.:
#         y = 798.-time/15.
#     else:
#         y = 798.-900./15.
#     # y = 798.
#     return y
bc3 = circuit1.add_BC("bc3","node1",'T',798.)
bc2 = circuit1.add_BC("bc2","node2",'msource',-730.)


#secondary circuit (IHX tube)
circuit2 = opensd.Circuit(identifier="circuit2")
circuit2.assign_fluid(flname="Water",fllib="CoolProp") #,flag_tp=True

node3 = circuit2.add_node("node3",elevation=0.)
node4 = circuit2.add_node("node4",elevation=L)

# def fun3(time):
#     if time <=10:
#         y = (156.-time)*1.E6
#     else:
#         y = (156.-10.)*1.E6
#     return y
pipe2=circuit2.add_pipe(identifier="pipe2",diameter=di,length=L,unode="node3",dnode="node4",fricopt='DW',roughness=30E-6,ncell=ninc,npar=N,heat_input=0.) #158.E6 qcrit=script3

bc4 = circuit2.add_BC("bc4","node3",'P',170.E5)
bc5 = circuit2.add_BC("bc5","node3",'T',508.)
bc6 = circuit2.add_BC("bc6","node4",'msource',-70.3)

Au = math.pi*do*L*N
Ad = math.pi*di*L*N

chromoly = opensd.Solid(name="chromoly")
chromoly.rhomass = 7600.0
chromoly.cpmass = 540.0
chromoly.conductivity = 20.0

# Solids collection
solids = opensd.Solids()
solids.append(chromoly)

# Export all solids to a single XML
solids.export_to_xml()

hslab1 = opensd.HSlab("hslab1",ucomp="pipe1",uvar="pipe",utype="script",uval="script1",dcomp="pipe2",dvar="pipe",dtype="constant",dval=10000,uarea=Au,config="parallel")
hslab1.add_layer(thk_elem=(do-di)/2.,thk_cros=L,nnodes=3,darea=Ad,solname='chromoly',sollib="User")

geometry = opensd.Geometry([circuit1,circuit2,hslab1])
geometry.export_to_xml()

# conditions = opensd.Conditions([bc1,bc2,bc3,bc4,bc5,bc6])
# conditions.export_to_xml('conditions.xml')

# initial_guess = opensd.InitialGuess(geometry,conditions)
# initial_guess.export_to_xml('initial_guess.xml')

settings = opensd.Settings()
settings.verbosity = 6
settings.temp_solve = True
settings.run_mode = "steady"
settings.no_main_iter = 200
settings.export_to_xml()

opensd.run(mpi_args=['mpiexec', '-n', '1'],opensd_exec='/home/vikram/Codes/opensd/build/opensd',threads=1)

