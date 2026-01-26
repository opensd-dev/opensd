# from scipy.interpolate import interp1d
# import numpy as np
# import math
# import CoolProp
from PINET import constants as const
# from PINET import solver_settings as solver
from PINET.flow_pipings import Face,GcrHEM

class GER(Face): #General Empirical Relationship
    def __init__(self,identifier,circuit,unode,ufrac,dnode,dfrac,Ck,m=2,n=2):
        super().__init__(identifier,unode,ufrac,dnode,dfrac)
        self.identifier = identifier
        self.circuit = circuit
        self.Ck = Ck
        self.m = m
        self.n = n
        self.delz = dnode.elevation - unode.elevation
        self.G = 0.
        self.opening = 1.
        unode.ofaces.append(self)
        dnode.ifaces.append(self)
        self.circuit.faces.append(self)

    def update_abcoef(self,time,delt,trans_sim,alpha_mom):
        self.branch.isolated = False
        if self.choked:
            self.aplus = self.bplus = 0.
            delta = 0.1
            y1 = self.Gcr/self.rhocr
            flstate = self.circuit.flstate
            flstate.update(CoolProp.HmassP_INPUTS,self.upstream.tenth_gues,self.upstream.tpres_gues+delta)
            Gcr2,pcr2,rhocr2 = GcrHEM(flstate)
            y2 = Gcr2/rhocr2
            self.aminus = (y2-y1)/delta*0.1
            self.bminus = (rhocr2-self.rhocr)/delta*0.1
        else:
            dr = (self.n * self.Ck * self.ther_gues.rhomass()**(self.m) * abs(self.vflow_gues)**(self.n-1) )
                                        # + 0.*2.*self.vflow_gues*(self.unode.ther_gues.rhomass()-self.dnode.ther_gues.rhomass())/(2.*self.cfarea**2) )
            self.aplus = ( ( 1.
                    + (self.spres_gues/self.tpres_gues*0.5*self.ther_gues.drho_dp_consth() * (  
                    const.grav*self.delz + self.m * self.Ck * self.ther_gues.rhomass()**(self.m-1) * self.vflow_gues*abs(self.vflow_gues)**(self.n-1) ) ) )
                    /dr )
            self.aminus = ( ( 1.
                    - (self.spres_gues/self.tpres_gues*0.5*self.ther_gues.drho_dp_consth() * (
                    const.grav*self.delz + self.m * self.Ck * self.ther_gues.rhomass()**(self.m-1) * self.vflow_gues*abs(self.vflow_gues)**(self.n-1) ) ) )
                    /dr )
            if self.aplus < 0. or self.aminus < 0.:
                if solver.show_warn:
                    print ("warning. acoef negative",self.identifier,self.aplus,self.aminus,self.unode.ther_gues.rhomass(),self.dnode.rhomass_gues,dr)
            self.bplus = self.bminus = self.spres_gues/self.tpres_gues * 0.5*self.ther_gues.drho_dp_consth()
            if self.bplus < 0:
                print (self.spres_gues,self.tpres_gues,self.ther_gues.drho_dp_consth())
                sys.exit("bcoefficient negative. stopping")
        
    def eqn_mom(self,x,time,delt,trans_sim,alpha_mom):
        if not self.choked:
            self.delp_fr = self.Ck * self.ther_gues.rhomass()**(self.m) * x * abs(x)**(self.n-1)
            self.delp_gr = self.ther_gues.rhomass()*const.grav*self.delz

            Term_old = (1.-alpha_mom) * ( (self.downstream.tpres_old - self.upstream.tpres_old)
                                           + self.ther_old.rhomass()*const.grav*self.delz
                                           + self.Ck * self.ther_old.rhomass() ** (self.m) * self.vflow_old * abs(self.vflow_old)**(self.n-1) )
            y = ( alpha_mom * ( (self.downstream.tpres_gues - self.upstream.tpres_gues)
                                       + self.delp_gr
                                       + self.delp_fr )
                    + Term_old )
        else:
            G = self.branch.faces[-1].G
            vflow_gues = G*self.cfarea/self.ther_gues.rhomass()
            y = x - vflow_gues
        return y
