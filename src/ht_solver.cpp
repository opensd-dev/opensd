//! \file ht_solver.cpp
#include "opensd/ht_solver.h"

// #include <algorithm>
// #include <cmath>         // For std::isinf and other math functions
// #include <iostream>
// #include <iomanip>
#include <Eigen/Dense>   // For matrix manipulations
#include <cstdlib>
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
// #include <stdexcept>

namespace opensd {

  namespace solid {

//==============================================================================
// Global variables
//==============================================================================

//==============================================================================
// Non-member functions
//==============================================================================

void exec_energy(double time, double delt, bool trans_sim, double alpha_heat, int main_iter) {
  
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

	alpha_heat = 1.0;
	
    int i = -1;
    for (auto &layer : hslab->layers) {
      for (auto &node : layer->snodes) {
        ++i;
        // East face
        if (node->eface != nullptr) {
          double aE  = alpha_heat * node->eface->A * node->eface->ther_gues->conductivity() / node->eface->delx;
          // double aE0 = (1.0 - alpha_heat) * node->eface->A * node->eface->ther_old.conductivity() / node->eface->delx;
          // b(i) -= (node->temp_old - node->eface->dnode->temp_old) * aE0;
          int col = i + hslab->ninc;
          if (col >= 0 && col < n) A(i, col) = -aE;
          A(i, i) = A(i, i) + aE;
        }
  
        // West face
        if (node->wface != nullptr) {
          double aW  = alpha_heat * node->wface->A * node->wface->ther_gues->conductivity() / node->wface->delx;
          // double aW0 = (1.0 - alpha_heat) * node->wface->A * node->wface->ther_old.conductivity() / node->wface->delx;
          // b(i) -= (node->temp_old - node->wface->unode->temp_old) * aW0;
          int col = i - hslab->ninc;
          if (col >= 0 && col < n) A(i, col) = -aW;
          A(i, i) = A(i, i) + aW;
        }
  
/*         // North face (note: multiplied by 0. in Python — kept as in original)
        if (node->nface != nullptr) {
          double aN  = alpha_heat * node->nface->A * node->nface->ther_gues->conductivity() / layer->dely * 0.0;
          // double aN0 = (1.0 - alpha_heat) * node->nface->A * node->nface->ther_old.conductivity() / layer->dely * 0.0;
          // b(i) -= (node->temp_old - node->nface->dnode->temp_old) * aN0;
          int col = i + 1;
          if (col >= 0 && col < n) A(i, col) = -aN;
          A(i, i) = A(i, i) + aN;
        }
  
        // South face (also multiplied by 0.)
        if (node->sface != nullptr) {
          double aS  = alpha_heat * node->sface->A * node->sface->ther_gues->conductivity() / layer->dely * 0.0;
          // double aS0 = (1.0 - alpha_heat) * node->sface->A * node->sface->ther_old.conductivity() / layer->dely * 0.0;
          // b(i) -= (node->temp_old - node->sface->unode->temp_old) * aS0;
          int col = i - 1;
          if (col >= 0 && col < n) A(i, col) = -aS;
          A(i, i) = A(i, i) + aS;
        }
 */  

        // Heat input and transient contribution
        // b(i) = b(i) + alpha_heat * node->heat_input + (1.0 - alpha_heat) * node->heat_input_old;
        // if (trans_sim) {
          // b(i) = b(i) + node->ther_old.cpmass() * node->ther_old.rhomass() * node->volume / delt * node->temp_old;
          // A(i, i) = A(i, i) + node->ther_gues.cpmass() * node->ther_gues.rhomass() * node->volume / delt;
        // }
  
        // Boundary conditions (uwnodes)
        if (std::find(hslab->uwnodes.begin(), hslab->uwnodes.end(), node) != hslab->uwnodes.end()) {
          auto [Ainc, binc] = exec_bc(hslab->uvar, hslab->uval, hslab->uval1, node->Ai, node, i); // exec_bc returns pair (Ainc,binc) - adapt to your implementation
          b(i) = b(i) + alpha_heat * binc; // - (1.0 - alpha_heat) * node->heat_transfer_old;
          A(i, i) = A(i, i) + alpha_heat * Ainc;
        }

        // Boundary conditions (dwnodes) - note Python used i-n+HSlab.ninc index for bc call
        if (std::find(hslab->dwnodes.begin(), hslab->dwnodes.end(), node) != hslab->dwnodes.end()) {
          int bc_index = i - n + hslab->ninc; // as in Python
          auto [Ainc, binc] = exec_bc(hslab->dvar, hslab->dval, hslab->dval1, node->Ai, node, bc_index);
          b(i) = b(i) + alpha_heat * binc; // - (1.0 - alpha_heat) * node->heat_transfer_old;
          A(i, i) = A(i, i) + alpha_heat * Ainc;
        }
      }
    } // end building A, b

    // Solve linear system A * temp = b
    Eigen::VectorXd tempVec(n);
    bool solved = true;
    if (A.cols() == 0 || A.rows() == 0) {
      solved = false;
    } else {
      // Try LDLT (symmetric PD) first, fallback to full-pivot QR if it fails.
      Eigen::LDLT<Eigen::MatrixXd> ldlt(A);
      if (ldlt.info() == Eigen::Success) {
        tempVec = ldlt.solve(b);
      } else {
        tempVec = A.colPivHouseholderQr().solve(b);
      }
    }
    // std::cout<<A<<std::endl<<std::endl;
    // std::cout<<b<<std::endl<<std::endl;
    // std::cout<<tempVec<<std::endl;

    // Assign temp_gues back to nodes (with relaxation where applied)
    i = -1;
    for (auto& layer : hslab->layers) {
      double relax = 1.0;
       // The original python toggles relax for some cases — kept simple here:
      if (!trans_sim) relax = 1.0;
      for (auto& snode : layer->snodes) {
        ++i;
        if (std::find(hslab->dwnodes.begin(), hslab->dwnodes.end(), snode) != hslab->dwnodes.end() ||
            std::find(hslab->uwnodes.begin(), hslab->uwnodes.end(), snode) != hslab->uwnodes.end()) {
          snode->temp_gues = relax * tempVec(i) + (1.0 - relax) * snode->temp_gues;
        } else {
          snode->temp_gues = tempVec(i);
        }
        // std::cout<<snode->identifier<< " " << snode->temp_gues<<std::endl;
      }
    }
  } // end first pass over HSlabs

  // Second pass: compute heat transfers and update thermo states
  for (auto& hslab : model::hslabs) {
    int n = 0;
    for (auto& layer : hslab->layers) n += layer->nnodes;
    n *= hslab->ninc;
    // if (!trans_sim && hslab->solveSS == false) continue;

    hslab->dheat_transfer = 0.0;
    hslab->uheat_transfer = 0.0;
    int i = -1;

    for (auto& layer : hslab->layers) {
      for (auto& snode : layer->snodes) {
        ++i;
        if (std::find(hslab->dwnodes.begin(), hslab->dwnodes.end(), snode) != hslab->dwnodes.end()) {
          snode->heat_transfer = exec_ht(hslab->dvar, hslab->dval, hslab->dval1, snode->Ai, snode, i - n + hslab->ninc);
          hslab->dheat_transfer += snode->heat_transfer;
        }

        if (std::find(hslab->uwnodes.begin(), hslab->uwnodes.end(), snode) != hslab->uwnodes.end()) {
          snode->heat_transfer = exec_ht(hslab->uvar, hslab->uval, hslab->uval1, snode->Ai, snode, i);
          hslab->uheat_transfer += snode->heat_transfer;
        }

        // update guessed thermodynamic state for node
        snode->ther_gues->update(snode->temp_gues);
      } // end nodes in layer
      // update interfaces in this layer
      for (auto iface : layer->ifaces) {
        iface->ther_gues->update();
        iface->update_temp();
        // std::cout << iface->temp_gues<<std::endl;
      }
      for (auto jface : layer->jfaces) {
        jface->ther_gues->update();
        jface->update_temp();
        // std::cout << jface->temp_gues<<std::endl;
      }
    } // end layers loop

    // final per-node updates for condensate efficiency and heat input
    for (auto& layer : hslab->layers) {
      for (auto& snode : layer->snodes) {
  //       node->update_condeff();
  //       node->update_heat_input(time, delt);
      }
    }
  } // end second pass over HSlabs

  simulation::time_solid_energy.stop();
  
}


// Return (Ainc, binc)
std::pair<double, double>
exec_bc(const std::string& bvar,
        Input bval, //const auto& bval,
		vector<std::shared_ptr<Face>> bval1,
        double A,
        std::shared_ptr<SNode> wall_node,
        int bound_ind)
{
  double Ainc = 0.0;
  double binc = 0.0;

  if (bvar == "hflux") {

    // binc = bval * A * wall_node->AFF;

  }
  else if (bvar == "conv") {

    // // bval[0] = h, bval[1] = Tf
    // binc = A * bval[0] * bval[1];
    // Ainc = bval[0] * A;

  }
  else if (bvar == "node") {

    // auto* flow_node = bval[1];
    // double h = calc_value(bval[0], flow_node, wall_node);

    // double relax = 1.0;
    // wall_node->htc = relax * h + (1.0 - relax) * wall_node->htc;

    // double Tf = flow_node->stemp_gues;
    // double hA = wall_node->htc * A;

    // binc = hA * Tf;
    // Ainc = hA;

  }
  else if (bvar == "pipe" || bvar == "pipenl") {

    auto flow_elem = bval1[bound_ind];
    // double h = 0.0;

    if (bvar == "pipe") {

      // h = calc_value(bval[0], flow_elem, wall_node);
	  double h = eval(bval,flow_elem,wall_node.get());

      // double relax = 1.0;
      // wall_node->htc = relax * h + (1.0 - relax) * wall_node->htc;
	  wall_node->htc = h;

      double Tf = flow_elem->stemp_gues;
      double hA = wall_node->htc * A;

      binc = hA * Tf;
      Ainc = hA;

    }
    else if (bvar == "pipenl") {

      // // bval[0] returns (Sc, Sp, h)
      // auto tup = bval[0](flow_elem, wall_node);

      // double Sc = std::get<0>(tup);
      // double Sp = std::get<1>(tup);
      // double h  = std::get<2>(tup);

      // wall_node->htc = h;   // for plotting only
      // wall_node->Sp  = Sp;
      // wall_node->Sc  = Sc;

      // binc = Sc * A;
      // Ainc = -Sp * A;
    }

  }
  else {
    throw std::runtime_error("BC not specified. stopping: " + bvar);
  }

  return {Ainc, binc};
}


double exec_ht(const std::string& bvar,
               Input bval,
               vector<std::shared_ptr<Face>> bval1,
               double A,
               std::shared_ptr<SNode> wall_node,
               int bound_ind)
{
    double heat_transfer = 0.0;
    double relax = 0.5;

    if (bvar == "pipe" || bvar == "pipenl") {

        auto flow_elem = bval1[bound_ind];

        if (bvar == "pipe") {
            double h = wall_node->htc;
            double Tf = flow_elem->stemp_gues;
            double hA = h * A;
            heat_transfer = relax * (wall_node->temp_gues - Tf) * hA + (1.0 - relax) * wall_node->heat_transfer;
            flow_elem->heat_hslab.push_back(heat_transfer);

    //     } else if (bvar == "pipenl") {
    //         double Sc = wall_node->Sc;
    //         double Sp = wall_node->Sp;
    //         heat_transfer = relax * (-wall_node->temp_gues * Sp - Sc) * A + (1.0 - relax) * wall_node->heat_transfer;
    //         flow_elem->heat_hslab.push_back(heat_transfer);
        }

    } else if (bvar == "conv") {
    //     heat_transfer = std::any_cast<double>(bval[0]) * (wall_node->temp_gues - std::any_cast<double>(bval[1])) * A;
    //
    } else if (bvar == "hflux") {
    //     heat_transfer = -std::any_cast<double>(bval[0]) * A * wall_node->AFF;
    //
    } else if (bvar == "node") {
    //     SNode* flow_node = std::any_cast<SNode*>(bval[1]);
    //     double h = wall_node->htc;
    //     double hA = h * A;
    //     double Tw = wall_node->temp_gues;
    //     double Tf = flow_node->stemp_gues;
    //     heat_transfer = relax * (Tw - Tf) * hA + (1.0 - relax) * wall_node->heat_transfer;
    //     flow_node->heat_hslab.push_back(heat_transfer);
    //
    } else {
        std::cerr << "unknown option in heat transfer. stopping" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return heat_transfer;
}

  }
}
