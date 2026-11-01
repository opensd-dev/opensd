.. _usersguide_scripts:

===========================
User-Defined Python Scripts
===========================

OpenSD can call Python functions for model inputs that are most conveniently
expressed as correlations.  The functions are placed in a file named
``scripts.py`` in the simulation working directory.  Function names supplied
to OpenSD must match the names defined in this file.

Callbacks should be deterministic and free of side effects.  OpenSD may call a
function many times while iterating to convergence, and compatible heat-transfer
callbacks may be evaluated using arrays for better performance.

HSLAB heat-transfer callbacks
=============================

For an HSLAB connection of type ``pipe``, the callback receives a fluid face
and the corresponding solid wall node and returns the heat-transfer coefficient
``h`` in W m\ :sup:`-2` K\ :sup:`-1`::

   def shell_htc(flow_elem, wall_node):
       Pe = (flow_elem.ther_gues.rhomass()
             * flow_elem.velocity
             * flow_elem.diameter
             * flow_elem.ther_gues.cpmass()
             / flow_elem.ther_gues.conductivity())
       Nu = 6.0 + 0.006 * Pe
       return Nu * flow_elem.ther_gues.conductivity() / flow_elem.diameter

Commonly used fluid-face quantities are:

``flow_elem.velocity``
   Fluid velocity at the face.

``flow_elem.diameter``
   Hydraulic diameter of a pipe face.

``flow_elem.ther_gues.rhomass()``
   Current fluid density.

``flow_elem.ther_gues.cpmass()``
   Current fluid specific heat capacity.

``flow_elem.ther_gues.conductivity()``
   Current fluid thermal conductivity.

The wall-node argument provides quantities such as ``temp_gues``, ``temp_old``,
``Ai``, and ``vol``.  A callback may use these quantities normally; such a
callback is evaluated through the general scalar path.

Automatic vectorization
=======================

OpenSD automatically vectorizes compatible ``pipe`` callbacks.  Users do not
need to write a second function, import NumPy, or attach a ``batch`` attribute.
The ordinary callback shown above is translated internally into an array
calculation when it uses elementwise arithmetic involving:

* ``flow_elem.velocity``;
* ``flow_elem.diameter``;
* ``rhomass()``;
* ``cpmass()``; and
* ``conductivity()``.

If a callback cannot be translated safely, OpenSD retains the original scalar
evaluation.  This fallback preserves general user-defined scripts.  Typical
reasons for using the scalar path include:

* scalar ``if`` statements or loops;
* dependence on ``wall_node``;
* access to other face or thermodynamic attributes; and
* calls to functions that do not accept NumPy arrays.

For best performance, keep frequently evaluated correlations as simple,
elementwise expressions.  Do not change a correlation merely to satisfy the
automatic vectorizer; correctness takes precedence, and the scalar path remains
fully supported.

Optional explicit batch function
================================

Advanced users may provide an explicit batch implementation.  OpenSD gives an
explicit implementation precedence over automatic vectorization::

   import numpy as np

   def shell_htc_batch(rhomass, velocity, diameter, cpmass, conductivity):
       rhomass = np.asarray(rhomass)
       velocity = np.asarray(velocity)
       diameter = np.asarray(diameter)
       cpmass = np.asarray(cpmass)
       conductivity = np.asarray(conductivity)
       Pe = rhomass * velocity * diameter * cpmass / conductivity
       return (6.0 + 0.006 * Pe) * conductivity / diameter

   shell_htc.batch = shell_htc_batch

The batch function must return one numeric value for every supplied face.  This
interface is optional and is mainly useful for correlations that can be
vectorized manually but are outside the patterns recognized automatically.

Practical guidance
==================

* Keep ``scripts.py`` beside the input files used for the run.
* Use the scalar callback as the authoritative definition of the correlation.
* Return values in the units expected by the selected model option.
* Avoid modifying OpenSD objects inside a callback.
* Test a new correlation first with a small mesh and result writing enabled.
* Check convergence and energy balance before performing timing studies.
* Disable result writing and diagnostic verbosity when measuring execution time.

The HSLAB callback examples used by Tutorial 6 are available in
``tutorials/tutorial6/scripts.py``.
