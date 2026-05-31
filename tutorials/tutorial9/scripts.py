import math

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

def script1(flow_elem,wall_node):
    global dh,L,E,do
    Pe = flow_elem.ther_gues.rhomass()*abs(flow_elem.velocity)*flow_elem.diameter*flow_elem.ther_gues.cpmass()/flow_elem.ther_gues.conductivity()
    Nu = 8 * (dh/L + 0.027*(E/do-1.1)**0.46) * Pe**0.6
    h = Nu * flow_elem.ther_gues.conductivity() / flow_elem.diameter
    return h

def script2(flow_elem,wall_node):
    
    index_spl_nb = wall_node.layer.hslab.dind_spl_nb
    index_nb_pd = wall_node.layer.hslab.dind_nb_pd
    index_pd_spv = wall_node.layer.hslab.dind_pd_spv
    
    if ( flow_elem.faceno < index_spl_nb ): #single phase liquid
        Re = flow_elem.ther_gues.rhomass()*abs(flow_elem.velocity)*flow_elem.diameter/flow_elem.ther_gues.viscosity() #may be flow_elem.Re directly used
        Pr = flow_elem.ther_gues.viscosity()*flow_elem.ther_gues.cpmass()/flow_elem.ther_gues.conductivity()
        n = 0.43
        if Re < 2300.:
            Nu = 4.364
        elif Re > 5000.:
            Nu = 0.021*Re**0.8*Pr**n
        else:
            Nu1 = 4.364
            Nu2 = 0.023*5000.**0.8*Pr**n
            Nu = Nu1 + (Re-2300.)*(Nu2-Nu1)/(5000.-2300.)
        h = Nu * flow_elem.ther_gues.conductivity() / flow_elem.diameter
        Sc = h*flow_elem.stemp_gues
        Sp = -h
        # h = 20000.
    elif ( flow_elem.faceno < index_nb_pd ): #nucleate boiling

        Ref = flow_elem.ther_gues.rhomass()*abs(flow_elem.velocity)*flow_elem.diameter/flow_elem.ther_gues.muf
        Prf = flow_elem.ther_gues.muf*flow_elem.ther_gues.cpf/flow_elem.ther_gues.kf
        hc=0.019*flow_elem.ther_gues.kf/flow_elem.diameter*Ref**0.8*Prf**0.333
		
        Tsat = flow_elem.ther_gues.Tsat
        hfg = flow_elem.ther_gues.hg-flow_elem.ther_gues.hf
        term1 = flow_elem.ther_gues.muf*hfg
        sigma = (7.66789E-02) - 1.675265E-04*(Tsat-273.) - 1.0E-07*(Tsat-273.)*(Tsat-273.) #pending move to appropriate location
        term2 = (grav*(flow_elem.ther_gues.rhof-flow_elem.ther_gues.rhog)/sigma)**0.5
        term3 = (flow_elem.ther_gues.cpf/(0.013*hfg*Prf**1.7))**3
        
        C = term1*term2*term3
        
        Tw = wall_node.temp_gues
        Tf = flow_elem.stemp_gues
        hb = C*(Tw-Tsat)**2
        h = (hb+hc)*(Tw-Tsat)/(Tw-Tf)
        
        B = hc
        Sp = -3.*C*(Tw-Tsat)**2-B
        Sc = -C*((Tw-Tsat)**3-3.*(Tw-Tsat)**2*Tw) + B*Tsat
        
    elif ( flow_elem.faceno < index_pd_spv ): #post dryout
        x = flow_elem.ther_gues.Qth()
        if x >1.: #to avoid complex number error #pending improvise
            x = 1.
        Yf = (1. - 0.1*(flow_elem.ther_gues.rhof/flow_elem.ther_gues.rhog-1.)**0.4*(1.-x)**0.4) * (x+flow_elem.ther_gues.rhog/flow_elem.ther_gues.rhof*(1.0-x))**0.8
        Re_vap = flow_elem.ther_gues.rhomass()*abs(flow_elem.velocity)*flow_elem.diameter/flow_elem.ther_gues.mug
        prandtl_vap = flow_elem.ther_gues.mug * flow_elem.ther_gues.cpg/flow_elem.ther_gues.kg
        Nu=0.021*Re_vap**0.8*prandtl_vap**0.43
        Nu = Nu * Yf
        h = Nu * flow_elem.flstate.conductivity() / flow_elem.diameter
        Sc = h*flow_elem.stemp_gues
        Sp = -h
        # h = 20000.
    else: #single phase vapor
        Re = flow_elem.ther_gues.rhomass()*abs(flow_elem.velocity)*flow_elem.diameter/flow_elem.ther_gues.viscosity() #may be flow_elem.Re directly used
        Pr = flow_elem.ther_gues.viscosity()*flow_elem.ther_gues.cpmass()/flow_elem.ther_gues.conductivity()
        n = 0.43
        if Re < 2300.:
            Nu = 4.364
        elif Re > 5000.:
            Nu = 0.021*Re**0.8*Pr**n
        else:
            Nu1 = 4.364
            Nu2 = 0.023*5000.**0.8*Pr**n
            Nu = Nu1 + (Re-2300.)*(Nu2-Nu1)/(5000.-2300.)
        h = Nu * flow_elem.ther_gues.conductivity() / flow_elem.diameter
        Sc = h*flow_elem.stemp_gues
        Sp = -h
    return Sc,Sp,h
    # h = 20000.
    # Sc = h*flow_elem.stemp_gues
    # Sp = -h
    # return h
