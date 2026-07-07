L = 22.
E = 0.0322
do = 0.0172
di = 0.0126
N = 547
Di = 0.831

import math

Af = math.pi*Di**2/4. - N*math.pi*do**2/4.
Pw = math.pi*Di + N*math.pi*do
dh = 4.*Af/Pw


def script1(flow_elem,wall_node):
    Pe = flow_elem.ther_gues.rhomass()*abs(flow_elem.velocity)*flow_elem.diameter*flow_elem.ther_gues.cpmass()/flow_elem.ther_gues.conductivity()
    Nu = 8 * (dh/L + 0.027*(E/do-1.1)**0.46) * Pe**0.6
    h = Nu * flow_elem.ther_gues.conductivity() / flow_elem.diameter
    return h
