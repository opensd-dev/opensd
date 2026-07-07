#!/usr/bin/env python
# coding: utf-8

# # Tutorial 3: Pressure Transients in a Water Pipe
# This tutorial problem demonstrates the transient modeling in OpenSD code.
# ## Problem Description
# A pipe with an inner diameter of 0.8 m, a wall thickness of 0.019 m, and a length of 6000 m connected to a reservoir with water is considered. Friction pressure drop shall be calculated using the Darcy Weisbach friction factor formula with a roughness value of 2 mm. Pipe material is stainless steel (Youngs modulus = 100 GPa, Poisson’s ratio = 0.26). The initial fluid velocity is 1.5 m/s. The steady-state pressure head in the upper reservoir is 100 m. The transient pressure in the pipe when a value at the pipe end is closed needs to be estimated. Valve closure time is 20 s (linear flow reduction to be assumed). The bulk modulus of elasticity of water is 2.07 GPa, density is 1000 kg/m3, and kinematic viscosity is 1.31 x 10-6 m2/s.

# ## Steps for input file creation
# Create an input file for steady state simulation of the problem similar to that described in Tutorial 1 and 2. Since the fluid (water) properties to be used in are specified in the problem, create, and use a user defined fluid.  

# In[ ]:


# Wichowski (1991)

#circuit inputs
from pathlib import Path

import opensd

actions_path = Path("actions.xml")
if actions_path.exists():
    actions_path.unlink()

circuit1 = opensd.Circuit(identifier="circuit1")
circuit1.assign_fluid("Water")

#node inputs
node1 = circuit1.add_node("node1")
node2 = circuit1.add_node("node2")

#pipe inputs
pipe1 = circuit1.add_pipe("pipe1",0.8,6000.,"node1","node2",'DW',2.E-3,60)


# Add wall to the pipe to account the wall elasticity. Since the wall properties to be used are specified in the problem, create, and use a user defined solid for wall material.

# In[ ]:


wall1 = pipe1.add_wall(thk=0.019,solname='SS3',sollib="User",restraint="long")


# Add boundary conditions to the circuit.

# In[ ]:


#boundary conditions
bc1 = circuit1.add_BC("bc1","node1",'P',1.E6)
bc2 = circuit1.add_BC("bc2","node2",'msource',-753.6)
#bc3 = comp.BC("bc3","node1",'T',300.)

#val1 = post.Monitor("node2","tpres_gues")


# Export the geometry

# In[ ]:


geometry = opensd.Geometry([circuit1])
geometry.export_to_xml()


# Export the boundary conditions

# In[ ]:


conditions = opensd.Conditions([bc1, bc2])
conditions.export_to_xml('conditions.xml')


# Export the steady-state settings and run once before starting the transient.

# In[ ]:


settings = opensd.Settings()
settings.no_main_iter = 500
settings.verbosity = 1
settings.temp_solve = False
settings.run_mode = "steady"
settings.flag_write = True
settings.export_to_xml()


# In[ ]:


opensd.run(mpi_args=['mpiexec', '-n', '1'],opensd_exec='/mnt/c/codes/opensd/build/opensd')


# Add transient actions using the code shown below:

# In[ ]:


times  = [0.0, 20.0, 50.0]
values = [-753.6, 0.0, 0.0]
tdist = opensd.Tabular(times, values)

a1 = opensd.Action("ramp_bc2", "bc2", "bval", tdist)


# Export the transient actions

# In[ ]:


actions = opensd.Actions([a1])
actions.export_to_xml()


# Export the transient settings

# In[ ]:


settings.no_main_iter = 500
settings.verbosity = 1
settings.temp_solve = False
settings.run_mode = "transient"
settings.tim_slot = [[0.005, 95.0]]
settings.flag_write = True
settings.export_to_xml()


# Run the code

# In[ ]:


opensd.run(mpi_args=['mpiexec', '-n', '1'],opensd_exec='/mnt/c/codes/opensd/build/opensd')


# ## Results
# Verify the pressure evolutions at the outlet and the half-length of the pipe as shown in Figure 1 and Figure 2, respectively.

# <figure>
# <img src="tutorial3-1.png" style="width:50%">
# <figcaption align = "center"> Figure 1: Evolution of Pressure at Pipe Outlet </figcaption>
# </figure>
# 
# <figure>
# <img src="tutorial3-1.png" style="width:50%">
# <figcaption align = "center"> Figure 2: Evolution of Pressure at Pipe Half Length </figcaption>
# </figure>
