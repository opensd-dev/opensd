import numpy as np
import lxml.etree as ET
import sys,os

comps = {}

def add_SNode(identifier,Ai,Aj,vol,solname,sollib,heat_frac,layer):
    node = SNode(identifier,Ai,Aj,vol,solname,sollib,heat_frac,layer=layer)
    return node

class SNode(object):
    def __init__(self,identifier,Ai=None,Aj=None,vol=None,solname=None,sollib=None,heat_frac=np.array(1.),layer=None):
        self.identifier = identifier
        if layer: self.node_ind = len(layer.nodes)
        self.Ai=Ai
        self.Aj=Aj
        if vol is not None:
            self.volume = vol
        self.heat_input = 0.
        self.heat_input_old = 0.
        self.heat_frac = heat_frac
        if solname is not None:
            if sollib == "User":
                # a_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
                # sys.path.insert(0,os.getcwd() + "/")
                mod = __import__(f"opensd.{solname}", fromlist=[''])
                mat_clas = getattr(mod,'solid')
                self.ther_old = mat_clas()
                self.ther_gues = mat_clas()
            elif sollib == "thinmam":
                import thinmam
                self.ther_old = thinmam.state(solname)
                self.ther_gues = thinmam.state(solname)
            else:
                print ("solid material not found. stopping",solname,sollib)
                sys.exit()

        if layer is not None:
            self.layer = layer
            self.heat_input = self.heat_input_old = self.heat_frac*layer.heat_input
        comps[identifier] = self

        self.heat_transfer = 0.
        self.heat_transfer_old = 0.
        self.htc = 0.
        self.Sp = 0.
        self.Sc = 0.
        self.AFF = 1. #used for hflux boundary condition

    def eqn_ener(self,time,delt,trans_sim,alpha_heat):
        y = - alpha_heat*self.heat_input - (1.-alpha_heat)*self.heat_input_old
        if trans_sim:
            cp_avg = 0.5*(self.ther_old.cpmass()+self.ther_gues.cpmass())
            y = y + cp_avg*(self.ther_gues.rhomass()*self.temp_gues-self.ther_old.rhomass()*self.temp_old)*self.volume/delt
        if self.eface is not None:
            y = (y - alpha_heat*self.eface.A*self.eface.ther_gues.conductivity()*(self.eface.dnode.temp_gues-self.temp_gues)/self.eface.delx
                      - (1.-alpha_heat)*self.eface.A*self.eface.ther_old.conductivity()*(self.eface.dnode.temp_old-self.temp_old)/self.eface.delx )
        else:
            if self.layer.hslab.dvar == "pipe" or self.layer.hslab.dvar == "vof_pipe" or self.layer.hslab.dvar == "pipenl":
                flow_elem = self.layer.hslab.dval[1][self.node_ind-self.layer.ninc*(self.layer.nnodes-1)] #dcomp.faces[self.layer.nodes.index(self)-self.layer.ninc*(self.layer.nnodes-1)]
                if self.layer.hslab.dvar == "vof_pipe":
                    if flow_elem.volfrac1 <= 1.E-6:
                        h = 0.
                    else:
                        h=flow_elem.volfrac1*calc_value(self.layer.hslab.dval[0],flow_elem,self)+(1.-flow_elem.volfrac1)*0. #adiabatic second component assumed
                elif self.layer.hslab.dvar == "pipe":
                    # temp = Temp()
                    # temp.htreg = calc_value(self.layer.hslab.dhtmap,flow_elem,self)
                    # temp.temp_gues = self.temp_gues
                    # h=calc_value(self.layer.hslab.dval[0],flow_elem,temp)
                    # if self.layer.hslab.dhtmap is not None:
                        # a,b,c=self.layer.hslab.dhtmap(flow_elem,self)
                    h=calc_value(self.layer.hslab.dval[0],flow_elem,self)
                # if flow_elem.vflow_gues >= 0.:
                    # hA = h*node.Ai / (1.+h*node.Ai/(2.*flow_elem.vflow_gues*flow_elem.ther_gues.rhomass()*flow_elem.ther_gues.cpmass()))
                    # try:
                        # Tf = (flow_elem.unode.stemp_gues + (flow_elem.unode.velocity**2-flow_elem.dnode.velocity**2)/(4.*flow_elem.ther_gues.cpmass()))
                    # except:
                        # Tf = (flow_elem.unode.stemp_gues) #for vof pipes
                # else:
                    # hA = h*node.Ai / (1.+h*node.Ai/(2.*flow_elem.vflow_gues*flow_elem.ther_gues.rhomass()*flow_elem.ther_gues.cpmass()))
                    # try:
                        # Tf = (flow_elem.dnode.stemp_gues + (flow_elem.dnode.velocity**2-flow_elem.unode.velocity**2)/(4.*flow_elem.ther_gues.cpmass()))
                    # except:
                        # Tf = (flow_elem.dnode.stemp_gues) #for vof pipes
                elif self.layer.hslab.dvar == "pipenl":
                    Sc,Sp,h=self.layer.hslab.dval[0](flow_elem,self)
                Tf=flow_elem.stemp_gues
                hA = h*self.Ai
                y = y + alpha_heat*(self.temp_gues-Tf)*hA + (1.-alpha_heat)*self.heat_transfer_old
            elif self.layer.hslab.dvar == "conv":
                y = ( y + self.layer.hslab.dval[0]*(self.temp_gues-self.layer.hslab.dval[1])*self.Ai*alpha_heat
                        + self.heat_transfer_old*(1.-alpha_heat) )
            elif self.layer.hslab.dvar == "node":
                y = ( y + self.layer.hslab.dval[0]*(self.temp_gues-self.layer.hslab.dval[1].stemp_gues)*self.Ai*alpha_heat
                        + self.heat_transfer_old*(1.-alpha_heat) )
            elif self.layer.hslab.dvar == "hflux":
                y = ( y - self.layer.hslab.dval*self.Ai*alpha_heat*self.AFF
                        + self.heat_transfer_old*(1.-alpha_heat) )
            else:
                print ("ht option not found. stopping",self.layer.hslab.dvar)
                sys.exit()
            # y = y + self.heat_transfer #usage of self.heat_transfer to be avoided here. instead to be freshly calculated pending
        if self.wface is not None:
            y = (y + alpha_heat*self.wface.A*self.wface.ther_gues.conductivity()*(self.temp_gues - self.wface.unode.temp_gues)/self.wface.delx
                       + (1.-alpha_heat)*self.wface.A*self.wface.ther_old.conductivity()*(self.temp_old - self.wface.unode.temp_old)/self.wface.delx )
        else:
            if self.layer.hslab.uvar == "pipe" or self.layer.hslab.uvar == "vof_pipe" or self.layer.hslab.uvar == "pipenl":
                flow_elem = self.layer.hslab.uval[1][self.node_ind] #ucomp.faces[self.layer.nodes.index(self)]
                if self.layer.hslab.uvar == "vof_pipe":
                    if flow_elem.volfrac1 <= 1.E-6:
                        h = 0.
                    else:
                        h=flow_elem.volfrac1*calc_value(self.layer.hslab.uval[0],flow_elem,self)+(1.-flow_elem.volfrac1)*0. #adiabatic second component assumed
                elif self.layer.hslab.uvar == "pipe":
                    # self.htreg = calc_value(self.layer.hslab.uhtmap,flow_elem,self)
                    # temp = Temp()
                    # temp.htreg = calc_value(self.layer.hslab.dhtmap,flow_elem,self)
                    # temp.temp_gues = self.temp_gues
                    # h=calc_value(self.layer.hslab.uval[0],flow_elem,temp)
                    # if self.layer.hslab.uhtmap is not None:
                        # a,b,c=self.layer.hslab.uhtmap(flow_elem,self)
                    h=calc_value(self.layer.hslab.uval[0],flow_elem,self)
                # if flow_elem.vflow_gues >= 0.:
                    # hA = h*node.Ai / (1.+h*node.Ai/(2.*flow_elem.vflow_gues*flow_elem.ther_gues.rhomass()*flow_elem.ther_gues.cpmass()))
                    # Tf = (flow_elem.unode.stemp_gues + (flow_elem.unode.velocity**2-flow_elem.dnode.velocity**2)/(4.*flow_elem.ther_gues.cpmass()))
                # else:
                    # hA = h*node.Ai / (1.+h*node.Ai/(2.*flow_elem.vflow_gues*flow_elem.ther_gues.rhomass()*flow_elem.ther_gues.cpmass()))
                    # Tf = (flow_elem.dnode.stemp_gues + (flow_elem.dnode.velocity**2-flow_elem.unode.velocity**2)/(4.*flow_elem.ther_gues.cpmass()))
                elif self.layer.hslab.uvar == "pipenl":
                    Sc,Sp,h=self.layer.hslab.uval[0](flow_elem,self)
                Tf=flow_elem.stemp_gues
                hA = h*self.Ai
                y = y + alpha_heat*(self.temp_gues-Tf)*hA + (1.-alpha_heat)*self.heat_transfer_old
                # print (self.identifier,(self.temp_gues-Tf)*hA,self.temp_gues,Tf,h,self.layer.nodes.index(self),flow_elem.circuit.identifier)
            elif self.layer.hslab.uvar == "conv":
                y = ( y + alpha_heat*self.layer.hslab.uval[0]*(self.temp_gues-self.layer.hslab.uval[1])*self.Ai
                           + (1.-alpha_heat)*self.heat_transfer_old )
            elif self.layer.hslab.uvar == "node":
                flow_node = self.layer.hslab.uval[1]
                Tf = flow_node.stemp_gues
                Tw = self.temp_gues
                h = calc_value(self.layer.hslab.uval[0],flow_node,self)
                hA = h*self.Ai
                y = ( y + alpha_heat*(Tw-Tf)*hA + (1.-alpha_heat)*self.heat_transfer_old )
            elif self.layer.hslab.uvar == "hflux":
                y = ( y - alpha_heat*self.layer.hslab.uval*self.Ai*self.AFF
                           + (1.-alpha_heat)*self.heat_transfer_old )
            else:
                print ("ht option not found. stopping")
                sys.exit()
            # y = y + self.heat_transfer
        if self.nface is not None:
            y = ( y - 0.*alpha_heat*self.nface.A*self.nface.ther_gues.conductivity()*(self.nface.dnode.temp_gues-self.temp_gues)/self.layer.dely
                      - 0.*(1.-alpha_heat)*self.nface.A*self.nface.ther_old.conductivity()*(self.nface.dnode.temp_old-self.temp_old)/self.layer.dely )
        if self.sface is not None:
            y = ( y + 0.*alpha_heat*self.sface.A*self.sface.ther_gues.conductivity()*(self.temp_gues - self.sface.unode.temp_gues)/self.layer.dely
                      + 0.*(1.-alpha_heat)*self.sface.A*self.sface.ther_old.conductivity()*(self.temp_old - self.sface.unode.temp_old)/self.layer.dely )
        return y

    def update_old(self):
        self.temp_old = self.temp_gues
        self.heat_transfer_old = self.heat_transfer
        self.ther_old.update(self.temp_old)
        self.condeff_old = self.condeff_gues
        self.heat_input_old = self.heat_input
    def update_gues(self):
        self.temp_gues = self.temp_old
    def update_condeff(self):
        if self.layer.gap:
            T1 = self.wface.temp_gues
            T2 = self.eface.temp_gues
            if self.layer.cyl:
                self.condeff_gues = self.ther_gues.conductivity() + sigma*self.layer.epsbar*(T1**2+T2**2)*(T1+T2)*self.layer.thk_elem*min(self.wface.A,self.eface.A)/self.Ai
            else:
                self.condeff_gues = self.ther_gues.conductivity() + sigma*self.layer.epsbar*(T1**2+T2**2)*(T1+T2)*self.layer.thk_elem
        else:
            self.condeff_gues = self.ther_gues.conductivity()
    def update_heat_input(self,time,delt):
        self.heat_input = self.heat_frac*self.layer.heat_input

class HSlab:
    """Heat slab.

    Parameters
    ----------
    **kwargs : dict, optional
        Any keyword arguments are used to set attributes on the instance.

    Attributes
    ----------
    identifier : str
        Heat slab identifier

    """
    _registry = []
    
    def __init__(self,identifier,ucomp,uvar,uval,dcomp,dvar,dval,uarea,config=None,solveSS=True,nlayers=0,AFF=None,ninc=1):
        """!@param identifier identifier text for the heat slab (string)
            @param ucomp upstream component identifier text (string)
            @param uvar upstream boundary condition
            Options:
            •	“conv” – for convective boundary with known h and T∞
            •	“pipe” – for convective boundary with pipe flow
            •	“hflux” – for boundary with known heat flux
            •	“pipenl” – for convective boundary with pipe flow with convective HTC as nonlinear function (nucleate boiling regime)
        """
        self._registry.append(self)
        self.identifier = identifier
        self.uvar = uvar
        self.uval = uval
        self.dvar = dvar
        self.dval = dval
        self.solveSS=solveSS
        self.nlayers = nlayers
        self.ucompid = ucomp
        self.dcompid = dcomp
        self.config = config
        if AFF is not None:
            self.AFF = AFF

        # if self.uvar == "pipe" and self.dvar == "pipe":
            # if config is None:
                # self.config = "parallel"
            # else:
                # self.config = config

        comps[identifier] = self

        self.ufaces = []
        self.dfaces = []
        self.find_comps(ucomp,dcomp,config,ninc)

        self.dind_spl_nb=self.dind_nb_pd=self.dind_pd_spv = self.ninc+1
        self.uind_spl_nb=self.uind_nb_pd=self.uind_pd_spv = self.ninc+1
        self.dflg_spl_nb=self.dflg_nb_pd=self.dflg_pd_spv = False
        self.uflg_spl_nb=self.uflg_nb_pd=self.uflg_pd_spv = False

        # dely = thk_cross/ninc

        self.uarea = uarea
        self.darea = uarea

        self.layers=[]
        self.uwnodes=[]

    def find_comps(self,ucomp,dcomp,config,ninc):
        from opensd.project import get_comp

        try:
            self.ucomp = get_comp(ucomp)
            if isinstance(self.ucomp,comp.Pipe):
                pipe = self.ucomp
                uninc = pipe.ncell #no of increments in the upstream pipe
                self.uval.append([pipe.faces[i] for i in range(pipe.ncell)])
                for i in range(pipe.ncell):
                    self.ufaces.append(pipe.faces[i])
            elif isinstance(self.ucomp,comp.Node):
                self.uval.append(self.ucomp)
            else:
                sys.exit("ucomp connection not defined. stopping" + self.ucomp.identifier)
        except:
            pass

        try:
            self.dcomp = get_comp(dcomp)
            if isinstance(self.dcomp,comp.Pipe):
                pipe = self.dcomp
                dninc = pipe.ncell #no of increments in the downstream pipe
                self.dval.append([])
                if config == "counter":
                    for i in range(pipe.ncell):
                        self.dval[1].append(pipe.faces[pipe.ncell-i-1])
                        self.dfaces.append(pipe.faces[i])
                else:
                    for i in range(pipe.ncell):
                        self.dval[1].append(pipe.faces[i])
                        self.dfaces.append(pipe.faces[i])
            elif isinstance(self.dcomp,comp.Node):
                self.dval.append(self.dcomp)
            else:
                sys.exit("dcomp connection not defined. stopping" + self.dcomp.identifer)
        except:
            pass

        for key,value in comps.items():
            if key == ucomp:
                self.ucomp = value
            if key == dcomp:
                self.dcomp = value
        if not hasattr(self,"dcomp"):
            print ("Heat slab downstream component not found. stopping",dcomp)
            sys.exit()
        if not hasattr(self,"ucomp"):
            print ("Heat slab upstream component not found. stopping")
            sys.exit()

        # ninc = 1
        try:
            if uninc - dninc != 0:
                print (self.identifier + "upstream and downstream increments are not equal. stopping")
                sys.exit()
        except:
            pass
        try:
            ninc = uninc
        except:
            pass
        try:
            ninc = dninc
        except:
            pass
        self.ninc = ninc

    def add_layer(self,thk_elem,thk_cros,nnodes,darea,solname,sollib,ninc=None,heat_input=0.,AFF=None,gap=False,cyl=False,eps1=0.,eps2=0.):
        if self.layers==[]:
            uarea = self.uarea
        else:
            uarea = self.layers[-1].darea
        if ninc is not None:
            if self.ninc != 1 and ninc != self.ninc:
                print ("no. of increments can't be ",ninc,"since connected pipe has ",self.ninc,"increments.stopping",self.identifier)
                sys.exit()
        else:
            ninc = self.ninc
        if AFF is None:
            AFF = np.ones(ninc)/ninc
        else:
            if abs(sum(AFF)) - 1. >=1.E-5:
                print ("sum of AFF is not equal to 1. Please provide normalized vector",self.identifier,sum(AFF))
            if len(AFF) != ninc :
                print ("no. of increments can't be ",ninc,"since AFF has ",len(AFF),"elements. stopping.",self.identifier)
                sys.exit()
        if gap and nnodes != 1:
            print ("no. of radial nodes can't be not equal to 1. stopping")
            sys.exit()
        layer=Layer(len(self.layers),self,thk_elem,thk_cros,nnodes,uarea,darea,solname,sollib,ninc,heat_input,AFF,gap,cyl,eps1,eps2)
        self.layers.append(layer)
        return layer
    #def update_old(self):
        #self.uind_nb_pd_old = self.uind_nb_pd
        #self.uind_pd_spv_old = self.uind_pd_spv
        #self.uind_spl_nb_old = self.uind_spl_nb
        #self.dind_nb_pd_old =  self.dind_nb_pd
        #self.dind_pd_spv_old = self.dind_pd_spv
        #self.dind_spl_nb_old = self.dind_spl_nb

    def to_xml_element(self):
        """Create a 'hslab' element to be written to an XML file.

        """

        # Reset xml element tree
        element = ET.Element("hslab")
        element.set("identifier", str(self.identifier))
        element.set("ucomp", str(self.ucompid))
        element.set("uvar",  str(self.uvar))
        element.set("uval",  str(self.uval))
        element.set("dcomp", str(self.dcompid))
        element.set("dvar",  str(self.dvar))
        element.set("dval",  str(self.dval))
        element.set("uarea", str(self.uarea))
        element.set("ninc", str(self.ninc))

        # if self.solveSS:
        #     element.set("solveSS", "true")

        if self.layers:
            for layer in self.layers:
                layer.to_xml_element(element)

        # if self.bcs:
        #     for bc in self.bcs:
        #         bc.to_xml_element(element)

        return element

    def get_reference_prop(self):
        pass
        # if len(HSlab.layers)==0:
            # break
        # if HSlab.solveSS == False:
            # print ("HSlab temperature initialized to ambient")
            # for layer in HSlab.layers:
                # for node in layer.nodes:
                    # node.temp_gues = solver_settings.T_ambient
        # tlist2 = []
        # for i,node in enumerate(HSlab.uwnodes):
            # if HSlab.uvar == "pipe" or HSlab.uvar == "pipenl":
                # flow_elem = HSlab.uval[1][i]
                # node.temp_old = flow_elem.stemp_gues #may be required in hslab first steps
                # tlist2.append(node.temp_old)
            # elif HSlab.uvar == "conv":
                # node.temp_old = HSlab.uval[1]
                # tlist2.append(node.temp_old)
            # elif HSlab.uvar == "node":
                # flow_elem = HSlab.ucomp
                # node.temp_old = flow_elem.stemp_gues
                # tlist2.append(node.temp_old)
        # for i,node in enumerate(HSlab.dwnodes):
            # if HSlab.dvar == "pipe" or HSlab.dvar == "pipenl":
                # flow_elem = HSlab.dval[1][i]
                # node.temp_old = flow_elem.stemp_gues
                # tlist2.append(node.temp_old)
            # elif HSlab.dvar == "conv":
                # node.temp_old = HSlab.dval[1]
                # tlist2.append(node.temp_old)
            # elif HSlab.dvar == "node":
                # flow_elem = HSlab.dcomp
                # node.temp_old = flow_elem.stemp_gues
                # tlist2.append(node.temp_old)
        # if len(tlist2) == 0:
            # print ("error: tlist2 empty. cannot calculate initial temp for HSlab. stopping", HSlab.identifier)
            # sys.exit()
        # else:
            # tmean2 = sum(tlist2)/len(tlist2)
        # tref2=tmean2
        # for layer in HSlab.layers: #may not be used check pending
            # for i,node in enumerate(layer.nodes):
                # if not hasattr(node,'temp_old'):
                    # node.temp_old = tref2
                # node.update_gues() 
            # for face in layer.ifaces:
                # face.update_temp()

        # for layer in HSlab.layers:
            # for node in layer.nodes:
                # node.update_condeff()
                # node.condeff_old = node.condeff_gues
            # for face in layer.ifaces:
                # face.ther_old.update()
                # face.ther_gues.update()
            # for face in layer.jfaces:
                # face.ther_old.update()
                # face.ther_gues.update()

class Layer(object):
    def __init__(self,layerno,hslab,thk_elem,thk_cros,nnodes,uarea,darea,solname,sollib,ninc,heat_input,AFF,gap,cyl,eps1,eps2):
        self.layerno = layerno
        self.hslab = hslab
        self.nnodes = nnodes
        self.ninc = ninc
        self.thk_elem = thk_elem
        self.thk_cros = thk_cros
        if self.hslab.nlayers == 1:
            self.delx = thk_elem/(nnodes-1)
        elif self.layerno == 0: #first layer
            self.delx = thk_elem/(nnodes-1+0.5)
        elif self.layerno < self.hslab.nlayers-1:
            self.delx = thk_elem/nnodes
        else: #last layer
            self.delx = thk_elem/(nnodes-1+0.5)
        self.dely = thk_cros/ninc
        self.uarea = uarea
        self.darea = darea
        # self.mat = mat
        self.gap = gap
        self.cyl = cyl
        self._solname = solname
        self._sollib = sollib
        if self.gap:
            if cyl:
                self.epsbar = 1./(1./eps1+min(self.uarea,self.darea)/max(self.uarea,self.darea)*(1./eps2-1.))
            else:
                self.epsbar = 1./(1./eps1+1./eps2-1.)
        self.heat_input = heat_input

        self.nodes=[] #nodes creation
        self.hslab.dwnodes=[]
        for i in range(nnodes):
            for j in range(ninc):
                if self.hslab.nlayers == 1:
                    Ai = (uarea - i*(uarea-darea)/(nnodes-1))/ninc
                elif self.layerno == 0: #first layer
                    Ai = (uarea - i*(uarea-darea)/(nnodes-1+0.5))/ninc
                elif self.layerno < self.hslab.nlayers-1:
                    Ai = (uarea - (i+0.5)*(uarea-darea)/nnodes)/ninc
                else: #last layer
                    Ai = (uarea - (i+0.5)*(uarea-darea)/(nnodes-1+0.5))/ninc
                delz = Ai/self.dely
                Aj = self.delx*delz
                name = "layer" + str(self.layerno) + "_node" + str(i) + str(j)
                vol = self.delx*Ai
                if self.hslab.nlayers == 1:
                    if i == 0 or i == nnodes-1:
                        vol = vol/2.
                        heat_frac = 0.5*AFF[j]/(nnodes-1)
                    else:
                        heat_frac = AFF[j]/(nnodes-1)
                elif self.layerno == 0: #first layer
                    if i == 0:
                        vol = vol/2.
                        heat_frac = 0.5*AFF[j]/(nnodes-1+0.5)
                    else:
                        heat_frac = AFF[j]/(nnodes-1+0.5)
                elif self.layerno < self.hslab.nlayers-1:
                    heat_frac = AFF[j]/nnodes
                else: #last layer
                    if i == nnodes-1:
                        vol = vol/2.
                        heat_frac = 0.5*AFF[j]/(nnodes-1+0.5)
                    else:
                        heat_frac = AFF[j]/(nnodes-1+0.5)
                node = add_SNode(name,Ai,Aj,vol,solname,sollib,heat_frac,self)
                self.nodes.append(node)
                if i == 0 and self.layerno == 0:
                    self.hslab.uwnodes.append(node)
                    if hasattr(self.hslab,"AFF"): node.AFF = self.hslab.AFF[j]
                if i == nnodes-1:
                    self.hslab.dwnodes.append(node)
                    if hasattr(self.hslab,"AFF"): node.AFF = self.hslab.AFF[j]
                    self.hslab.darea = darea

        self.ifaces=[] #ifaces creation
        for i in range(nnodes-1):
            for j in range(ninc):
                name = "layer" + str(self.layerno) + "_iface" + str(i) + str(j)
                if self.hslab.nlayers == 1:
                    area = (uarea - (i+0.5)*(uarea-darea)/(nnodes-1))/ninc
                elif self.layerno == 0: #first layer
                    area = (uarea - (i+0.5)*(uarea-darea)/(nnodes-1+0.5))/ninc
                elif self.layerno < self.hslab.nlayers-1:
                    area = (uarea - (i+1)*(uarea-darea)/nnodes)/ninc
                else: #last layer
                    area = (uarea - (i+1)*(uarea-darea)/(nnodes-1+0.5))/ninc
                self.ifaces.append(Face(name,self.nodes[j+ninc*i],self.nodes[j+ninc*(i+1)],area))
        if self.layerno > 0: #layer interface faces
            for j in range(ninc):
                name = "interface" + str(self.layerno-1) + "_iface" + str(j)
                area = uarea/ninc
                prelayer = self.hslab.layers[self.layerno-1]
                self.ifaces.append(Face(name,prelayer.nodes[-ninc+j],self.nodes[j],area))

        self.jfaces=[] #jfaces creation
        for i in range(nnodes):
            for j in range(ninc-1):
                name = "layer" + str(self.layerno) + "_jface" + str(i) + str(j)
                area = 0.5*(self.nodes[j+ninc*i].Aj+self.nodes[j+ninc*i+1].Aj)
                self.jfaces.append(Face(name,self.nodes[j+ninc*i],self.nodes[j+ninc*i+1],area))

        for i in range(nnodes): #ifaces attachment to nodes
            for j in range(ninc):
                if i == nnodes-1: #downstream boundary nodes
                    self.nodes[j+ninc*i].eface=None
                    self.nodes[j+ninc*i].wface=self.ifaces[j+ninc*(i-1)]

                elif i == 0: #upstream boundary nodes
                    self.nodes[j+ninc*i].eface=self.ifaces[j+ninc*i]
                    self.nodes[j+ninc*i].wface=None
                else: #centrol nodes
                    self.nodes[j+ninc*i].eface=self.ifaces[j+ninc*i]
                    self.nodes[j+ninc*i].wface=self.ifaces[j+ninc*(i-1)]

        if self.layerno > 0: #layer interface faces
            for j in range(ninc):
                self.hslab.layers[self.layerno-1].nodes[-ninc+j].eface = self.ifaces[j+ninc*(nnodes-1)]
                self.nodes[j].wface = self.ifaces[j+ninc*(nnodes-1)]

        for i in range(nnodes): #jfaces attachment to nodes
            for j in range(ninc):
                if ninc == 1:
                    self.nodes[j+ninc*i].nface = None
                    self.nodes[j+ninc*i].sface = None
                elif j == ninc-1: #downstream boundary nodes
                    self.nodes[j+ninc*i].nface=None
                    self.nodes[j+ninc*i].sface=self.jfaces[j+(ninc-1)*i-1]
                elif j == 0: #upstream boundary nodes
                    self.nodes[j+ninc*i].nface=self.jfaces[j+(ninc-1)*i]
                    self.nodes[j+ninc*i].sface=None
                else: #centrol nodes
                    self.nodes[j+ninc*i].nface=self.jfaces[j+(ninc-1)*i]
                    self.nodes[j+ninc*i].sface=self.jfaces[j+(ninc-1)*i-1]

    def to_xml_element(self,element):
        """Create a 'layer' element to be written to an XML file.

        """

        # Reset xml element tree
        subelement = ET.SubElement(element, "layer")
        subelement.set("layerno", str(self.layerno))

        if self._solname is not None:
            subelement.set("solname", self._solname)
        else:
            raise ValueError(f'Solid has not been assigned for heat slab {self.hslab.identifier} layer  {self.layerno}!')

        subelement.set("sollib", self._sollib)
        subelement.set("thk_elem", str(self.thk_elem))
        subelement.set("thk_cros", str(self.thk_cros))
        subelement.set("nnodes", str(self.nnodes))
        subelement.set("darea", str(self.darea))

class Face(object):
    def __init__(self,identifier,unode,dnode,A):
        self.identifier = identifier

        self.unode = unode
        self.dnode = dnode

        self.A = A
        self.delx1 = self.unode.layer.delx
        self.delx2 = self.dnode.layer.delx
        self.delx = 0.5*(self.delx1+self.delx2)

        self.ther_old = FaceTher(unode,dnode,'old')
        self.ther_gues = FaceTher(unode,dnode,'gues')

    def update_old(self):
        self.ther_old.update()

    def update_temp(self):
        self.temp_gues = ( (self.unode.temp_gues*self.unode.ther_gues.conductivity()*self.delx2 + #interface temperature
                                    self.dnode.temp_gues*self.dnode.ther_gues.conductivity()*self.delx1) /
                                        (self.unode.ther_gues.conductivity()*self.delx2+self.dnode.ther_gues.conductivity()*self.delx1) )
        # self.temp_gues2 = (self.unode.temp_gues + self.dnode.temp_gues) / 2

class FaceTher(object):
    def __init__(self,unode,dnode,tempo):
        self.unode=unode
        self.dnode=dnode
        self.tempo=tempo
        self.delx1 = self.unode.layer.delx
        self.delx2 = self.dnode.layer.delx
        # self.update()

    def update(self):
        if self.tempo == 'old':
            self._conductivity = (self.delx1+self.delx2)/(self.delx1/self.unode.condeff_old+self.delx2/self.dnode.condeff_old)
        elif self.tempo == 'gues':
            self._conductivity = (self.delx1+self.delx2)/(self.delx1/self.unode.condeff_gues+self.delx2/self.dnode.condeff_gues)
        else:
            print ("incorrect tempo. stopping")
            sys.exit()

    def conductivity(self):
        return self._conductivity
