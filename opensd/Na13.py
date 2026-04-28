import sys
import math
from fluidframe import fluidframe

class fluid(fluidframe):
    def __init__(self):
        self._rhomass = 860.
        self._molar_mass = 23.E-3
        self._viscosity = 3.75E-4
        self._adiabatic_compressibility=1.86E-10
        self._isothermal_compressibility=1.86E-10 #assumed equal to adiabatic_compressibility
        # self._first_partial_deriv = self._isothermal_compressibility*self._rhomass
        self._cpmass=1267.0
        self._cvmass=1266.9 #approximately
        self._conductivity=70.
        self._phase = 0
        self._Q = -1000.
        self._p = 1.E5

    def update(self,input_pair,val1,val2):
        if input_pair == 9: #PT_INPUTS tag in CoolProp
            self._T = val2
            self._hmass = self._cpmass*self._T
        elif input_pair == 20: #HmassP_INPUTS tag in CoolProp
            self._T = val1/self._cpmass
            self._hmass = self._cpmass*self._T
        elif input_pair == 2: #PQ_INPUTS
            if (val2 >= 0. and val2 <= 1.):
                self._T = 883.+273.
                self._hmass = self._cpmass*self._T + val2*2.23E6
            else:
                print ("Q out of range. stopping",val2)
                sys.exit()
        else:
            print ("input_pair not recognized. stopping",input_pair)
            sys.exit()
        self._speed_sound = math.sqrt(1./(self._adiabatic_compressibility*self._rhomass))
