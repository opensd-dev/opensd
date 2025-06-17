//! \file simulation.cpp
#include "opensd/simulation.h"

#include <iostream>
#include <ctime>
#include <cstdlib> // for std::exit
#include <cmath> // for rounding function
#include <iomanip> // for setting precision in output

#include "opensd/settings.h"
#include "opensd/convergence.h"
#include "opensd/flow_solver.h"
#include "opensd/post.h"
#include "opensd/hdf5_interface.h"

//==============================================================================
// C API functions
//==============================================================================

// OPENSD_RUN encompasses all the main logic where iterations are performed
// over the time steps 

int opensd_run()
{
  using namespace opensd;

  std::clock_t start_time;
  start_time = std::clock();

  // Ensure that a timestep isn't executed in the case that the maximum number of
  // time steps has already been run in a restart statepoint file

  // opensd::settings::alpha_mom

  // Loop through time slots
  for (int i = 0; i < settings::tim_slot.size(); ++i) {
    simulation::current_time = settings::tim_slot[i];
    if (i != 0) {
      simulation::delt = settings::tim_slot[i] - settings::tim_slot[i-1];
    }
    
    if (settings::verbosity >= 1) std::cout << "time=" << std::setprecision(5) << simulation::current_time << " ";
    
    // action_setup.update(time, delt);
    bool trans_sim = false;
    bool converged;
    double eps_m, eps_p, eps_h, eps_t;
    for (int main_iter = 0; main_iter < settings::no_main_iter; ++main_iter) {
      
      for (int flow_iter = 0; flow_iter < settings::no_flow_iter; ++flow_iter) {
        exec_massmom(simulation::current_time, simulation::delt, trans_sim, settings::alpha_mom, main_iter, flow_iter);
        std::tuple<bool, std::tuple<double, double>> result = check_conv(simulation::current_time, simulation::delt, trans_sim, settings::alpha_mom, "massmom");
        converged = std::get<0>(result);
        std::tie(eps_m, eps_p) = std::get<1>(result);
        if (converged) {
          if (settings::verbosity >= 3 || (settings::verbosity >= 2 && !trans_sim)) {
            std::cout << "massmom converged in " << flow_iter + 1 << " iter. " << eps_m << " " << eps_p << std::endl;
          }
          break;
        } else {
          if (settings::verbosity >= 3 || (settings::verbosity >= 2 && !trans_sim)) {
            std::cout << "massmom iteration " << flow_iter + 1 << " " << eps_m << " " << eps_p << std::endl;
          }
        }
      }
      
      if (!converged) {
        std::cerr << "massmom not converged. stopping " << eps_m << " " << eps_p << std::endl;
        std::exit(EXIT_FAILURE);
      }

      if (settings::temp_solve) {
        // HT_solver.exec_energy(time, delt, trans_sim, alpha_heat, main_iter);
        exec_energy(simulation::current_time, simulation::delt, trans_sim, settings::alpha_ener, main_iter);
        std::exit(1);

        // bool converged;
        // double eps_h, eps_t, eps_hvof;
        // std::tie(converged, std::tie(eps_m, eps_p, eps_h, eps_t, eps_hvof)) = convergence.check_conv(
            // time, delt, trans_sim, alpha_mom, alpha_ener, "all", alpha_heat);
      }

      if (converged && main_iter > 1) {
        if (settings::temp_solve) {
          if (settings::verbosity >= 1 || (settings::verbosity >= 0 && !trans_sim)) {
              eps_h = 0.;
              eps_t = 0.;
            std::cout << "main converged in " << main_iter + 1 << " iter. " << eps_m << " " << eps_p << " " << eps_h << " " << eps_t << std::endl;
          }
        } else {
          if (settings::verbosity >= 1 || (settings::verbosity >= 0 && !trans_sim)) {
            std::cout << "main converged in " << main_iter + 1 << " iter. " << eps_m << " " << eps_p << std::endl;
          }
        }
        break;
      } else {
        if (settings::verbosity >= 2 || (settings::verbosity >= 1 && !trans_sim)) {
          std::cout << "main iteration " << main_iter + 1 << " " << eps_m << " " << eps_p << " " << eps_h << " " << eps_t << std::endl;
        }
      }
    }

    if (!converged) {
      std::cerr << "main not converged. stopping " << eps_m << " " << eps_p << " " << eps_h << " " << eps_t << " " << std::endl;
      std::exit(EXIT_FAILURE);
    }

    // for (auto& lmass : HTcomp.LumpedMass::_registry) {
      // lmass.update(time, delt);
    // }

    // post.update_calcs(time, delt);

    // if (flag_write) {
      openFile("outputfile");
      writeOutput(simulation::current_time, simulation::delt);
    // }
  }
  
  std::cout << "Execution time = " << (std::clock() - start_time) / (double)CLOCKS_PER_SEC << std::endl;
    

  try {
    hid_t file_id = opensd::create_or_open_file("circuits.h5");
  
    // Create /circuits group if it doesn't exist
    if (!H5Lexists(file_id, "/circuits", H5P_DEFAULT)) {
      hid_t circuits_group = H5Gcreate(file_id, "/circuits", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
      if (circuits_group < 0) throw std::runtime_error("Failed to create /circuits group");
      H5Gclose(circuits_group);
    }
  
    for (size_t i = 0; i < model::circuits.size(); ++i) {
      std::string group_name = "/circuits/circuit_" + std::to_string(i);
      hid_t group_id = H5Gcreate(file_id, group_name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
      if (group_id < 0) throw std::runtime_error("Failed to create group: " + group_name);
  
      model::circuits[i]->save_to_hdf5(group_id);
  
      H5Gclose(group_id);
    }
  
    opensd::close_file(file_id);
    std::cout << "Circuits saved to HDF5 successfully.\n";
  
  } catch (const std::exception& e) {
    std::cerr << "HDF5 error during save: " << e.what() << std::endl;
  }
  
  model::circuits.clear();
  
  try {
    hid_t file_id = H5Fopen("circuits.h5", H5F_ACC_RDONLY, H5P_DEFAULT);
  
    size_t index = 0;
    while (true) {
      std::string group_name = "/circuits/circuit_" + std::to_string(index);
      if (H5Lexists(file_id, group_name.c_str(), H5P_DEFAULT) <= 0) break;
  
      hid_t group_id = H5Gopen(file_id, group_name.c_str(), H5P_DEFAULT);
      auto circuit = std::make_shared<opensd::Circuit>();
      circuit->load_from_hdf5(group_id);
      model::circuits.push_back(circuit);
      H5Gclose(group_id);
      ++index;
    }
  
    H5Fclose(file_id);
  
    std::cout << "Read " << model::circuits.size() << " circuit(s) from HDF5.\n";
  
  } catch (const std::exception& e) {
    std::cerr << "HDF5 error during load: " << e.what() << std::endl;
  }


// Print to verify
for (size_t i = 0; i < model::circuits.size(); ++i) {
  const auto& circuit = model::circuits[i];
  std::cout << "Circuit [" << i << "] ID: " << circuit->identifier << "\n";
  std::cout << "  mean_flow: " << circuit->mean_flow << "\n";
  std::cout << "  eps_h: " << circuit->eps_h << "\n";

  // Nodes
  std::cout << "  Nodes (" << circuit->nodes.size() << "):\n";
  for (size_t j = 0; j < circuit->nodes.size(); ++j) {
    const auto& node = circuit->nodes[j];
    if (node) {
      std::cout << "    Node [" << j << "] ID: " << node->identifier << "\n";
      std::cout << "      volume: " << node->volume << "\n";
      std::cout << "      mflow_in: " << node->mflow_in << "\n";
      std::cout << "      mflow_out: " << node->mflow_out << "\n";
    } else {
      std::cout << "    Node [" << j << "] is null\n";
    }
  }

  // Pipes
  std::cout << "  Pipes (" << circuit->pipes.size() << "):\n";
  for (size_t j = 0; j < circuit->pipes.size(); ++j) {
    const auto& pipe = circuit->pipes[j];
    if (pipe) {
      std::cout << "    Pipe [" << j << "] ID: " << pipe->identifier << "\n";
      std::cout << "      length: " << pipe->length << "\n";
      std::cout << "      diameter: " << pipe->diameter << "\n";
    } else {
      std::cout << "    Pipe [" << j << "] is null\n";
    }
  }

  // BCs
  std::cout << "  BCs (" << circuit->bcs.size() << "):\n";
  for (size_t j = 0; j < circuit->bcs.size(); ++j) {
    const auto& bc = circuit->bcs[j];
    std::cout << "    BC [" << j << "] ID: " << bc.identifier << "\n";
    std::cout << "      node: " << bc.node_ << "\n";
    std::cout << "      var: " << bc.var_ << "\n";
    std::cout << "      val: " << bc.val_ << "\n";
  }
  
    // Faces
  std::cout << "  Faces (" << circuit->faces.size() << "):\n";
  for (size_t j = 0; j < circuit->faces.size(); ++j) {
    const auto& face = circuit->faces[j];
    if (face) {
      std::cout << "    Face [" << j << "] faceno: " << face->faceno << "\n";

      if (face->unode) {
        std::cout << "      unode ID: " << face->unode->identifier << "\n";
        std::cout << "      ufrac: " << face->ufrac << ", uheight: " << face->uheight << "\n";
      } else {
        std::cout << "      unode is null\n";
      }

      if (face->dnode) {
        std::cout << "      dnode ID: " << face->dnode->identifier << "\n";
        std::cout << "      dfrac: " << face->dfrac << ", dheight: " << face->dheight << "\n";
      } else {
        std::cout << "      dnode is null\n";
      }

      std::cout << "      mflow: " << face->mflow << ", velocity: " << face->velocity << "\n";
      std::cout << "      choked: " << std::boolalpha << face->choked << "\n";
      std::cout << "      heat_input: " << face->heat_input << "\n";
      // std::cout << "      heat_hslab size: " << face->heat_hslab.size() << "\n";

    } else {
      std::cout << "    Face [" << j << "] is null\n";
    }
  }

  
}



  return 0;

}

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================

namespace simulation {

double current_time; //!< current time
double delt {1.E8}; //!< time step

} // namespace simulation

//==============================================================================
// Non-member functions
//==============================================================================

} // namespace opensd
