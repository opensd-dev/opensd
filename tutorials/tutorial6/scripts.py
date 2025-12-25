# def script1(flow_elem,WallTemp):
#     Pe = flow_elem.ther_gues.rhomass()*flow_elem.velocity*flow_elem.diameter*flow_elem.ther_gues.cpmass()/flow_elem.ther_gues.conductivity()
#     Nu = 6 + 0.006*Pe
#     h = Nu * flow_elem.ther_gues.conductivity() / flow_elem.diameter
#     return h
#
# def script2(flow_elem,WallTemp):
#     Pe = flow_elem.ther_gues.rhomass()*flow_elem.velocity*flow_elem.diameter*flow_elem.ther_gues.cpmass()/flow_elem.ther_gues.conductivity()
#     Nu = 4.82 + 0.0185*Pe**0.827
#     h = Nu * flow_elem.ther_gues.conductivity() / flow_elem.diameter
#     return h

def script1(velocity):
    # print("python flag1",velocity)
    h = 10000
    return h

def script2(velocity):
    # print("python flag2",velocity)
    h = 10000
    return h
