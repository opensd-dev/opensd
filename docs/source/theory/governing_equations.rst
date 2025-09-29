.. _theory_governing_equations:

===================
Governing Equations
===================

Governing Equations for Fluids (for "Pipe" Component)
=====================================================
The flow solver of the code solves fluid mass, momentum, and energy equations in one dimension. In two-phase regime, homogeneous flow assumption is made.


Continuity Equation
-------------------
The general continuity equation for fluid is given by :eq:`general_continuity`. Applying solid mechanics principles as given by :eq:`solid_mechanics`, the continuity equation can be rewritten in a more useful form for thin-walled pipes, as shown in :eq:`continuity`.

.. math::
   :label: general_continuity

	\frac{\partial {(\rho A)}}{{A}\partial t} + \frac{\partial {(\rho VA)}}{{A}\partial x} = d'''

.. math::
   :label: solid_mechanics

   \frac{dA}{dp} = \frac{A D c_1}{e E}

.. math::
   :label: continuity
   
   \rho \left( \frac{D c_1}{E e} + \frac{1}{\rho} \left( \frac{d\rho}{dp} \right)_h \right) \frac{\partial p}{\partial t}
   + \frac{1}{A} \frac{\partial (\rho V A)}{\partial x} = d^{\prime\prime\prime}


Where d''' is mass source per unit volume

Momentum Equation
-----------------
The momentum equation takes different forms for different flow components. 
The equation for a general pipe element in a non-choked condition is given 
by Equation :eq:`momentum`. A quantity named *total pressure* is defined in 
Equation :eq:`total_pressure`. Equation :eq:`momentum` can then be rewritten 
in terms of total pressure, as shown in Equation :eq:`total_pressure_form`. 
By writing in this form, the problematic acceleration pressure drop term is 
cast numerically more conveniently.

.. math::
   :label: momentum

   \frac{\partial(\rho V A)}{A \partial t}
   + \frac{\partial(\rho V^2 A)}{A \partial x}
   + \frac{\partial p}{\partial x}
   + \rho g \cos \theta
   + f \frac{\rho V |V|}{2 D}
   = S_x

.. math::
   :label: total_pressure

   p_0 = p + \frac{1}{2} \rho V^2

.. math::
   :label: total_pressure_form

   \rho \frac{\partial V}{\partial t}
   - \frac{V^2}{2} \frac{\partial \rho}{\partial x}
   + \frac{\partial p_0}{\partial x}
   + \rho g \cos \theta
   + f \frac{\rho V |V|}{2 D}
   = S_x


Energy Equation
---------------
The general energy equation for fluids is given by Equation :eq:`energy_general`. 
Total enthalpy is defined as shown in Equation :eq:`total_enthalpy`. 
Equation :eq:`energy_general` can then be rewritten, as shown in Equation :eq:`energy_rewritten`. 
The first term in the equation can then be linearized to obtain Equation :eq:`energy_linearized`. 
It is noted that the axial conduction in the fluid is not considered.

.. math::
   :label: energy_general

   \frac{\partial}{\partial t} \left( \rho h + \frac{\rho V^2}{2} - p \right)
   + \frac{\partial}{A \partial x} \left[ \rho V A \left( h + \frac{V^2}{2} + gZ \right) \right]
   + q''' = 0

.. math::
   :label: total_enthalpy

   h_0 = h + \frac{1}{2} V^2

.. math::
   :label: energy_rewritten

   \frac{\partial}{\partial t} \left( \rho h_0 - p \right)
   + \frac{\partial}{A \partial x} \left[ \rho V A \left( h_0 + gZ \right) \right]
   + q''' = 0

.. math::
   :label: energy_linearized

   \left[ \rho + h \left( \frac{\partial \rho}{\partial h} \right)_p \right] \frac{\partial h_0}{\partial t}
   - \frac{\partial p}{\partial t}
   + \frac{\partial}{A \partial x} \left[ \rho V A \left( h_0 + gZ \right) \right]
   + q''' = 0

Apart from conservation equations, the equation of state (EOS) is required to estimate density and its derivatives as a function of the primary variables, namely, pressure and enthalpy. 
The code has the provision to directly import all material properties from the CoolProp library :cite:`bell2014`, an open-source fluid properties library with more than 100 fluids.

Governing Equation for Solids (for “HeatSlab” Component)
========================================================
The general energy equation for solids is given by Equation :eq:`energy_solid`.

.. math::
   :label: energy_solid

   C_p \frac{\partial (\rho T)}{\partial t}
   = \frac{1}{A} \frac{\partial}{\partial x} \left( kA \frac{\partial T}{\partial x} \right)
   + \frac{1}{A} \frac{\partial}{\partial y} \left( kA \frac{\partial T}{\partial y} \right)
   + q'''



.. bibliography::
   :style: unsrt
   :filter: docname in docnames
