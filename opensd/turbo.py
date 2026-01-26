import numpy as np
from scipy.interpolate import interp1d
import lxml.etree as ET
import math
from opensd.face import Face

class Pump(Face):
    def __init__(self,identifier,unode,ufrac,dnode,dfrac,Nop,flowreg):
        super().__init__(identifier,unode,ufrac,dnode,dfrac)
        self.identifier = identifier
        #pipe defined
        self.flowreg = flowreg
        delz = dnode.elevation - unode.elevation
        self.delz = delz
        unode.ofaces.append(self)
        dnode.ifaces.append(self)
        # unode.ofaces.append(self.faces[0])

        self.Nop = Nop
        self.circuit = self.unode.circuit
        self.circuit.faces.append(self)
        self.opening = 1.

    def update_abcoef(self,time,delt,trans_sim,alpha_mom):
        relax = 1
        self.aplus = (-1.-self.spres_gues/self.tpres_gues*0.5*self.ther_gues.drho_dp_consth()*self.delz*const.grav)/(relax*self.dHdQfuncs(self.vflow_gues))
        self.aminus = (-1.+self.spres_gues/self.tpres_gues*0.5*self.ther_gues.drho_dp_consth()*self.delz*const.grav)/(relax*self.dHdQfuncs(self.vflow_gues))
        self.bplus = self.bminus = self.spres_gues/self.tpres_gues * 0.5*self.ther_gues.drho_dp_consth()

    def eqn_mom(self,x,time,delt,trans_sim,alpha_mom):
        z = self.QHfuncs(x) - (self.dnode.tpres_gues-self.unode.tpres_gues) - self.ther_gues.rhomass()*const.grav*self.delz
        return z

class VSPump(Pump):
    def __init__(self,identifier,unode,ufrac,dnode,dfrac,curves,Nop,flowreg):
        super().__init__(identifier,unode,ufrac,dnode,dfrac,Nop,flowreg)
        #pump specific
        import pandas as pd
        speeds = []
        QHfuncs = []
        dHdQfuncs = []
        self.curves=curves
        for i,curve in enumerate(curves):
            speeds.append(curve[0])
            df = pd.read_csv(curve[1])
            Q = df.iloc[:,0]
            H = df.iloc[:,1]
            dHdQ = [np.gradient(H)/np.gradient(Q)]
            QHfuncs.append(interp1d(Q,H,fill_value="extrapolate"))
            dHdQfuncs.append(interp1d(Q,dHdQ,fill_value="extrapolate"))

        self.QHfuncs = lambda x: self.fun1(QHfuncs,x)
        self.dHdQfuncs = lambda x: self.fun1(dHdQfuncs,x)
        self.speeds = speeds
        self.Head = 0

    def fun1(self,funcs,x):
        ul = next(x for x,val in enumerate(self.speeds) if val >= self.Nop)
        N1=self.speeds[ul-1]
        N2=self.speeds[ul]
        try:
            y1=funcs[ul-1](x)[0]
            y2=funcs[ul](x)[0]
        except:
            y1=funcs[ul-1](x)
            y2=funcs[ul](x)
        y = interp1d([N1,N2],[y1,y2])(self.Nop)
        return y

    def to_xml_element(self,element):

        subelement = ET.SubElement(element, "vspump")
        subelement.set("identifier", str(self.identifier))
        subelement.set("curve_speed", str(self.curves[0][0]))
        subelement.set("curve_file", str(self.curves[0][1]))
        subelement.set("Nop", str(self.Nop))
        subelement.set("unode",      str(self.unode.identifier))
        subelement.set("dnode",      str(self.dnode.identifier))

class HPump(Pump):
    WH = np.array([0.634,0.643,0.646,0.640,0.629,0.613,0.595,0.575,0.552,0.533,.516,.505,0.504,0.51,0.512,0.522,0.539,0.559,\
        0.58,0.601,0.63,0.662,0.692,0.722,0.753,0.782,0.808,0.832,0.857,0.879,0.904,0.93,0.959,0.996,1.027,1.06,\
        1.09,1.124,1.165,1.204,1.238,1.258,1.271,1.282,1.288,1.281,1.26,1.225,1.172,1.107,1.031,0.942,0.842,0.733,\
        0.617,.5,.368,.24,.125,.011,-0.102,-.168,-.255,-.342,-.423,-.494,-.556,-.62,-.655,-.67,-.67,-.66,\
        -.655,-.64,-.6,-.57,-.52,-.47,-.43,-.36,-.275,-.16,-.04,+.13,.295,.43,.55,.62,.634])
    WB = np.array([-.684,-.547,-.414,-.292,-.187,-.105,-.053,-.012,.042,.097,.156,.227,.3,.371,.444,.522,.596,.672,\
        .738,.763,.797,.837,.865,.882,.886,.877,.859,.838,.804,.758,.703,.645,.583,.52,.454,.408,.37,.343,.331,.329,.338,.354,\
        .372,.405,.45,.486,.52,.552,.579,.603,.616,.617,.606,.582,.546,.5,.432,.36,.288,.214,.123,.037,-.053,-.161,-.248,-.314,\
        -.372,-.58,-.74,-.88,-1.,-1.12,-1.25,-1.37,-1.49,-1.59,-1.66,-1.69,-1.77,-1.65,-1.59,-1.52,-1.42,
        -1.32,-1.23,-1.1,-.98,-.82,-.684])
    DELX = math.pi/44.
    dWHdX = np.gradient(WH)/DELX

    def __init__(self,identifier,unode,ufrac,dnode,dfrac,data,Nop,flowreg):
        super().__init__(identifier,unode,ufrac,dnode,dfrac,Nop,flowreg)
        #pump specific
        self.Nrat = data[0]
        self.Qrat = data[1]
        self.Hrat = data[2]
        self.Trat = data[3]
        if (len(data)==4):
            pass
        elif (len(data)==6):
            self.WHfun = data[4]
            self.WH1fun = data[5]
        else:
            sys.exit("incorrect data for homologous pump characteristics. stopping")

    def QHfuncs(self,Q):

        alpha = self.Nop/self.Nrat
        upsilon = Q/self.Qrat
        WW = upsilon**2+alpha**2
        X = math.pi + math.atan2(upsilon,alpha)
        I1 = round(X/self.DELX+1)
        if hasattr(self,'WHfun'):
            WHH = self.WHfun(X)
        else:
            WHH = self.WH[I1-1]+(self.WH[I1]-self.WH[I1-1])*(X-(I1-1)*self.DELX)/self.DELX
        WBB = self.WB[I1-1]+(self.WB[I1]-self.WB[I1-1])*(X-(I1-1)*self.DELX)/self.DELX
        h = WHH*WW
        beta = WBB*WW
        BTAP = upsilon*h/alpha
        ETA = BTAP/beta
        H = h*self.Hrat

    # PUMP TORQUE PO CALCULATED HERE INCLUDES THE TORQUE CONSUMED BY FRICTION IN THE DRIVE SYSTEM -THE CORRELATION USED IS AS THAT USED IN SSC-L.
        if (alpha > 0.0117): #important pending change to absolute
            beta = beta + 0.023*alpha + 0.012
        elif (alpha <= 0.005):
            beta = beta + 14.77*alpha
        else:
            beta = beta + 0.117 - 8.97*alpha
        T = beta*self.Trat
        self.Head = H
        return H

    def dHdQfuncs(self,Q):
        upsilon = Q/self.Qrat
        alpha = self.Nop/self.Nrat

        X = math.pi + math.atan2(upsilon,alpha)
        if hasattr(self,'WHfun'):
            WHH = self.WHfun(X)
            WHH1 = self.WH1fun(X)
        else:
            I1 = round(X/self.DELX+1)
            WHH =  self.WH[I1-1]   +(self.WH[I1]   -self.WH[I1-1])   *(X-(I1-1)*self.DELX)/self.DELX
            WHH1 = self.dWHdX[I1-1]+(self.dWHdX[I1]-self.dWHdX[I1-1])*(X-(I1-1)*self.DELX)/self.DELX
        y = self.Hrat/self.Qrat*(2*upsilon*WHH + alpha*WHH1)

        return y


    def to_xml_element(self,element):

        subelement = ET.SubElement(element, "hpump")
        subelement.set("identifier", str(self.identifier))
        subelement.set("Nrat", str(self.Nrat))
        subelement.set("Qrat", str(self.Qrat))
        subelement.set("Hrat", str(self.Hrat))
        subelement.set("Trat", str(self.Trat))
        subelement.set("Nop", str(self.Nop))
        subelement.set("unode",      str(self.unode.identifier))
        subelement.set("dnode",      str(self.dnode.identifier))
