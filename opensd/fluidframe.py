import sys
import math
import CoolProp

class fluidframe(object):
    def __init__(self):
        pass

    def rhomass(self):
        return self._rhomass
    def molar_mass(self):
        return self._molar_mass
    def viscosity(self):
        return self._viscosity
    # def adiabatic_compressibility(self):
        # return self._isothermal_compressibility
    def isothermal_compressibility(self):
        return self._isothermal_compressibility
    def first_partial_deriv(self,var1,var2,var3):
        if (var1 == CoolProp.iDmass and var2 == CoolProp.iP and var3 == CoolProp.iHmass):
            return self._isothermal_compressibility*self._rhomass
        elif (var1 == CoolProp.iDmass and var2 == CoolProp.iHmass and var3 == CoolProp.iP):
            return 0.
        else:
            print ("first_partial_deriv option not available. stopping")
            sys.exit()
    def first_two_phase_deriv(self,var1,var2,var3):
        return self._first_two_phase_deriv
    def speed_sound(self):
        return self._speed_sound
    def cpmass(self):
        return self._cpmass
    def cvmass(self):
        return self._cvmass
    def hmass(self):
        return self._hmass
    def conductivity(self):
        return self._conductivity
    def T(self):
        return self._T
    def phase(self):
        return self._phase
    def Q(self):
        return self._Q
    def p(self):
        return self._p

    def update(self,input_pair,val1,val2):
        if input_pair == CoolProp.PT_INPUTS:
            self._T = val2
            self._hmass = self._cpmass*self._T
        elif input_pair == CoolProp.HmassP_INPUTS:
            self._T = val1/self._cpmass
            self._hmass = self._cpmass*self._T
        elif input_pair == CoolProp.PQ_INPUTS:
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
