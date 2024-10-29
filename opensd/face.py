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

    def to_xml_element(self,element):

        subelement = ET.SubElement(element, "face")
        subelement.set("faceno", str(self.faceno))
        subelement.set("pipe", self.pipe.identifier)
        subelement.set("unode", self.unode.identifier)
        subelement.set("dnode", self.dnode.identifier)
