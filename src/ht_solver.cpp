//! \file ht_solver.cpp
#include "opensd/ht_solver.h"

// #include <algorithm>
// #include <cmath>         // For std::isinf and other math functions
// #include <iostream>
// #include <iomanip>
#include <Eigen/Dense>   // For matrix manipulations
// #include <cstdlib>
// #include "opensd/vector.h"
// #include "opensd/message_passing.h"
#include "opensd/simulation.h"
// #include "opensd/settings.h"
#include "opensd/timer.h"

#include "opensd/hslab.h"
#include "opensd/circuit.h"
// #include <numeric>     // For std::accumulate
// #include <copy>          // For std::copy in Arow and brow
// #include <petscksp.h>
// #include <petscsnes.h>
// #include <fstream>
// #include <gsl/gsl_errno.h>
// #include <gsl/gsl_roots.h>
// #include <cassert>

namespace opensd {

  namespace solid {

//==============================================================================
// Global variables
//==============================================================================

//==============================================================================
// Non-member functions
//==============================================================================

void exec_energy(double time, double delt, bool trans_sim, double alpha_ener, int main_iter) {
  
  simulation::time_solid_energy.start();

  for (auto& circuit : model::circuits) {
    for (auto& pipe : circuit->pipes) {
      for (auto& face : pipe->faces) {
        face->heat_hslab.clear();
      }
    }
    for (auto& node : circuit->nodes) {
      node->heat_hslab.clear();
    }
  }

// Assumptions:
// - Classes/types: comp::Circuit, vof_comp::Circuit, HTcomp::HSlab, Layer, Node, Face, Iface, Jface, Therm (with methods conductivity(), cpmass(), rhomass(), update(...))
// - Helper functions available in your codebase: exec_htreg(...), exec_bc(...), exec_ht(...)
// - Fields/methods used mirror your Python names (adjust if different).
// - Use -> for pointer access (user preference).

  for (auto& hslab : model::hslabs) {

  //   if (!trans_sim && HSlab->solveSS == false) continue;

  //   const double relax_spl_nb = 0.6;
  //   const double relax_nb_pd   = 0.5;
  //   const double relax_pd_spv  = 0.6;
  //
  //   // u-side (upper) regression if pipe and circuit.flag_tp
  //   if ((HSlab->uvar == "pipe" || HSlab->uvar == "pipenl") && HSlab->ucomp->circuit.flag_tp) {
  //     auto inds = exec_htreg(HSlab->uflg_spl_nb,
  //                            HSlab->uflg_nb_pd,
  //                            HSlab->uflg_pd_spv,
  //                            HSlab->uind_spl_nb,
  //                            HSlab->uind_nb_pd,
  //                            HSlab->uind_pd_spv,
  //                            HSlab->uval,
  //                            HSlab->uwnodes);
  //     // exec_htreg assumed to return a tuple/struct of three doubles/ints; adapt if different.
  //     double ind_spl_nb = inds.ind_spl_nb;
  //     double ind_nb_pd  = inds.ind_nb_pd;
  //     double ind_pd_spv = inds.ind_pd_spv;
  //
  //     HSlab->uind_spl_nb  = ind_spl_nb * relax_spl_nb + HSlab->uind_spl_nb * (1.0 - relax_spl_nb);
  //     HSlab->uind_nb_pd   = ind_nb_pd  * relax_nb_pd   + HSlab->uind_nb_pd  * (1.0 - relax_nb_pd);
  //     HSlab->uind_pd_spv  = ind_pd_spv * relax_pd_spv  + HSlab->uind_pd_spv * (1.0 - relax_pd_spv);
  //   }
  //
  //   // d-side (down) regression
  //   if ((HSlab->dvar == "pipe" || HSlab->dvar == "pipenl") && HSlab->dcomp->circuit.flag_tp) {
  //     auto inds = exec_htreg(HSlab->dflg_spl_nb,
  //                            HSlab->dflg_nb_pd,
  //                            HSlab->dflg_pd_spv,
  //                            HSlab->dind_spl_nb,
  //                            HSlab->dind_nb_pd,
  //                            HSlab->dind_pd_spv,
  //                            HSlab->dval,
  //                            HSlab->dwnodes);
  //     double ind_spl_nb = inds.ind_spl_nb;
  //     double ind_nb_pd  = inds.ind_nb_pd;
  //     double ind_pd_spv = inds.ind_pd_spv;
  //
  //     HSlab->dind_spl_nb = ind_spl_nb * relax_spl_nb + HSlab->dind_spl_nb * (1.0 - relax_spl_nb);
  //     HSlab->dind_nb_pd  = ind_nb_pd  * relax_nb_pd  + HSlab->dind_nb_pd  * (1.0 - relax_nb_pd);
  //     HSlab->dind_pd_spv = ind_pd_spv * relax_pd_spv + HSlab->dind_pd_spv * (1.0 - relax_pd_spv);
  //   }

    // Build system size n
    int n = 0;
    for (auto &layer : hslab->layers) n += layer->nnodes;
    n *= hslab->ninc;
    if (n <= 0) continue;

    Eigen::MatrixXd A = Eigen::MatrixXd::Zero(n, n);
    Eigen::VectorXd b = Eigen::VectorXd::Zero(n);

  //   int i = -1;
  //   for (auto &layer : HSlab->layers) {
  //     for (auto *node : layer->nodes) {
  //       ++i;
  //       // East face
  //       if (node->eface != nullptr) {
  //         double aE  = alpha_heat * node->eface->A * node->eface->ther_gues.conductivity() / node->eface->delx;
  //         double aE0 = (1.0 - alpha_heat) * node->eface->A * node->eface->ther_old.conductivity() / node->eface->delx;
  //         b(i) -= (node->temp_old - node->eface->dnode->temp_old) * aE0;
  //         int col = i + HSlab->ninc;
  //         if (col >= 0 && col < n) A(i, col) = -aE;
  //         A(i, i) += aE;
  //       }
  //
  //       // West face
  //       if (node->wface != nullptr) {
  //         double aW  = alpha_heat * node->wface->A * node->wface->ther_gues.conductivity() / node->wface->delx;
  //         double aW0 = (1.0 - alpha_heat) * node->wface->A * node->wface->ther_old.conductivity() / node->wface->delx;
  //         b(i) -= (node->temp_old - node->wface->unode->temp_old) * aW0;
  //         int col = i - HSlab->ninc;
  //         if (col >= 0 && col < n) A(i, col) = -aW;
  //         A(i, i) += aW;
  //       }
  //
  //       // North face (note: multiplied by 0. in Python — kept as in original)
  //       if (node->nface != nullptr) {
  //         double aN  = alpha_heat * node->nface->A * node->nface->ther_gues.conductivity() / layer->dely * 0.0;
  //         double aN0 = (1.0 - alpha_heat) * node->nface->A * node->nface->ther_old.conductivity() / layer->dely * 0.0;
  //         b(i) -= (node->temp_old - node->nface->dnode->temp_old) * aN0;
  //         int col = i + 1;
  //         if (col >= 0 && col < n) A(i, col) = -aN;
  //         A(i, i) += aN;
  //       }
  //
  //       // South face (also multiplied by 0.)
  //       if (node->sface != nullptr) {
  //         double aS  = alpha_heat * node->sface->A * node->sface->ther_gues.conductivity() / layer->dely * 0.0;
  //         double aS0 = (1.0 - alpha_heat) * node->sface->A * node->sface->ther_old.conductivity() / layer->dely * 0.0;
  //         b(i) -= (node->temp_old - node->sface->unode->temp_old) * aS0;
  //         int col = i - 1;
  //         if (col >= 0 && col < n) A(i, col) = -aS;
  //         A(i, i) += aS;
  //       }
  //
  //       // Heat input and transient contribution
  //       b(i) += alpha_heat * node->heat_input + (1.0 - alpha_heat) * node->heat_input_old;
  //       if (trans_sim) {
  //         b(i) += node->ther_old.cpmass() * node->ther_old.rhomass() * node->volume / delt * node->temp_old;
  //         A(i, i) += node->ther_gues.cpmass() * node->ther_gues.rhomass() * node->volume / delt;
  //       }
  //
  //       // Boundary conditions (uwnodes)
  //       if (std::find(HSlab->uwnodes.begin(), HSlab->uwnodes.end(), node) != HSlab->uwnodes.end()) {
  //         // exec_bc returns pair (Ainc,binc) - adapt to your implementation
  //         auto A_b = exec_bc(HSlab->uvar, HSlab->uval, node->Ai, node, i);
  //         double Ainc = A_b.Ainc;
  //         double binc = A_b.binc;
  //         b(i) += alpha_heat * binc - (1.0 - alpha_heat) * node->heat_transfer_old;
  //         A(i, i) += alpha_heat * Ainc;
  //       }
  //
  //       // Boundary conditions (dwnodes) - note Python used i-n+HSlab.ninc index for bc call
  //       if (std::find(HSlab->dwnodes.begin(), HSlab->dwnodes.end(), node) != HSlab->dwnodes.end()) {
  //         int bc_index = i - n + HSlab->ninc; // as in Python
  //         auto A_b = exec_bc(HSlab->dvar, HSlab->dval, node->Ai, node, bc_index);
  //         double Ainc = A_b.Ainc;
  //         double binc = A_b.binc;
  //         b(i) += alpha_heat * binc - (1.0 - alpha_heat) * node->heat_transfer_old;
  //         A(i, i) += alpha_heat * Ainc;
  //       }
  //     }
  //   } // end building A, b
  //
  //   // Solve linear system A * temp = b
  //   Eigen::VectorXd tempVec(n);
  //   bool solved = true;
  //   if (A.cols() == 0 || A.rows() == 0) {
  //     solved = false;
  //   } else {
  //     // Try LDLT (symmetric PD) first, fallback to full-pivot QR if it fails.
  //     Eigen::LDLT<Eigen::MatrixXd> ldlt(A);
  //     if (ldlt.info() == Eigen::Success) {
  //       tempVec = ldlt.solve(b);
  //     } else {
  //       tempVec = A.colPivHouseholderQr().solve(b);
  //     }
  //   }
  //
  //   // Assign temp_gues back to nodes (with relaxation where applied)
  //   i = -1;
  //   for (auto &layer : HSlab->layers) {
  //     double relax = 1.0;
  //     // The original python toggles relax for some cases — kept simple here:
  //     if (!trans_sim) relax = 1.0;
  //
  //     for (auto *node : layer->nodes) {
  //       ++i;
  //       if (std::find(HSlab->dwnodes.begin(), HSlab->dwnodes.end(), node) != HSlab->dwnodes.end() ||
  //           std::find(HSlab->uwnodes.begin(), HSlab->uwnodes.end(), node) != HSlab->uwnodes.end()) {
  //         node->temp_gues = relax * tempVec(i) + (1.0 - relax) * node->temp_gues;
  //       } else {
  //         node->temp_gues = tempVec(i);
  //       }
  //     }
  //   }
  } // end first pass over HSlabs

  // // Second pass: compute heat transfers and update thermo states
  // for (auto *HSlab : HTcomp::HSlab::_registry) {
  //   int n = 0;
  //   for (auto &layer : HSlab->layers) n += layer->nnodes;
  //   n *= HSlab->ninc;
  //   if (!trans_sim && HSlab->solveSS == false) continue;
  //
  //   HSlab->dheat_transfer = 0.0;
  //   HSlab->uheat_transfer = 0.0;
  //   int i = -1;
  //
  //   for (auto &layer : HSlab->layers) {
  //     for (auto *node : layer->nodes) {
  //       ++i;
  //       if (std::find(HSlab->dwnodes.begin(), HSlab->dwnodes.end(), node) != HSlab->dwnodes.end()) {
  //         node->heat_transfer = exec_ht(HSlab->dvar, HSlab->dval, node->Ai, node, i - n + HSlab->ninc);
  //         HSlab->dheat_transfer += node->heat_transfer;
  //       }
  //
  //       if (std::find(HSlab->uwnodes.begin(), HSlab->uwnodes.end(), node) != HSlab->uwnodes.end()) {
  //         node->heat_transfer = exec_ht(HSlab->uvar, HSlab->uval, node->Ai, node, i);
  //         HSlab->uheat_transfer += node->heat_transfer;
  //       }
  //
  //       // update guessed thermodynamic state for node
  //       node->ther_gues.update(node->temp_gues);
  //     } // end nodes in layer
  //
  //     // update interfaces in this layer
  //     for (auto *iface : layer->ifaces) {
  //       iface->ther_gues.update();
  //       iface->update_temp();
  //     }
  //     for (auto *jface : layer->jfaces) {
  //       jface->ther_gues.update();
  //       jface->update_temp();
  //     }
  //   } // end layers loop
  //
  //   // final per-node updates for condensate efficiency and heat input
  //   for (auto &layer : HSlab->layers) {
  //     for (auto *node : layer->nodes) {
  //       node->update_condeff();
  //       node->update_heat_input(time, delt);
  //     }
  //   }
  // } // end second pass over HSlabs




  simulation::time_solid_energy.stop();
  
}
  }
}
