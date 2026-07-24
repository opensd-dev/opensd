import math
import numpy as np
import lxml.etree as ET
from opensd.face import PFace

class Pipe(object):

    def __init__(self,identifier,circuit,diameter,length,unode,ufrac,dnode,dfrac,fricopt,roughness,ncell,heat_input,cfarea,npar,qcrit,Kforward,flowreg):
        self.identifier=identifier
        self.circuit = circuit
        self.flowreg = flowreg

        self.diameter=diameter
        self.length=length

        self.ncell=ncell
        self.unode=unode
        self.ufrac = ufrac
        self.dnode=dnode
        self.dfrac = dfrac
        
        self.npar = npar
        self.qcrit = qcrit
        self.Kforward_old = self.Kforward = Kforward
        self.fricopt = fricopt

        delx = length/ncell
        delz = (dnode.elevation - unode.elevation)/ncell
        if delz - delx > 1.E-6:
            print ("length is less than the elevation difference. stopping",self.identifier)
            sys.exit()
        
        self.cfarea1 = cfarea
        if cfarea is None:
            cfarea=math.pi*diameter**2/4.
            
        self.cfarea = cfarea * npar
            
        self.heat_input = heat_input
        self.roughness = roughness

        # self.nodes = []
        # for i in range(ncell-1):
            # node=circuit.add_node(identifier+"_node"+str(i),volume=delx*cfarea,elevation=unode.elevation+(dnode.elevation-unode.elevation)*(i+1)/ncell)
            # node.i = i
            # self.nodes.append(node)
        
        # self.faces = []
        # for i in range(ncell):
            # if i==0 and ncell==1:
                # self.faces.append(PFace(i,self,unode,ufrac,dnode,dfrac,diameter,cfarea,delx,delz,fricopt,roughness))
            # elif i==0:
                # self.faces.append(PFace(i,self,unode,ufrac,self.nodes[0],None,diameter,cfarea,delx,delz,fricopt,roughness))
            # elif i==ncell-1:
                # self.faces.append(PFace(i,self,self.nodes[ncell-2],None,dnode,dfrac,diameter,cfarea,delx,delz,fricopt,roughness))
            # else:
                # self.faces.append(PFace(i,self,self.nodes[i-1],None,self.nodes[i],None,diameter,cfarea,delx,delz,fricopt,roughness))
            
        # for i in range(ncell-1):
            # self.nodes[i].ifaces.append(self.faces[i])
            # self.nodes[i].ofaces.append(self.faces[i+1])

        # unode.ofaces.append(self.faces[0])
        unode.volume = unode.volume + 0.5*delx*self.cfarea

        # dnode.ifaces.append(self.faces[-1])
        dnode.volume = dnode.volume + 0.5*delx*self.cfarea
        
        self.mflow = 0.
        
    def add_wall(self,thk,solname,sollib,restraint):
        wall = Wall(thk,solname,sollib,restraint)
        # for face in self.faces:
            # face.wall = wall

    def to_xml_element(self,element):
        subelement = ET.SubElement(element, "pipe")
        subelement.set("identifier", self.identifier)
        subelement.set("diameter",   str(self.diameter))
        subelement.set("length",     str(self.length))
        subelement.set("ncell",      str(self.ncell))
        subelement.set("unode",      str(self.unode.identifier))
        subelement.set("dnode",      str(self.dnode.identifier))
        subelement.set("roughness",  str(self.roughness))
        subelement.set("fricopt",    str(self.fricopt))
        subelement.set("cfarea",     str(self.cfarea))
        subelement.set("heat_input", str(self.heat_input))
        subelement.set("Kforward",   str(self.Kforward))
        subelement.set("ufrac",      "-1" if self.ufrac is None else str(self.ufrac))
        subelement.set("dfrac",      "-1" if self.dfrac is None else str(self.dfrac))

class Wall(object):
    def __init__(self,thk,solname,sollib,restraint):
        if sollib == "User":
            import sys
            import os
            sys.path.insert(0,os.getcwd() + "/")
            mod = __import__(solname)
            mat_clas = getattr(mod,'solid')
            self.mech_gues = mat_clas('mech')
        elif sollib == "thinmam":
            import thinmam
            self.mech_gues = thinmam.state(solname,'mech')
        else:
            print ("solid material not found. stopping",solname,sollib)
            sys.exit()
        if restraint == "long":
            self.c1 = 5./4. - self.mech_gues.poissons_ratio()
        else:
            print ("restraint option not found. stopping",self.identifier,restraint)
            sys.exit()
        self.thk = thk
