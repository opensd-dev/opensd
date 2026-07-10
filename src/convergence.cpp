//! \file convergence.cpp

#include "opensd/convergence.h"

#include <iostream>
#include <iomanip>
#include <tuple>
//#include <cstdlib>

#include "opensd/initialize.h"
#include "opensd/message_passing.h"
#include "opensd/settings.h"
#include "opensd/simulation.h"
#include "opensd/timer.h"

namespace opensd {
    
std::tuple<bool, double, double, double, double> check_conv(double time, double delt, bool trans_sim, double alpha_mom, double alpha_ener, std::string opt, double alpha_heat) {
  double eps_mtot = 0.0, eps_ptot = 0.0, eps_htot = 0.0, eps_ttot = 0.0;

  for (auto& circuit : model::circuits_owned) {
    simulation::time_conv_mass.start();
    vector<double> eps_mlist;
    #pragma omp parallel for
    for (size_t n = 0; n < circuit->nodes_owned.size(); ++n) {
      auto& node = circuit->nodes_owned[n];
      node->mresidue = node->eqn_cont(time,delt,trans_sim,alpha_mom);
      // std::cout << "rank " << mpi::rank << " " << node->identifier << " " << node->mresidue << std::endl;
      // if (dynamic_cast<comp::Reservoir*>(&node)) node.mresidue = 0;
    }
    for (size_t n = 0; n < circuit->nodes_owned.size(); ++n) {
      auto& node = circuit->nodes_owned[n];
      if (std::abs(node->mflow_in) > 1.E-5 || std::abs(node->mflow_out) > 1.E-5) {
        eps_mlist.push_back(std::abs(node->mresidue));
      }
    }
    simulation::time_conv_mass.stop();

    simulation::time_conv_mom.start();
    double eps_p_sum = 0.0;
    std::vector<double> e_mass;

    #pragma omp parallel for reduction(+:eps_p_sum)
    for (size_t i = 0; i < circuit->faces_owned.size(); ++i) {
      auto& face = circuit->faces_owned[i];
      face->presidue = face->eqn_mom(face->vflow_gues, time, delt, trans_sim, alpha_mom);
      // std::cout << "rank " << mpi::rank << " " << std::setprecision(12) << std::fixed << " face " << face->faceno << " " << face->presidue << std::endl;
      eps_p_sum += std::abs(face->presidue) / face->tpres_gues;
      face->mflow = face->vflow_gues * face->ther_gues->rhomass();
    }
    circuit->eps_p = eps_p_sum;

    for (auto& face : circuit->faces_owned) {
      if (std::abs(face->mflow) > 1.0E-5) {
        e_mass.push_back(std::abs(face->mflow));
      }
    }
    simulation::time_conv_mom.stop();

    for (auto& pipe : circuit->pipes) {
      if (!pipe->faces.empty()) {
        double total_mflow = 0.0;
        for (auto& face : pipe->faces) {
          total_mflow += face->mflow;
        }
        pipe->mflow = total_mflow / pipe->faces.size();
      }
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
          // std::cout << "flag1 " << node->identifier << " " << abs(node->hresidue)/(node->tenth_gues*circuit->mean_flow) <<std::endl;
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




  if (opt == "all") {

    for (auto& hslab : model::hslabs) {

      // if (!trans_sim && HSlab->solveSS == false)
      //   continue;
      //
      hslab->eps_tlist.clear();
      hslab->htlist.clear();

      double eps_t;

      for (auto& layer : hslab->layers) {
        for (auto& snode : layer->snodes) {

          double eq = std::abs(snode->eqn_ener(time, delt, trans_sim, alpha_heat));
          hslab->eps_tlist.push_back(eq);

          if (snode->heat_transfer != 0.0)
            hslab->htlist.push_back(std::abs(snode->heat_transfer));
          //std::cout<<snode->identifier<<" "<<snode->temp_gues<<std::endl;
        }
      }
      // std::exit(0);

      if (hslab->htlist.empty()) {
        eps_t = 0.0;
      }
      else {

        hslab->mean_ht =  std::accumulate(hslab->htlist.begin(), hslab->htlist.end(), 0.0) / hslab->htlist.size();

        double max_eps_t = *std::max_element(hslab->eps_tlist.begin(),
                                             hslab->eps_tlist.end());

        eps_t = (hslab->mean_ht < 1.0 && max_eps_t < 1.0)
                  ? 0.0
                  : max_eps_t / hslab->mean_ht;

      }

      eps_ttot = std::max(eps_ttot, eps_t);
    }
  }









  auto within_criterion = [](double value, double criterion) {
    constexpr double absolute_margin = 2.0e-12;
    return value <= criterion + absolute_margin;
  };

  bool condition;
  if (opt == "all") {
    double conv_crit_temp;
    if (trans_sim) {
      conv_crit_temp = settings::conv_crit_temp_trans;
    } else {
      conv_crit_temp = settings::conv_crit_temp_SS;
    }
    condition = within_criterion(eps_ptot, settings::conv_crit_flow) &&
                     within_criterion(eps_mtot, settings::conv_crit_flow) &&
                     within_criterion(eps_htot, conv_crit_temp) &&
                     within_criterion(eps_ttot, settings::conv_crit_ht);
					 
  } else if (opt == "massmom") {
    condition = within_criterion(eps_ptot, settings::conv_crit_flow) &&
                     within_criterion(eps_mtot, settings::conv_crit_flow);  

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

    for (auto& hslab : model::hslabs) {
      for (auto& layer : hslab->layers) {
        for (auto& snode : layer->snodes) {
          snode->update_old();
        }
        for (auto& iface : layer->ifaces) {
          iface->update_old();
        }
        for (auto& jface : layer->jfaces) {
          jface->update_old();
        }
      }
    }
}

}
