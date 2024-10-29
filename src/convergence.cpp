//! \file convergence.cpp

#include "opensd/convergence.h"

#include <iostream>
#include <tuple>

#include "opensd/initialize.h"
#include "opensd/settings.h"

namespace opensd {
    
std::tuple<bool, std::tuple<double, double>> check_conv(double time, double delt, bool trans_sim, double alpha_mom, std::string opt) {
  double eps_mtot = 0.0, eps_ptot = 0.0, eps_htot = 0.0, eps_ttot = 0.0;

  for (auto& circuit : model::circuits) {
    
    vector<double> eps_mlist;
    for (auto& node : circuit->nodes) {
      node->mresidue = node->eqn_cont(alpha_mom);
      // if (dynamic_cast<comp::Reservoir*>(&node)) node.mresidue = 0;
      if (std::abs(node->mflow_in) > 1.E-5 || std::abs(node->mflow_out) > 1.E-5) {
        eps_mlist.push_back(std::abs(node->mresidue));
      }
    }

    circuit->eps_p = 0.0;
    std::vector<double> e_mass;
    for (auto& face : circuit->faces) {
      face->presidue = face->eqn_mom(face->vflow_gues, time, delt, trans_sim, alpha_mom);
      circuit->eps_p += std::abs(face->presidue) / face->tpres_gues;
      face->mflow = face->vflow_gues * face->ther_gues->rhomass();
      if (std::abs(face->mflow) > 1.0E-5) {
        e_mass.push_back(std::abs(face->mflow));
      }
      face->mflow = face->vflow_gues * face->ther_gues->rhomass();
    }
    for (auto& pipe : circuit->pipes) {
      // pipe.update_mflow();
    }


    if (e_mass.empty()) {
      circuit->eps_m = 0.0;
    } else {
      circuit->mean_flow = std::accumulate(e_mass.begin(), e_mass.end(), 0.0) / e_mass.size();
      if (!eps_mlist.empty()) {
        circuit->eps_m = *std::max_element(eps_mlist.begin(), eps_mlist.end()) / circuit->mean_flow;
      }
    }

    
    
    eps_mtot = std::max(eps_mtot, circuit->eps_m);
    eps_ptot = std::max(eps_ptot, circuit->eps_p);
    if (opt == "all") {
      eps_htot = std::max(eps_htot, circuit->eps_h);
    }
  }

  bool condition = (eps_ptot < settings::conv_crit_flow) && (eps_mtot < settings::conv_crit_flow);
  std::tuple<double, double> ret_args = std::make_tuple(eps_mtot, eps_ptot);

  if (condition) {
    
    
    for (auto& circuit : model::circuits) {
    for (auto& node : circuit->nodes) {
      // std::cout << node->identifier << " " << node->tpres_gues/1.E6 << std::endl;
    }

    for (auto& face : circuit->faces) {
      // std::cout << face->vflow_gues*face->ther_gues->rhomass() << std::endl;
    }


    }

    
    return std::make_tuple(true, ret_args);
  } else {
    return std::make_tuple(false, ret_args);
  }
}

}