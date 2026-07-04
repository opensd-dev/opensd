import math
import sys

import CoolProp
import lxml.etree as ET

class Node(object):

    def __init__(self,identifier,circuit,volume,heat_input,elevation,tpres_old,ttemp_old,tenth_old,geom):
        """!@param identifier name specified by user (user input)
            @param heat_input heat input to the node (user input)
            @param esource energy source calculated for nodes with fixed 'T' or 'H' (output)
            @param msource mass source calculated for nodes with fixed 'P' (output) or mass source specified for nodes with fixed 'msource' (input)
        """
        ## name specified by user (user input)
        self.identifier = identifier
        self.circuit = circuit
        self.node_ind = len(self.circuit.nodes)

        if tpres_old is not None: self.tpres_old = tpres_old
        if ttemp_old is not None: self.ttemp_old = ttemp_old
        if tenth_old is not None: self.tenth_old = tenth_old
        
        self.ifaces=[]
        self.ofaces=[]
        
        self.heat_input = heat_input
        self.elevation = elevation
        
        self.fixed_var = []
        ## energy source calculated for nodes with fixed 'T' or 'H' (output)
        self.esource = 0. #for printing initialization
        self.mflow_in = 1.E-4
        ## mass source calculated for nodes with fixed 'P' (output) or \n mass source specified for nodes with fixed 'msource' (input)
        self.msource = 0.
        self.flowreg = "Homogeneous"
        self.hresidue = 0.
        self.mresidue = 0.
        self.volume = volume
        self.pc_flag = False

    # def assign_staticvar(self):
        # self.spres_old = self.tpres_old
        # self.stemp_old = self.ttemp_old
        # self.senth_old = self.tenth_old
        # self.velocity = 0.

    def to_xml_element(self,element):
        subelement = ET.SubElement(element, "node")
        subelement.set("identifier", self.identifier)
        # if self.elevation != 0.:
        subelement.set("elevation", str(self.elevation))
        subelement.set("tpres_old", str(self.tpres_old))
        subelement.set("ttemp_old", str(self.ttemp_old))
        subelement.set("tenth_old", str(self.tenth_old))
        subelement.set("msource",   str(self.msource))
        subelement.set("volume",    str(self.volume))
        subelement.set("heat_input",str(self.heat_input))
        subelement.set("fixed_var", ','.join(self.fixed_var))

class Reservoir(Node):
    def __init__(self,identifier,circuit,volume,heat_input,elevation,tpres_old,ttemp_old,tenth_old,geom):
        super().__init__(identifier,circuit,volume,heat_input,elevation,tpres_old,ttemp_old,tenth_old,geom)

        self.watermass = 0.
        if geom[0] == "vertcyl" or geom[0] == "horicyl":
            if geom[0] == "vertcyl":
                self.height = geom[2]
            elif geom[0] == "horicyl":
                self.height = geom[1]
        else:
            sys.exit("geometry not found. stopping")
        self.level = self.level_old = 0.
        self.geom = geom

class TPTank(Reservoir):
    def __init__(self,identifier,circuit,volume,heat_input,elevation,tpres_old,ttemp_old,tenth_old,geom):
        super().__init__(identifier,circuit,volume,heat_input,elevation,tpres_old,ttemp_old,tenth_old,geom)
        if geom[0] == "vertcyl" or geom[0] == "horicyl":
            self.tpvolume = math.pi*geom[1]**2/4.*geom[2]
            self.volume = self.tpvolume

