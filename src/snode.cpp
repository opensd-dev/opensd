//! \file snode.cpp
#include "opensd/snode.h"

// #include <iostream>
// #include <cstdlib>
// #include <cmath>
// #include <stdexcept>

#include "opensd/error.h"
// #include "opensd/xml_interface.h"
#include "opensd/hslab.h"
#include "opensd/sface.h"
#include "opensd/solid.h"

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================

//==============================================================================
// Node implementation
//==============================================================================

SNode::SNode(std::string identifier)
    : identifier(identifier), heat_input(0.0),heat_input_old(0.0) {

}


// SNode class method
double SNode::eqn_ener(double time, double delt, bool trans_sim, double alpha_ener) {
  // Start: combine heat input terms
  double y = - alpha_ener * heat_input - (1.0 - alpha_ener) * heat_input_old;

  // Transient term (mass * cp * dT / dt)
  // if (trans_sim) {
  //   double cp_avg = 0.5 * (this->ther_old.cpmass() + this->ther_gues.cpmass());
  //   y += cp_avg * ( this->ther_gues.rhomass() * this->temp_gues
  //                 - this->ther_old.rhomass() * this->temp_old )
  //        * this->volume / delt;
  // }

  // EAST face conduction or boundary condition
  // if (eface != nullptr) {
  //   y = ( y
  //        - alpha_ener * eface->A * eface->ther_gues.conductivity()
  //            * ( eface->dnode->temp_gues - temp_gues ) / eface->delx
  //        - (1.0 - alpha_ener) * eface->A * eface->ther_old.conductivity()
  //            * ( eface->dnode->temp_old - temp_old ) / eface->delx );
  // }
  // else {
  //   // Downstream boundary condition depends on hslab.dvar
  //   auto hslab = this->layer->hslab;
  //   std::string dvar = hslab->dvar;
  //
  //   if (dvar == "pipe" || dvar == "vof_pipe" || dvar == "pipenl") {
  //     // indexing: node_ind - ninc*(nnodes-1)
  //     int idx = this->node_ind - this->layer->ninc * (this->layer->nnodes - 1);
  //
  //     // flow_elem assumed to be pointer type; adapt if it's a smart pointer
  //     auto* flow_elem = hslab->dval[1][idx]; // adapt types if necessary
  //
  //     double h = 0.0;
  //     if (dvar == "vof_pipe") {
  //       if (flow_elem->volfrac1 <= 1.0E-6) {
  //         h = 0.0;
  //       } else {
  //         // calc_value returns a double when called with (hslab->dval[0], flow_elem, this)
  //         h = flow_elem->volfrac1 * calc_value(hslab->dval[0], flow_elem, this)
  //             + (1.0 - flow_elem->volfrac1) * 0.0; // adiabatic second component
  //       }
  //     }
  //     else if (dvar == "pipe") {
  //       // direct call mapping
  //       h = calc_value(hslab->dval[0], flow_elem, this);
  //     }
  //     else if (dvar == "pipenl") {
  //       // pipenl: dval[0] is callable that returns (Sc, Sp, h)
  //       // adapt call depending on how you store function objects
  //       // Example: auto tup = hslab->dval[0](flow_elem,this); // returns struct or tuple
  //       // Here we assume it fills h by reference or returns via tuple-like struct
  //       double Sc = 0.0, Sp = 0.0;
  //       // If dval[0] is std::function<std::tuple<double,double,double>(FlowElem*, SNode*)>:
  //       auto tup = hslab->dval[0](flow_elem, this); // adjust to your signature
  //       Sc = std::get<0>(tup);
  //       Sp = std::get<1>(tup);
  //       h  = std::get<2>(tup);
  //     }
  //
  //     double Tf = flow_elem->stemp_gues;
  //     double hA = h * this->Ai;
  //     y = y + alpha_ener * (this->temp_gues - Tf) * hA + (1.0 - alpha_ener) * this->heat_transfer_old;
  //   }
  //   else if (dvar == "conv") {
  //     // dval[0] is h coefficient, dval[1] is reference temperature
  //     y = y + hslab->dval[0] * (this->temp_gues - hslab->dval[1]) * this->Ai * alpha_ener
  //           + this->heat_transfer_old * (1.0 - alpha_ener);
  //   }
  //   else if (dvar == "node") {
  //     // dval[1] is a node-like object with stemp_gues
  //     y = y + hslab->dval[0] * (this->temp_gues - hslab->dval[1].stemp_gues) * this->Ai * alpha_ener
  //           + this->heat_transfer_old * (1.0 - alpha_ener);
  //   }
  //   else if (dvar == "hflux") {
  //     // dval is a scalar heat flux per area
  //     y = y - hslab->dval * this->Ai * alpha_ener * this->AFF
  //           + this->heat_transfer_old * (1.0 - alpha_ener);
  //   }
  //   else {
  //     throw std::runtime_error(std::string("ht option not found. stopping: ") + dvar);
  //   }
  // } // end eface branch
  //
  // // WEST face conduction or upstream boundary
  // if (this->wface != nullptr) {
  //   y = ( y
  //        + alpha_ener * this->wface->A * this->wface->ther_gues.conductivity()
  //            * ( this->temp_gues - this->wface->unode->temp_gues ) / this->wface->delx
  //        + (1.0 - alpha_ener) * this->wface->A * this->wface->ther_old.conductivity()
  //            * ( this->temp_old - this->wface->unode->temp_old ) / this->wface->delx );
  // }
  // else {
  //   auto hslab = this->layer->hslab;
  //   std::string uvar = hslab->uvar;
  //
  //   if (uvar == "pipe" || uvar == "vof_pipe" || uvar == "pipenl") {
  //     auto* flow_elem = hslab->uval[1][ this->node_ind ]; // adapt types
  //
  //     double h = 0.0;
  //     if (uvar == "vof_pipe") {
  //       if (flow_elem->volfrac1 <= 1.0E-6) {
  //         h = 0.0;
  //       } else {
  //         h = flow_elem->volfrac1 * calc_value(hslab->uval[0], flow_elem, this)
  //             + (1.0 - flow_elem->volfrac1) * 0.0;
  //       }
  //     }
  //     else if (uvar == "pipe") {
  //       h = calc_value(hslab->uval[0], flow_elem, this);
  //     }
  //     else if (uvar == "pipenl") {
  //       auto tup = hslab->uval[0](flow_elem, this); // adjust signature
  //       // unpack as before
  //       // double Sc = std::get<0>(tup); double Sp = std::get<1>(tup);
  //       h = std::get<2>(tup);
  //     }
  //
  //     double Tf = flow_elem->stemp_gues;
  //     double hA = h * this->Ai;
  //     y = y + alpha_ener * (this->temp_gues - Tf) * hA + (1.0 - alpha_ener) * this->heat_transfer_old;
  //   }
  //   else if (uvar == "conv") {
  //     y = y + alpha_ener * hslab->uval[0] * (this->temp_gues - hslab->uval[1]) * this->Ai
  //           + (1.0 - alpha_ener) * this->heat_transfer_old;
  //   }
  //   else if (uvar == "node") {
  //     auto& flow_node = hslab->uval[1];
  //     double Tf = flow_node.stemp_gues;
  //     double Tw = this->temp_gues;
  //     double h = calc_value(hslab->uval[0], &flow_node, this); // adapt if signature different
  //     double hA = h * this->Ai;
  //     y = y + alpha_ener * (Tw - Tf) * hA + (1.0 - alpha_ener) * this->heat_transfer_old;
  //   }
  //   else if (uvar == "hflux") {
  //     y = y - alpha_ener * hslab->uval * this->Ai * this->AFF
  //           + (1.0 - alpha_ener) * this->heat_transfer_old;
  //   }
  //   else {
  //     throw std::runtime_error("ht option not found. stopping (uvar)");
  //   }
  // } // end wface branch
  //
  // // North face conduction (if present)
  // if (this->nface != nullptr) {
  //   y = y
  //       - alpha_ener * this->nface->A * this->nface->ther_gues.conductivity()
  //           * ( this->nface->dnode->temp_gues - this->temp_gues ) / this->layer->dely
  //       - (1.0 - alpha_ener) * this->nface->A * this->nface->ther_old.conductivity()
  //           * ( this->nface->dnode->temp_old - this->temp_old ) / this->layer->dely;
  // }
  //
  // // South face conduction (if present)
  // if (this->sface != nullptr) {
  //   y = y
  //       + alpha_ener * this->sface->A * this->sface->ther_gues.conductivity()
  //           * ( this->temp_gues - this->sface->unode->temp_gues ) / this->layer->dely
  //       + (1.0 - alpha_ener) * this->sface->A * this->sface->ther_old.conductivity()
  //           * ( this->temp_old - this->sface->unode->temp_old ) / this->layer->dely;
  // }

  return 0;
}

void SNode::assign_prop() {
  bool found = false;
  for (auto& fptr : model::solids) {
    if (fptr->name() == solname) {
      ther_gues = fptr->clone(); // deep copy
      ther_old  = fptr->clone();
      found = true;
      break;
    }
  }
  if (!found) fatal_error(fmt::format("Could not find solid '{}'", solname));
}

} // namespace opensd
