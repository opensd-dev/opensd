import numpy as np
import lxml.etree as ET

class Face(object): #partial class
    def __init__(self,faceno,unode,ufrac,dnode,dfrac):
        
        self.vflow_old=np.array(1.E-8)
        self.mflow = 0.
        self.velocity=0.

        self.unode = unode
        self.ufrac = ufrac
        if ufrac is not None and isinstance(self.unode,Reservoir): self.uheight = ufrac*self.unode.height
        self.dnode = dnode
        self.dfrac = dfrac
        if dfrac is not None and isinstance(self.dnode,Reservoir): self.dheight = dfrac*self.dnode.height
        
        
        # self.heat_input = self.heat_input_old = 0.
        self.heat_input_old = 0.
        self.heat_input = 0.
        self.heat_hslab = self.heat_hslab_old = []
        self.choked = False
        self.presidue = 0.
        self.Gcr = 1.E8
        self.pcr = 0.

    def assign_statevar(self):
        self.tpres_old = 0.5*(self.unode.tpres_old+self.dnode.tpres_old)
        self.spres_old = 0.5*(self.unode.spres_old+self.dnode.spres_old)
        self.ttemp_old = 0.5*(self.unode.ttemp_old+self.dnode.ttemp_old)
        self.stemp_old = 0.5*(self.unode.stemp_old+self.dnode.stemp_old)

class PFace(Face):

    def __init__(self,faceno,pipe,unode,ufrac,dnode,dfrac,diameter,cfarea,delx,delz,fricopt,roughness):
        super().__init__(faceno,unode,ufrac,dnode,dfrac)
        self.circuit = pipe.circuit
        self.faceno=faceno
        self.pipe=pipe
        self.diameter=diameter
        self.cfarea=cfarea
        self.delx=delx
        self.delz=delz
        self.roughness=roughness
        self.Re = 0. 
        self.fricopt = fricopt
        self.fricfact_old = 64. #maximum initial friction factor considered
        # self.flstate = self.pipe.circuit.flstate
        self.opening = 1.
        self.circuit.faces.append(self)

    # def to_xml_element(self,element):
    #
    #     subelement = ET.SubElement(element, "face")
    #     subelement.set("faceno", str(self.faceno))
    #     subelement.set("pipe", self.pipe.identifier)
    #     subelement.set("unode", self.unode.identifier)
    #     subelement.set("dnode", self.dnode.identifier)

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

    def to_xml_element(self,element):

        subelement = ET.SubElement(element, "ger")
        subelement.set("identifier", str(self.identifier))
        subelement.set("Ck", str(self.Ck))
        subelement.set("m", str(self.m))
        subelement.set("n", str(self.n))
        subelement.set("unode",      str(self.unode.identifier))
        subelement.set("dnode",      str(self.dnode.identifier))
