//! \file convergence.cpp

#include "opensd/convergence.h"

#include <iostream>
#include <iomanip>
#include <tuple>

#include "opensd/initialize.h"
#include "opensd/message_passing.h"
#include "opensd/settings.h"

namespace opensd {
    
std::tuple<bool, double, double, double, double> check_conv(double time, double delt, bool trans_sim, double alpha_mom, double alpha_ener, std::string opt, double alpha_heat) {
  double eps_mtot = 0.0, eps_ptot = 0.0, eps_htot = 0.0, eps_ttot = 0.0;

  for (auto& circuit : model::circuits_owned) {
    
    vector<double> eps_mlist;
    for (auto& node : circuit->nodes_owned) {
      node->mresidue = node->eqn_cont(time,delt,trans_sim,alpha_mom);
      // std::cout << "rank " << mpi::rank << " " << node->identifier << " " << node->mresidue << std::endl;
      // if (dynamic_cast<comp::Reservoir*>(&node)) node.mresidue = 0;
      if (std::abs(node->mflow_in) > 1.E-5 || std::abs(node->mflow_out) > 1.E-5) {
        eps_mlist.push_back(std::abs(node->mresidue));
      }
    }

    circuit->eps_p = 0.0;
    std::vector<double> e_mass;
    for (auto& face : circuit->faces_owned) {
      face->presidue = face->eqn_mom(face->vflow_gues, time, delt, trans_sim, alpha_mom);
      // std::cout << "rank " << mpi::rank << " " << std::setprecision(12) << std::fixed << " face " << face->faceno << " " << face->presidue << std::endl;
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
    for (auto& face : circuit->faces_owned) {
      // std::cout << "rank " << mpi::rank << " vflow face " << face->faceno << " " << face->ther_gues->rhomass() << std::endl;
    }
    // for (auto& node : circuit->nodes_owned) {
    //   std::cout << "rank " << mpi::rank << " " << node->identifier << " " << node->tpres_gues << std::endl;
    // }


    if (e_mass.empty()) {
      circuit->eps_m = 0.0;
    } else {
      circuit->mean_flow = std::accumulate(e_mass.begin(), e_mass.end(), 0.0) / e_mass.size();
      if (!eps_mlist.empty()) {
        circuit->eps_m = *std::max_element(eps_mlist.begin(), eps_mlist.end()) / circuit->mean_flow;
      }
    }


	if (opt == "all") {
	  circuit->eps_h = 0.;
	  for (auto& node : circuit->nodes_owned) {
	    if (not e_mass.empty() != 0) { // node.flowreg == "Homogeneous" and
          node->hresidue = node->eqn_ener(time,delt,trans_sim,alpha_ener);
	      circuit->eps_h = std::max(circuit->eps_h,abs(node->hresidue)/(node->tenth_gues*circuit->mean_flow)); // node.tenth_gues*node.volume*node.ther_gues.rhomass()/delt)(or) node.tenth_gues*mean_flow
		}
	  }
	}
	if (circuit->mean_flow <= 1.E-1) {
	  circuit->eps_h = 1.E-11;
	}

    
    
    eps_mtot = std::max(eps_mtot, circuit->eps_m);
    eps_ptot = std::max(eps_ptot, circuit->eps_p);
    if (opt == "all") {
      eps_htot = std::max(eps_htot, circuit->eps_h);
    }
  }
  
  bool condition;
  if (opt == "all") {
    double conv_crit_temp;
    if (trans_sim) {
      conv_crit_temp = settings::conv_crit_temp_trans;
    } else {
      conv_crit_temp = settings::conv_crit_temp_SS;
    }
    condition = (eps_ptot < settings::conv_crit_flow) &&
                     (eps_mtot < settings::conv_crit_flow) &&
                     (eps_htot < conv_crit_temp) &&
                     (eps_ttot < settings::conv_crit_ht);
					 
  } else if (opt == "massmom") {
    condition = (eps_ptot < settings::conv_crit_flow) &&
                     (eps_mtot < settings::conv_crit_flow);  

  }

  if (condition) {
    return {true, eps_mtot, eps_ptot, eps_htot, eps_ttot};
  } else {
    return {false, eps_mtot, eps_ptot, eps_htot, eps_ttot};
  }

}

void update_old() {
    
    
    for (auto& circuit : model::circuits_owned) {
    for (auto& node : circuit->nodes_owned) {
      node->update_old();
      // std::cout << node->identifier << " " << node->tpres_gues/1.E6 << std::endl;
    }
	
    for (auto& node : circuit->ghost_nodes_owned1) {
      node->update_old();
    }


    for (auto& face : circuit->faces_owned) {
      if (not face->choked) {
        face->ther_gues->update();
      }
      face->update_old();
      // std::cout << face->vflow_gues*face->ther_gues->rhomass() << std::endl;
    }

    for (size_t j = 0; j < circuit->ghost_face_indices_owned.size(); ++j) {
      auto idx = circuit->ghost_face_indices_owned[j];
      auto& face = circuit->faces[idx];
      // if (not face->choked) {
        // face->ther_gues->update();
      // }
      // face->update_old();
	  face->vflow_old = face->vflow_gues;
	  face->ther_old->set_rhomass(face->ther_gues->rhomass());
    }
    
	}
}

}
