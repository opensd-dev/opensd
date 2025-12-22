def script1(flow_elem,wall_node):
    global dh,L,E,do
    Pe = flow_elem.ther_gues.rhomass()*abs(flow_elem.velocity)*flow_elem.diameter*flow_elem.ther_gues.cpmass()/flow_elem.ther_gues.conductivity()
    Nu = 8 * (dh/L + 0.027*(E/do-1.1)**0.46) * Pe**0.6
    h = Nu * flow_elem.ther_gues.conductivity() / flow_elem.diameter
    return h
