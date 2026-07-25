//! \file connection.cpp
#include "opensd/connection.h"

#include "opensd/circuit.h"

namespace opensd {
    
Connection::Connection(std::shared_ptr<Node> node, double frac, double height) : node(node), frac(frac), height(height) {}

 void Connection::update_gues() {
  if (node->circuit->fltype != FluidType::INCOMPRESSIBLE && node->ther_gues->phase() == 6) {
    if (node->is_tptank) {
      drho_dp_consth = 0.0;
    } else {
      drho_dp_consth = node->ther_gues->first_two_phase_deriv(CoolProp::iDmass, CoolProp::iP, CoolProp::iHmass);
    }
  } else {
    drho_dp_consth = node->ther_gues->first_partial_deriv(CoolProp::iDmass, CoolProp::iP, CoolProp::iHmass);
  }

  if (node->is_tptank && frac >= 0.0 && node->ther_gues->phase() == 6) {
    if (height > node->level) {
      tpres_gues = node->spres_gues;
      tenth_gues = node->hg;
      rhomass_gues = node->rhog;
      viscosity = node->mug;
      cpmass = node->cpg;
      conductivity = node->kg;
    } else {
      tpres_gues = node->spres_gues + node->rhof * 9.81 * (node->level - height);
      tenth_gues = node->hf;
      rhomass_gues = node->rhof;
      viscosity = node->muf;
      cpmass = node->cpf;
      conductivity = node->kf;
    }
  } else {
    tpres_gues = node->tpres_gues;
    tenth_gues = node->tenth_gues;
    rhomass_gues = node->ther_gues->rhomass();
    viscosity = node->ther_gues->viscosity();
    cpmass = node->ther_gues->cpmass();
    conductivity = node->ther_gues->conductivity();
  }

  if (node->is_tptank && frac >= 0.0 && node->ther_gues->phase() == 0) {
    tpres_gues += node->rhof * 9.81 * (node->level - height);
  }

}

void Connection::update_old() {
  if (node->is_tptank && frac >= 0.0 && node->ther_old->phase() == 6) {
    if (height > node->level_old) {
      tpres_old = node->tpres_old;
      tenth_old = node->hg;
      rhomass_old = node->rhog;
    } else {
      tpres_old = node->tpres_old + node->rhof * 9.81 * (node->level_old - height);
      tenth_old = node->hf;
      rhomass_old = node->rhof;
    }
  } else {
    tpres_old = node->tpres_old;
    tenth_old = node->tenth_old;
    rhomass_old = node->ther_old->rhomass();
  }

  if (node->is_tptank && frac >= 0.0 && node->ther_old->phase() == 0) {
    tpres_old += node->rhof * 9.81 * (node->level_old - height);
  }
}

} // namespace opensd
