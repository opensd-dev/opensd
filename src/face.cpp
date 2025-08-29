//! \file face.cpp
#include "opensd/face.h"

#include <iostream>
#include <cmath>
#include <cstdlib>

#include "opensd/circuit.h"
#include "opensd/constants.h"
#include "opensd/hdf5_interface.h"
#include "opensd/node.h"

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================

//==============================================================================
// Face implementation
//==============================================================================

Face::Face(int faceno, std::shared_ptr<Node> unode, double ufrac, std::shared_ptr<Node> dnode, double dfrac)
    : faceno(faceno), unode(unode), ufrac(ufrac), dnode(dnode), dfrac(dfrac),
      vflow_old{1.0E-8}, vflow_gues{1.0E-8}, mflow(0.0), velocity(0.0), choked(false), 
	  presidue(0.0), Gcr(1.0E8), pcr(0.0),heat_input_old(0.0), heat_input(0.0),
	  heat_hslab_old{}, owner(0) {
  // if (ufrac != nullptr && typeid(*unode) == typeid(Reservoir)) {
    // uheight = ufrac * unode->height;
  // }
  // if (dfrac != nullptr && typeid(*dnode) == typeid(Reservoir)) {
    // dheight = dfrac * dnode->height;
  // }
}

void Face::update_statevar(){
  tpres_gues = 0.5 * (unode->tpres_gues + dnode->tpres_gues);
  spres_gues = 0.5 * (unode->spres_gues + dnode->spres_gues);
  ttemp_gues = 0.5 * (unode->ttemp_gues + dnode->ttemp_gues);
  stemp_gues = 0.5 * (unode->stemp_gues + dnode->stemp_gues);
}

void Face::assign_statevar() {
  tpres_old = 0.5 * (unode->tpres_old + dnode->tpres_old);
  spres_old = 0.5 * (unode->spres_old + dnode->spres_old);
  ttemp_old = 0.5 * (unode->ttemp_old + dnode->ttemp_old);
  stemp_old = 0.5 * (unode->stemp_old + dnode->stemp_old);
}

void Face::update_gues() {
  vflow_gues = vflow_old;
  tpres_gues = tpres_old;
  spres_gues = spres_old;
  ttemp_gues = ttemp_old;
  stemp_gues = stemp_old;
  // if self.choked:
    // ther_gues.update(self.ther_old)
  // else:
    ther_gues->update();
  upstream->update_gues();
  downstream->update_gues();
}

void Face::assign_prop() {
  // if (dfrac != ufrac && dfrac != nullptr && ufrac != nullptr) {
    // std::cerr << "face properties not defined. stopping " << pipe.identifier << " face" << faceno << std::endl;
    // std::exit(EXIT_FAILURE);
  // }

  // ther_old = new FaceTher(unode->ther_old, dnode->ther_old, this, flag_tp, flowreg, unode, dnode);
  // ther_gues = new FaceTher(unode->ther_gues, dnode->ther_gues, this, flag_tp, flowreg, unode, dnode);
  ther_old = new FaceTher(this);
  ther_gues = new FaceTher(this);


  // if (circuit->fllib == "CoolProp") {
    // ther_cr = CoolProp::AbstractState::factory("BICUBIC&HEOS", circuit->flname);
  // } else if (circuit->fllib == "thiravam") {
    // ther_cr = thiravam::state(circuit->flname);
  // } else if (circuit->fllib == "User") {
    // auto mod = __import__(circuit->flname);
    // auto clas = getattr(mod, "fluid");
    // ther_cr = clas();
  // }

  // if (dynamic_cast<Reservoir*>(unode) && ufrac != nullptr) {
    // upstream = new Connection(unode, ufrac, uheight);
  // } else {
    upstream = new Connection(unode, ufrac, 0);
  // }
  upstream->update_old();

  // if (dynamic_cast<Reservoir*>(dnode) && dfrac != nullptr) {
    // downstream = new Connection(dnode, dfrac, dheight);
  // } else {
    downstream = new Connection(dnode, dfrac, 0);
  // }
  downstream->update_old();
  aplus = aminus = bplus = bminus = 0.;
  // if (choked) {
    // auto flstate = circuit->flstate;
    // flstate.update(CoolProp::PSmass_INPUTS, spres_gues, s0);
    // ther_old->update(flstate);
  // } else {
    ther_old->update();
  // }
}

void Face::update_old() {
    vflow_old = vflow_gues;
    tpres_old = tpres_gues;
    spres_old = spres_gues;
    ttemp_old = ttemp_gues;
    stemp_old = stemp_gues;

    // if (choked) {
    //   ther_old.update(ther_gues);
    // } else {
      ther_old->update();
    // }

    // heat_hslab_old = heat_hslab;
    // heat_input_old = heat_input;
    //
    // if (upstream) upstream->update_old();
    // if (downstream) downstream->update_old();
}


//==============================================================================
// PFace implementation
//==============================================================================

// PFace::PFace(pugi::xml_node pface_xnode)
// {
  // if (check_for_node(pface_xnode, "faceno")) {
    // faceno = get_node_value(pface_xnode, "faceno");

  // } else {
    // fatal_error("Must specify faceno of flow face in geometry XML file.");
  // }
  // mresidue = 0.;
  // mflow_in = 1.E-4;
  // volume = 0.;
// }

PFace::PFace(int faceno, std::shared_ptr<Pipe> pipe, std::shared_ptr<Node> unode, double ufrac, std::shared_ptr<Node> dnode, double dfrac,
            double diameter, double cfarea, double delx, double delz, double fricopt, double roughness)
  : Face(faceno, unode, ufrac, dnode, dfrac),
    circuit(pipe->circuit), pipe(pipe), diameter(diameter), cfarea(cfarea),
    delx(delx), delz(delz), roughness(roughness), Re(0.0), fricopt(fricopt),
    fricfact_old(64.0), fricfact_gues(64.0), opening(1.0) {
}

double PFace::eqn_mom(double x, double time, double delt, bool trans_sim, double alpha_mom) {
  double delp_fr = fricfact_gues * delx * ther_gues->rhomass() * x * std::abs(x) / (2. * diameter * cfarea * cfarea);
/*  if (faceno == 0) {
    delp_fr += pipe.Kforward * ther_gues.rhomass() * x * std::abs(x) / (2. * cfarea * cfarea);
  }
*/
  double delp_gr = ther_gues->rhomass() * grav * delz; 
  double term_old = ((1. - alpha_mom) * ((dnode->tpres_gues - unode->tpres_gues) //downstream.tpres_old - upstream.tpres_old
                    - vflow_old * vflow_old / (2. * cfarea * cfarea) * 0. //(downstream.rhomass_old - upstream.rhomass_old)
                    + ther_old->rhomass() * grav * delz
                    + fricfact_old * delx * ther_old->rhomass() * vflow_old * std::abs(vflow_old) / (2. * diameter * cfarea * cfarea)));

/*
  if (faceno == 0) {
    Term_old += (1. - alpha_mom) * pipe.Kforward_old * ther_old.rhomass() * vflow_old * std::abs(vflow_old) / (2. * cfarea * cfarea);
  }
*/

  double y = (trans_sim * delx * ther_gues->rhomass() * (x - vflow_old) / (delt * cfarea)
            + alpha_mom * (dnode->tpres_gues - unode->tpres_gues //downstream.tpres_gues - upstream.tpres_gues 
                          - vflow_gues * vflow_gues / (2. * cfarea * cfarea) * 0. //(downstream.rhomass_gues - upstream.rhomass_gues) 
                          + delp_gr 
                          + delp_fr) 
            + term_old);
  return y;
}


void PFace::update_abcoef(double time, double delt, double trans_sim, double alpha_mom) {
  if (!choked) {
    double A = 0.;
    // if (dnode.ther_gues.phase() == 6) {
      // A = pow(vflow_gues, 2) / (2 * pow(cfarea, 2)) * dnode.spres_gues / dnode.tpres_gues * dnode.ther_gues.first_two_phase_deriv(1, 2, 3); // replace iDmass, iP, iHmass with appropriate values
    // } else {
      // A = pow(vflow_gues, 2) / (2 * pow(cfarea, 2)) * dnode.spres_gues / dnode.tpres_gues * dnode.ther_gues.first_partial_deriv(1, 2, 3); // replace iDmass, iP, iHmass with appropriate values
    // }

    double dr;
    dr = (trans_sim * ther_gues->rhomass() * delx / (cfarea * delt)
                 + alpha_mom * (2 * (fricfact_gues * delx / diameter) * ther_gues->rhomass() * fabs(vflow_gues) / (2 * pow(cfarea, 2))
                                + 0.0 * 2.0 * vflow_gues * (unode->ther_gues->rhomass() - dnode->ther_gues->rhomass()) / (2 * pow(cfarea, 2))));

    if (faceno == 0) {
      dr += alpha_mom * 2.0 * pipe->Kforward * ther_gues->rhomass() * fabs(vflow_gues) / (2 * pow(cfarea, 2));
    }

    aplus = ((alpha_mom * (1.0 - A * 0.0)
              + (spres_gues / tpres_gues * delx * 0.5 * ther_gues->drho_dp_consth() *
                 (trans_sim * (vflow_gues - vflow_old) / (cfarea * delt) + 0.0 * alpha_mom * 9.81 * delz / delx + alpha_mom * (fricfact_gues / diameter) * vflow_gues * fabs(vflow_gues) / (2 * pow(cfarea, 2)))))
             / dr);

    // if (faceno == 0) {
      // aplus += (spres_gues / tpres_gues * 0.5 * ther_gues.drho_dp_consth() *
                // alpha_mom * pipe.Kforward * vflow_gues * fabs(vflow_gues) / (2 * pow(cfarea, 2))) / dr;
    // }

    double B = 0.;
    // if (unode.ther_gues.phase() == 6) {
      // B = pow(vflow_gues, 2) / (2 * pow(cfarea, 2)) * unode.spres_gues / unode.tpres_gues * unode.ther_gues.first_two_phase_deriv(1, 2, 3); // replace iDmass, iP, iHmass with appropriate values
    // } else {
      // B = pow(vflow_gues, 2) / (2 * pow(cfarea, 2)) * unode.spres_gues / unode.tpres_gues * unode.ther_gues.first_partial_deriv(1, 2, 3); // replace iDmass, iP, iHmass with appropriate values
    // }

    aminus = ((alpha_mom * (1.0 - B * 0.0)
               - (spres_gues / tpres_gues * delx * 0.5 * ther_gues->drho_dp_consth() *
                  (trans_sim * (vflow_gues - vflow_old) / (cfarea * delt) + 0.0 * alpha_mom * 9.81 * delz / delx + alpha_mom * fricfact_gues * vflow_gues * fabs(vflow_gues) / (2 * diameter * pow(cfarea, 2)))))
              / dr);
    
    // if (faceno == 0) {
      // aminus -= (spres_gues / tpres_gues * 0.5 * dnode.ther_gues.drho_dp_consth() *
                 // alpha_mom * pipe.Kforward * vflow_gues * fabs(vflow_gues) / (2 * pow(cfarea, 2))) / dr;
    // }

    bplus = bminus = spres_gues / tpres_gues * 0.5 * ther_gues->drho_dp_consth();

    // if (aplus < 0.0 || aminus < 0.0) {
      // if ((true && trans_sim) || (!trans_sim)) { // replace true with appropriate condition if `show_warn` is a variable
        // std::cout << "warning. acoef negative." << pipe.identifier << faceno << aplus << aminus << unode.ther_gues.rhomass() << dnode.ther_gues.rhomass() << A << B << dr << vflow_gues << std::endl;
      // }
    // }

    // if (bplus < 0) {
      // std::cout << spres_gues << tpres_gues << dnode.ther_gues.drho_dp_consth() << std::endl;
      // std::exit(EXIT_FAILURE);
    // }
  } else {
/*     double delta = 0.1;
    double y1 = Gcr / rhocr;
    auto& flstate = circuit.flstate;
    flstate.update(1, 1, 1 + delta); // replace HmassP_INPUTS, upstream.tenth_gues, upstream.tpres_gues with appropriate values
    double Gcr2, pcr2, rhocr2;
    std::tie(Gcr2, pcr2, rhocr2) = GcrHEM(flstate); // replace with appropriate function call
    double y2 = Gcr2 / rhocr2;
    aminus = (y2 - y1) / delta * 0.1;
    bminus = (rhocr2 - rhocr) / delta * 0.1;
 */  }
}

void PFace::update_old() {
  Face::update_old();
  fricfact_old = fricfact_gues;
  pipe->Kforward_old = pipe->Kforward;
}

void PFace::update_gues() {
  Face::update_gues();
  fricfact_gues = fricfact_old;
}

void PFace::update_velocity() {
  velocity = vflow_gues / cfarea;
}

void PFace::update_fricfact() {
  fricfact_gues = 0.02;
  // if (fricopt == "HW") {
    // fricfact_gues = 10.78 * M_PI * M_PI * constants::grav / 8.0 * std::pow(diameter, 0.13) /
                    // (std::pow(roughness, 1.852) * std::pow(std::abs(vflow_gues), 0.148));
  // } 
  // else if (fricopt == "DW") {
    // update_Re();
    // if (Re < 1.0E-6) {
      // fricfact_gues = 64.0 / 1.0E-6;
    // } 
    // else if (Re < 2300.0) {
      // fricfact_gues = 64.0 / Re;
    // } 
    // else if (Re > 5000.0) {
      // fricfact_gues = 0.25 / std::pow(0.434294 * std::log(roughness / (3.7 * diameter) + 5.74 / std::pow(Re, 0.9)), 2);
    // } 
    // else {
      // double f1 = 64.0 / 2300.0;
      // double f2 = 0.25 / std::pow(0.434294 * std::log(roughness / (3.7 * diameter) + 5.74 / std::pow(5000.0, 0.9)), 2);
      // fricfact_gues = (Re - 2300.0) * (f2 - f1) / (5000.0 - 2300.0) + f1;
    // }
  // } 
  // else if (fricopt == "BL") {
    // update_Re();
    // if (Re < 1.0E-6) {
      // fricfact_gues = 64.0 / 1.0E-6;
    // } 
    // else if (Re < 2300.0) {
      // fricfact_gues = 64.0 / Re;
    // } 
    // else if (Re > 5000.0) {
      // fricfact_gues = 0.316 / std::pow(Re, 0.25);
    // } 
    // else {
      // double f1 = 64.0 / 2300.0;
      // double f2 = 0.316 / std::pow(5000.0, 0.25);
      // fricfact_gues = (Re - 2300.0) * (f2 - f1) / (5000.0 - 2300.0) + f1;
    // }
  // } 
  // else if (std::holds_alternative<double>(fricopt)) {
    // fricfact_gues = std::get<double>(fricopt);
  // } 
  // else if (std::holds_alternative<std::function<double(double)>>(fricopt)) {
    // update_Re();
    // auto func = std::get<std::function<double(double)>>(fricopt);
    // fricfact_gues = func(Re);
  // } 
  // else {
    // std::cerr << "Friction factor not defined. Stopping." << std::endl;
    // std::exit(EXIT_FAILURE);
  // }
}


void PFace::save_to_hdf5(hid_t group_id) const {
  write_scalar(group_id, "faceno", faceno);
  write_string(group_id, "unode", unode ? unode->identifier : "");
  write_scalar(group_id, "ufrac", ufrac);
  write_scalar(group_id, "uheight", uheight);
  write_string(group_id, "dnode", dnode ? dnode->identifier : "");
  write_scalar(group_id, "dfrac", dfrac);
  write_scalar(group_id, "dheight", dheight);

  write_scalar(group_id, "vflow_old", vflow_old);
  write_scalar(group_id, "vflow_gues", vflow_gues);
  write_scalar(group_id, "mflow", mflow);
  write_scalar(group_id, "velocity", velocity);
  write_scalar(group_id, "heat_input_old", heat_input_old);
  write_scalar(group_id, "heat_input", heat_input);

  // Vectors
  // write_vector(group_id, "heat_hslab", heat_hslab);
  // write_vector(group_id, "heat_hslab_old", heat_hslab_old);

  write_scalar(group_id, "choked", choked ? 1.0 : 0.0);
  write_scalar(group_id, "presidue", presidue);
  write_scalar(group_id, "Gcr", Gcr);
  write_scalar(group_id, "pcr", pcr);

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
  write_scalar(group_id, "bminus", bminus);
  write_scalar(group_id, "fricfact_gues", fricfact_gues);
}

void PFace::load_from_hdf5(hid_t group_id) {
  faceno = static_cast<int>(read_scalar(group_id, "faceno"));
  ufrac = read_scalar(group_id, "ufrac");
  uheight = read_scalar(group_id, "uheight");
  dfrac = read_scalar(group_id, "dfrac");
  dheight = read_scalar(group_id, "dheight");

  vflow_old = read_scalar(group_id, "vflow_gues");
  vflow_gues = read_scalar(group_id, "vflow_gues");
  mflow = read_scalar(group_id, "mflow");
  velocity = read_scalar(group_id, "velocity");

  heat_input_old = read_scalar(group_id, "heat_input_old");
  heat_input = read_scalar(group_id, "heat_input");

  // heat_hslab = read_vector_double(group_id, "heat_hslab");
  // heat_hslab_old = read_vector_double(group_id, "heat_hslab_old");

  choked = static_cast<bool>(read_scalar(group_id, "choked"));
  presidue = read_scalar(group_id, "presidue");
  Gcr = read_scalar(group_id, "Gcr");
  pcr = read_scalar(group_id, "pcr");

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
  fricfact_old = read_scalar(group_id, "fricfact_gues");
}


} // namespace opensd
