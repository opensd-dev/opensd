//! \file sface.cpp
#include "opensd/sface.h"

// #include <iostream>
// #include <cmath>
// #include <cstdlib>
//
// #include "opensd/circuit.h"
#include "opensd/layer.h"
// #include "opensd/constants.h"
// #include "opensd/hdf5_interface.h"
// #include "opensd/node.h"

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================

//==============================================================================
// Face implementation
//==============================================================================

SFace::SFace(std::string identifier, std::shared_ptr<SNode> unode, std::shared_ptr<SNode> dnode, double A)
    : identifier(identifier), unode(unode), dnode(dnode), A(A) {

  // ther_old = new FaceTher(this);
  ther_gues = new SFaceTher(this);

  double delx1 = unode->layer->delx;
  double delx2 = dnode->layer->delx;
  delx = 0.5*(delx1+delx2);


}

// void Face::update_statevar(){
//   tpres_gues = 0.5 * (unode->tpres_gues + dnode->tpres_gues);
//   spres_gues = 0.5 * (unode->spres_gues + dnode->spres_gues);
//   ttemp_gues = 0.5 * (unode->ttemp_gues + dnode->ttemp_gues);
//   stemp_gues = 0.5 * (unode->stemp_gues + dnode->stemp_gues);
// }
//
// void Face::update_staticpres() {
//   spres_gues = 0.5 * (unode->spres_gues + dnode->spres_gues);
// }
//
// void Face::assign_statevar() {
//   tpres_old = 0.5 * (unode->tpres_old + dnode->tpres_old);
//   spres_old = 0.5 * (unode->spres_old + dnode->spres_old);
//   ttemp_old = 0.5 * (unode->ttemp_old + dnode->ttemp_old);
//   stemp_old = 0.5 * (unode->stemp_old + dnode->stemp_old);
// }
//
// void Face::update_gues() {
//   vflow_gues = vflow_old;
//   tpres_gues = tpres_old;
//   spres_gues = spres_old;
//   ttemp_gues = ttemp_old;
//   stemp_gues = stemp_old;
//   // if self.choked:
//     // ther_gues.update(self.ther_old)
//   // else:
//     ther_gues->update();
//   upstream->update_gues();
//   downstream->update_gues();
// }
//
// void Face::assign_prop() {
//   // if (dfrac != ufrac && dfrac != nullptr && ufrac != nullptr) {
//     // std::cerr << "face properties not defined. stopping " << pipe.identifier << " face" << faceno << std::endl;
//     // std::exit(EXIT_FAILURE);
//   // }
//
//   // ther_old = new FaceTher(unode->ther_old, dnode->ther_old, this, flag_tp, flowreg, unode, dnode);
//   // ther_gues = new FaceTher(unode->ther_gues, dnode->ther_gues, this, flag_tp, flowreg, unode, dnode);
//   ther_old = new FaceTher(this);
//   ther_gues = new FaceTher(this);
//
//
//   // if (circuit->fllib == "CoolProp") {
//     // ther_cr = CoolProp::AbstractState::factory("BICUBIC&HEOS", circuit->flname);
//   // } else if (circuit->fllib == "thiravam") {
//     // ther_cr = thiravam::state(circuit->flname);
//   // } else if (circuit->fllib == "User") {
//     // auto mod = __import__(circuit->flname);
//     // auto clas = getattr(mod, "fluid");
//     // ther_cr = clas();
//   // }
//
//   // if (dynamic_cast<Reservoir*>(unode) && ufrac != nullptr) {
//     // upstream = new Connection(unode, ufrac, uheight);
//   // } else {
//     upstream = new Connection(unode, ufrac, 0);
//   // }
//   upstream->update_old();
//
//   // if (dynamic_cast<Reservoir*>(dnode) && dfrac != nullptr) {
//     // downstream = new Connection(dnode, dfrac, dheight);
//   // } else {
//     downstream = new Connection(dnode, dfrac, 0);
//   // }
//   downstream->update_old();
//   aplus = aminus = bplus = bminus = 0.;
//   // if (choked) {
//     // auto flstate = circuit->flstate;
//     // flstate.update(CoolProp::PSmass_INPUTS, spres_gues, s0);
//     // ther_old->update(flstate);
//   // } else {
//     ther_old->update();
//   // }
// }
//
// void Face::update_old() {
//     vflow_old = vflow_gues;
//     tpres_old = tpres_gues;
//     spres_old = spres_gues;
//     ttemp_old = ttemp_gues;
//     stemp_old = stemp_gues;
//
//     // if (choked) {
//     //   ther_old.update(ther_gues);
//     // } else {
//       ther_old->update();
//     // }
//
//     // heat_hslab_old = heat_hslab;
//     heat_input_old = heat_input;
//     //
//     // if (upstream) upstream->update_old();
//     // if (downstream) downstream->update_old();
// }


} // namespace opensd
