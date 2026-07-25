//! \file facether.cpp

#include "opensd/facether.h"

namespace opensd {
    
// FaceTher::FaceTher(Upstream uther, Downstream dther, Face face, bool flag_tp, double flowreg, Upstream unode, Downstream dnode) :
  // unode(unode), dnode(dnode), uther(uther), dther(dther), face(face), flag_tp(flag_tp), flowreg(flowreg), flstate(unode.circuit.flstate) {}
FaceTher::FaceTher(Face* face) :
  face(face) {}

 // void FaceTher::update(FlState* flstate) {
  void FaceTher::update() {
  // if (flstate == nullptr) {
    face->upstream->update_gues();
    face->downstream->update_gues();

    _drho_dp_consth = 0.5 * (face->upstream->drho_dp_consth + face->downstream->drho_dp_consth);
    _rhomass        = 0.5 * (face->upstream->rhomass_gues   + face->downstream->rhomass_gues);
    _viscosity      = 0.5 * (face->upstream->viscosity      + face->downstream->viscosity);
    _hmass          = 0.5 * (face->upstream->tenth_gues     + face->downstream->tenth_gues);
    _cpmass         = 0.5 * (face->upstream->cpmass         + face->downstream->cpmass);
    _conductivity   = 0.5 * (face->upstream->conductivity   + face->downstream->conductivity);
    
    // if (flag_tp) {
      // if (uther.Q() * dther.Q() < 0. && abs(face.heat_input) + abs(sum(face.heat_hslab)) > 0. && !(unode instanceof TPTank) && !(dnode instanceof TPTank)) {
        // update_sat();
        // if (_Qth < 0.) 
          // _Qth = 0.;
        // else if (_Qth > 1.) 
          // _Qth = 1.;
        // _rhomass = 1. / (_Qth / rhog + (1. - _Qth) / rhof);
        // _viscosity = 1. / (_Qth / mug + (1. - _Qth) / muf);
        // _hmass = _Qth * hg + (1. - _Qth) * hf;
      // }
    // }
  // } else {
    // _rhomass = flstate->rhomass();
    // _viscosity = flstate->viscosity();
    // _cpmass = flstate->cpmass();
    // _conductivity = flstate->conductivity();
    // _hmass = flstate->hmass();
    // _drho_dp_consth = 0.; //pending temp
  // }
}

void FaceTher::set_state(double rhomass, double cpmass, double viscosity,
                         double conductivity, double hmass, double drho_dp_consth) {
  _rhomass = rhomass;
  _cpmass = cpmass;
  _viscosity = viscosity;
  _conductivity = conductivity;
  _hmass = hmass;
  _drho_dp_consth = drho_dp_consth;
}
/*
void FaceTher::update_sat(double p) {
  if (p == nullptr) p = face.spres_gues;
  AbstractStateSat.update_sat(p);
}
*/

double FaceTher::rhomass() {
  return _rhomass;
}

double FaceTher::cpmass() {
  return _cpmass;
}

double FaceTher::viscosity() {
  return _viscosity;
}

double FaceTher::conductivity() {
  return _conductivity;
}
double FaceTher::drho_dp_consth() {
  return _drho_dp_consth;
}
/*
double FaceTher::Qth() {
  return _Qth;
}
 */

double FaceTher::hmass() {
  return _hmass;
}

} //namespace opensd
