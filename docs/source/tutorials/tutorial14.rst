Tutorial 14: Sensitivity and Optimizer Functionalities
======================================================

Reference
---------

PINET reference: ``Tutorial 14 - Optimiser.docx``.

OpenSD files:

* :download:`Input script <../../../tutorials/tutorial14/tutorial.py>`

Problem description
-------------------

It is required to carry water through a pipe for a distance of 300 m, for a duration of 60000 h.
The discharge required through the pipe is 20 kg/s at 30 bar and 30 degrees C. The total cost of
installation and operation is calculated from the cost terms described below:

The pipe diameter is to be optimized.

Modeling steps
--------------

1. The circuit is built following the usual procedure.

2. Add the pipe diameter as the designer parameter and set the search range from 0.08 m to 0.35 m.

3. Define the optimizer result as total cost, including pump capital cost, pump running cost, and pipe cost.

4. Then sensitivity analysis is run to understand the effect of changing the pipe diameter.

5. When pipe diameter is varied from 0.08 to 0.35 m it is seen that the total cost decreases and then increases. Hence, there is a minimum in this range.

6. Now optimizer is run to obtain the minimum value.

Results
-------

The minimum total cost is obtained as 39944 for a diameter of 0.266 m
