//! \file connection.cpp
#include "opensd/connection.h"

#include "opensd/circuit.h"

namespace opensd {
    
Connection::Connection(std::shared_ptr<Node> node, double frac, double height) : node(node), frac(frac), height(height) {}

 void Connection::update_gues() {
  if (node->circuit->fltype != FluidType::INCOMPRESSIBLE && node->ther_gues->phase() == 6) {
    // if (node instanceof TPTank) {
      // drho_dp_consth = 0.;
    // } else {
      drho_dp_consth = node->ther_gues->first_two_phase_deriv(CoolProp::iDmass, CoolProp::iP, CoolProp::iHmass);
    // }
  } else {
    drho_dp_consth = node->ther_gues->first_partial_deriv(CoolProp::iDmass, CoolProp::iP, CoolProp::iHmass);
  }

  // if (node instanceof TPTank && frac != nullptr && node->ther_gues.phase() == 6) {
    // if (height > node->level) {
      // tpres_gues = node->spres_gues;
      // tenth_gues = node->ther_gues.hg;
      // rhomass_gues = node->ther_gues.rhog;
      // viscosity = node->ther_gues.mug;
      // cpmass = node->ther_gues.cpg;
      // conductivity = node->ther_gues.kg;
    // } else {
      // tpres_gues = node->spres_gues + node->ther_gues.rhof * const.grav * (node->level - height);
      // tenth_gues = node->ther_gues.hf;
      // rhomass_gues = node->ther_gues.rhof;
      // viscosity = node->ther_gues.muf;
      // cpmass = node->ther_gues.cpf;
      // conductivity = node->ther_gues.kf;
    // }
  // } else {
    tpres_gues = node->tpres_gues;
    tenth_gues = node->tenth_gues;
    rhomass_gues = node->ther_gues->rhomass();
    viscosity = node->ther_gues->viscosity();
    cpmass = node->ther_gues->cpmass();
    conductivity = node->ther_gues->conductivity();
  // }

  // if (node instanceof TPTank && frac != nullptr && node->ther_gues.phase() == 0) {
    // tpres_gues = tpres_gues + node->ther_gues.rhof * const.grav * (node->level - height);
  // }

}

void Connection::update_old() {
  // if (node instanceof TPTank && frac != nullptr && node->ther_old.phase() == 6) {
    // if (height > node->level_old) {
      // tpres_old = node->tpres_old;
      // tenth_old = node->ther_old.hg;
      // rhomass_old = node->ther_old.rhog;
    // } else {
      // tpres_old = node->tpres_old + node->ther_old.rhof * const.grav * (node->level_old - height);
      // tenth_old = node->ther_old.hf;
      // rhomass_old = node->ther_old.rhof;
    // }
  // } else {
    tpres_old = node->tpres_old;
    tenth_old = node->tenth_old;
    rhomass_old = node->ther_old->rhomass();
  // }

  // if (node instanceof TPTank && frac != nullptr && node->ther_old.phase() == 0) {
    // tpres_old = tpres_old + node->ther_old.rhof * const.grav * (node->level_old - height);
  // }
}

} // namespace opensd
