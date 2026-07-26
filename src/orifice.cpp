//! \file orifice.cpp
#include "opensd/orifice.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <tuple>

#include "AbstractState.h"
#include "CoolProp.h"
#include "opensd/circuit.h"
#include "opensd/constants.h"
#include "opensd/error.h"
#include "opensd/hdf5_interface.h"
#include "opensd/xml_interface.h"

namespace opensd {

namespace {

struct CriticalState {
  double Gcr = 0.0;
  double pcr = 0.0;
  double rhocr = 0.0;
  double T = 0.0;
  double h = 0.0;
  double cp = 0.0;
  double mu = 0.0;
  double k = 0.0;
};

CriticalState GcrHEM(CoolProp::AbstractState& flstate)
{
  const double h0 = flstate.hmass();
  const double p0 = flstate.p();
  const double s0 = flstate.smass();
  const double pmin = std::max(0.001 * p0, 1000.0);
  const double pmax = std::min(0.999 * p0, 220.0E5);

  CriticalState cr;
  cr.pcr = pmin;
  cr.rhocr = flstate.rhomass();

  for (int i = 0; i < 50; ++i) {
    const double p = pmin + (pmax - pmin) * static_cast<double>(i) / 49.0;
    flstate.update(CoolProp::PSmass_INPUTS, p, s0);
    const double h = flstate.hmass();
    const double rho = flstate.rhomass();
    const double G = h > h0 ? 0.0 : rho * std::sqrt(2.0 * (h0 - h));
    if (G > cr.Gcr) {
      cr.Gcr = G;
      cr.pcr = p;
      cr.rhocr = rho;
    }
  }

  flstate.update(CoolProp::PSmass_INPUTS, cr.pcr, s0);
  cr.T = flstate.T();
  cr.h = flstate.hmass();
  cr.cp = flstate.cpmass();
  cr.mu = flstate.viscosity();
  cr.k = flstate.conductivity();
  return cr;
}

} // namespace

Orifice::Orifice(pugi::xml_node orifice_node)
{
  if (check_for_node(orifice_node, "identifier")) {
    this->identifier = get_node_value(orifice_node, "identifier");
  } else {
    fatal_error("Must specify identifier of orifice in geometry XML file.");
  }

  this->diameter  = stod(get_node_value(orifice_node, "diameter"));
  this->Cd        = stod(get_node_value(orifice_node, "Cd"));
  this->opening   = orifice_node.attribute("opening").as_double(1.0);
  this->dnode_str = get_node_value(orifice_node, "dnode");
  this->unode_str = get_node_value(orifice_node, "unode");
  this->ufrac = orifice_node.attribute("ufrac").as_double(-1.0);
  this->dfrac = orifice_node.attribute("dfrac").as_double(-1.0);
  this->cfarea = PI * diameter * diameter / 4.0;
  this->G = 0.0;
  this->unode = nullptr;
  this->dnode = nullptr;
  this->faceno = 0;
  this->uheight = 0.0;
  this->dheight = 0.0;
  this->delz = 0.0;
  this->vflow_old = 1.0E-8;
  this->vflow_gues = 1.0E-8;
  this->mflow = 0.0;
  this->velocity = 0.0;
  this->heat_input_old = 0.0;
  this->heat_input = 0.0;
  this->choked = false;
  this->presidue = 0.0;
  this->Gcr = 1.0E8;
  this->pcr = 0.0;
  this->rhocr = 0.0;
  this->cr_ttemp = 0.0;
  this->cr_hmass = 0.0;
  this->cr_cpmass = 0.0;
  this->cr_viscosity = 0.0;
  this->cr_conductivity = 0.0;
  this->aplus = 0.0;
  this->aminus = 0.0;
  this->bplus = 0.0;
  this->bminus = 0.0;
  this->owner = 0;
}

double Orifice::eqn_mom(double x, double time, double delt, bool trans_sim, double alpha_mom)
{
  if (opening == 0.0) {
    return x;
  }

  if (!choked) {
    delp_fr = 1.0 / (opening * opening * Cd * Cd)
              * ther_gues->rhomass() * x * std::abs(x) / (2.0 * cfarea * cfarea);
    delp_gr = ther_gues->rhomass() * grav * delz;

    double term_old =
      (1.0 - alpha_mom) *
      (
        (downstream->tpres_old - upstream->tpres_old)
        - vflow_old * vflow_old / (2.0 * cfarea * cfarea)
          * (downstream->rhomass_old - upstream->rhomass_old)
        + ther_old->rhomass() * grav * delz
        + 1.0 / (opening * opening * Cd * Cd)
          * ther_old->rhomass() * vflow_old * std::abs(vflow_old)
          / (2.0 * cfarea * cfarea)
      );

    return alpha_mom *
      (
        (downstream->tpres_gues - upstream->tpres_gues)
        - vflow_gues * vflow_gues / (2.0 * cfarea * cfarea)
          * (downstream->rhomass_gues - downstream->rhomass_old)
        + delp_gr
        + delp_fr
      )
      + term_old;
  }

  double vflow_choked = G * cfarea / ther_gues->rhomass();
  return x - vflow_choked;
}

void Orifice::update_abcoef(double time, double delt, double trans_sim, double alpha_mom)
{
  if (opening == 0.0) {
    aminus = aplus = 0.0;
    return;
  }

  if (choked) {
    aplus = bplus = 0.0;
    const double delta = 0.1;
    try {
      if (!flstate) {
        flstate.reset(CoolProp::AbstractState::factory("BICUBIC&HEOS", unode->circuit->flname));
      }
      flstate->update(CoolProp::HmassP_INPUTS, upstream->tenth_gues, upstream->tpres_gues + delta);
      const auto cr2 = GcrHEM(*flstate);
      aminus = ((cr2.Gcr / cr2.rhocr) - (Gcr / rhocr)) / delta * 0.1;
      bminus = (cr2.rhocr - rhocr) / delta * 0.1;
    } catch (const CoolProp::CoolPropBaseError&) {
      aminus = bminus = 0.0;
    }
    return;
  }

  double dr = (
    1.0 / (opening * opening * Cd * Cd)
    * 2.0 * ther_gues->rhomass() * std::abs(vflow_gues)
    / (2.0 * cfarea * cfarea)
  );

  double prop_term =
    spres_gues / tpres_gues * 0.5 * ther_gues->drho_dp_consth()
    * (1.0 / (opening * opening * Cd * Cd)
       * vflow_gues * std::abs(vflow_gues) / (2.0 * cfarea * cfarea));

  aplus = (1.0 + prop_term) / dr;
  aminus = (1.0 - prop_term) / dr;

  if (aplus < 0.0 || aminus < 0.0) {
    std::cout << "warning. acoef negative "
              << identifier << " "
              << aplus << " "
              << aminus << " "
              << unode->ther_gues->rhomass() << " "
              << dnode->ther_gues->rhomass() << " "
              << dr << std::endl;
  }

  bplus = bminus = spres_gues / tpres_gues * 0.5 * ther_gues->drho_dp_consth();
  if (bplus < 0.0) {
    std::cout << spres_gues << " " << tpres_gues << " "
              << ther_gues->drho_dp_consth() << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

void Orifice::update_velocity()
{
  velocity = vflow_gues / cfarea;
}

void Orifice::update_gues()
{
  Face::update_gues();
  if (choked && rhocr > 0.0) {
    spres_gues = pcr;
    ttemp_gues = cr_ttemp;
    stemp_gues = cr_ttemp;
    ther_gues->set_state(rhocr, cr_cpmass, cr_viscosity, cr_conductivity, cr_hmass);
    ther_old->set_state(rhocr, cr_cpmass, cr_viscosity, cr_conductivity, cr_hmass);
  }
}

void Orifice::update_Gcr()
{
  upstream->update_gues();

  try {
    if (!flstate) {
      flstate.reset(CoolProp::AbstractState::factory("BICUBIC&HEOS", unode->circuit->flname));
    }
    flstate->update(CoolProp::HmassP_INPUTS, upstream->tenth_gues, upstream->tpres_gues);
    const auto cr = GcrHEM(*flstate);
    Gcr = cr.Gcr;
    pcr = cr.pcr;
    rhocr = cr.rhocr;
    cr_ttemp = cr.T;
    cr_hmass = cr.h;
    cr_cpmass = cr.cp;
    cr_viscosity = cr.mu;
    cr_conductivity = cr.k;
  } catch (const CoolProp::CoolPropBaseError&) {
    try {
      if (!flstate) {
        flstate.reset(CoolProp::AbstractState::factory("BICUBIC&HEOS", unode->circuit->flname));
      }
      flstate->update(CoolProp::PSmass_INPUTS, upstream->tpres_gues, unode->ther_gues->smass());
      const auto cr = GcrHEM(*flstate);
      Gcr = cr.Gcr;
      pcr = cr.pcr;
      rhocr = cr.rhocr;
      cr_ttemp = cr.T;
      cr_hmass = cr.h;
      cr_cpmass = cr.cp;
      cr_viscosity = cr.mu;
      cr_conductivity = cr.k;
    } catch (const CoolProp::CoolPropBaseError&) {
      if (Gcr <= 0.0) {
        Gcr = 1.0E8;
        pcr = 0.0;
        rhocr = ther_gues->rhomass();
      }
    }
  }
}

void Orifice::save_to_hdf5(hid_t group_id) const
{
  write_string(group_id, "identifier", identifier);
  write_scalar(group_id, "diameter", diameter);
  write_scalar(group_id, "Cd", Cd);
  write_scalar(group_id, "opening", opening);
  write_scalar(group_id, "cfarea", cfarea);
  write_scalar(group_id, "faceno", faceno);
  write_scalar(group_id, "ufrac", ufrac);
  write_scalar(group_id, "uheight", uheight);
  write_scalar(group_id, "dfrac", dfrac);
  write_scalar(group_id, "dheight", dheight);
  write_scalar(group_id, "delz", delz);
  write_scalar(group_id, "vflow_old", vflow_old);
  write_scalar(group_id, "vflow_gues", vflow_gues);
  write_scalar(group_id, "mflow", mflow);
  write_scalar(group_id, "velocity", velocity);
  write_scalar(group_id, "heat_input_old", heat_input_old);
  write_scalar(group_id, "heat_input", heat_input);
  write_scalar(group_id, "choked", choked ? 1.0 : 0.0);
  write_scalar(group_id, "presidue", presidue);
  write_scalar(group_id, "G", G);
  write_scalar(group_id, "Gcr", Gcr);
  write_scalar(group_id, "pcr", pcr);
  write_scalar(group_id, "rhocr", rhocr);
  write_scalar(group_id, "cr_ttemp", cr_ttemp);
  write_scalar(group_id, "cr_hmass", cr_hmass);
  write_scalar(group_id, "cr_cpmass", cr_cpmass);
  write_scalar(group_id, "cr_viscosity", cr_viscosity);
  write_scalar(group_id, "cr_conductivity", cr_conductivity);
  write_scalar(group_id, "tpres_old", tpres_old);
  write_scalar(group_id, "spres_old", spres_old);
  write_scalar(group_id, "ttemp_old", ttemp_old);
  write_scalar(group_id, "stemp_old", stemp_old);
  write_scalar(group_id, "tpres_gues", tpres_gues);
  write_scalar(group_id, "spres_gues", spres_gues);
  write_scalar(group_id, "ttemp_gues", ttemp_gues);
  write_scalar(group_id, "stemp_gues", stemp_gues);
  write_scalar(group_id, "aplus", aplus);
  write_scalar(group_id, "aminus", aminus);
  write_scalar(group_id, "bplus", bplus);
  write_scalar(group_id, "bminus", bminus);

  if (!heat_hslab.empty()) {
    write_vector(group_id, "heat_hslab", heat_hslab);
  }
  if (!heat_hslab_old.empty()) {
    write_vector(group_id, "heat_hslab_old", heat_hslab_old);
  }
}

void Orifice::load_from_hdf5(hid_t group_id)
{
  diameter = read_scalar(group_id, "diameter");
  Cd = read_scalar(group_id, "Cd");
  opening = read_scalar(group_id, "opening");
  cfarea = read_scalar(group_id, "cfarea");
  faceno = static_cast<int>(read_scalar(group_id, "faceno"));
  ufrac = read_scalar(group_id, "ufrac");
  uheight = read_scalar(group_id, "uheight");
  dfrac = read_scalar(group_id, "dfrac");
  dheight = read_scalar(group_id, "dheight");
  delz = read_scalar(group_id, "delz");
  vflow_old = read_scalar(group_id, "vflow_old");
  vflow_gues = read_scalar(group_id, "vflow_gues");
  mflow = read_scalar(group_id, "mflow");
  velocity = read_scalar(group_id, "velocity");
  heat_input_old = read_scalar(group_id, "heat_input_old");
  heat_input = read_scalar(group_id, "heat_input");
  choked = static_cast<bool>(read_scalar(group_id, "choked"));
  presidue = read_scalar(group_id, "presidue");
  G = read_scalar(group_id, "G");
  Gcr = read_scalar(group_id, "Gcr");
  pcr = read_scalar(group_id, "pcr");
  rhocr = read_scalar(group_id, "rhocr");
  cr_ttemp = read_scalar(group_id, "cr_ttemp");
  cr_hmass = read_scalar(group_id, "cr_hmass");
  cr_cpmass = read_scalar(group_id, "cr_cpmass");
  cr_viscosity = read_scalar(group_id, "cr_viscosity");
  cr_conductivity = read_scalar(group_id, "cr_conductivity");
  tpres_old = read_scalar(group_id, "tpres_old");
  spres_old = read_scalar(group_id, "spres_old");
  ttemp_old = read_scalar(group_id, "ttemp_old");
  stemp_old = read_scalar(group_id, "stemp_old");
  tpres_gues = read_scalar(group_id, "tpres_gues");
  spres_gues = read_scalar(group_id, "spres_gues");
  ttemp_gues = read_scalar(group_id, "ttemp_gues");
  stemp_gues = read_scalar(group_id, "stemp_gues");
  aplus = read_scalar(group_id, "aplus");
  aminus = read_scalar(group_id, "aminus");
  bplus = read_scalar(group_id, "bplus");
  bminus = read_scalar(group_id, "bminus");

  heat_hslab.clear();
  heat_hslab_old.clear();
  if (H5Lexists(group_id, "heat_hslab", H5P_DEFAULT) > 0) {
    heat_hslab = read_vector_double(group_id, "heat_hslab");
  }
  if (H5Lexists(group_id, "heat_hslab_old", H5P_DEFAULT) > 0) {
    heat_hslab_old = read_vector_double(group_id, "heat_hslab_old");
  }

}

} // namespace opensd
