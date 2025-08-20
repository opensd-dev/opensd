//! \file simulation.cpp
#include "opensd/simulation.h"

#include <iostream>
#include <ctime>
#include <cstdlib> // for std::exit
#include <cmath> // for rounding function
#include <iomanip> // for setting precision in output

#include "opensd/capi.h"
#include "opensd/convergence.h"
#include "opensd/error.h"
#include "opensd/flow_solver.h"
#include "opensd/hdf5_interface.h"
#include "opensd/message_passing.h"
#include "opensd/output.h"
#include "opensd/post.h"
#include "opensd/settings.h"
#include "opensd/timer.h"
#include <metis.h>
#include <unordered_map>
#include <fstream>
#include <petscksp.h>

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

  opensd::simulation::time_total.start();
  opensd_simulation_init();

  // Ensure that a timestep isn't executed in the case that the maximum number of
  // time steps has already been run in a restart statepoint file

  // opensd::settings::alpha_mom
  openFile("outputfile");

  // Loop through time slots
  for (int i = 0; i < settings::tim_slot.size(); ++i) {
    simulation::current_time = settings::tim_slot[i];
    
    bool trans_sim = settings::run_mode == RunMode::TRANSIENT;
	double alpha_mom = settings::alpha_mom;
    if (trans_sim) {
      simulation::delt = settings::tim_slot[i] - settings::tim_slot[i-1];
	  alpha_mom = 0.6;
    }
    if (mpi::rank == 0) {
    if (settings::verbosity >= 1) std::cout << "time=" << std::setprecision(5) << simulation::current_time << " ";
    }
    // action_setup.update(time, delt);
    bool converged;
    double eps_m, eps_p, eps_h, eps_t;
    for (int main_iter = 0; main_iter < settings::no_main_iter; ++main_iter) {
      
      for (int flow_iter = 0; flow_iter < settings::no_flow_iter; ++flow_iter) {
        exec_massmom(simulation::current_time, simulation::delt, trans_sim, alpha_mom, main_iter, flow_iter);
        // if (mpi::rank == 0) {
          std::tuple<bool, std::tuple<double, double>> result = check_conv(simulation::current_time, simulation::delt, trans_sim, alpha_mom, "massmom");
          converged = std::get<0>(result);
          std::tie(eps_m, eps_p) = std::get<1>(result);
		// }
		
		MPI_Bcast(&converged, 1, MPI_C_BOOL, 0, mpi::intracomm);
		
		if (mpi::rank == 0) {
          if (converged) {
            if (settings::verbosity >= 2 || (settings::verbosity >= 1 && !trans_sim)) {
              std::cout << "massmom converged in " << flow_iter + 1 << " iter. " << eps_m << " " << eps_p << std::endl;
            }
          } else {
            if (settings::verbosity >= 3 || (settings::verbosity >= 2 && !trans_sim)) {
              std::cout << "massmom iteration " << flow_iter + 1 << " " << eps_m << " " << eps_p << std::endl;
            }
          }
        }
		
		if (converged) break;
		
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

      if (converged) {
        if (settings::temp_solve) {
          if (settings::verbosity >= 1 || (settings::verbosity >= 0 && !trans_sim)) {
              eps_h = 0.;
              eps_t = 0.;
            std::cout << "main converged in " << main_iter + 1 << " iter. " << eps_m << " " << eps_p << " " << eps_h << " " << eps_t << std::endl;
          }
        } else {
		  if (mpi::rank == 0) {
            if (settings::verbosity >= 1 || (settings::verbosity >= 0 && !trans_sim)) {
              std::cout << "main converged in " << main_iter + 1 << " iter. " << eps_m << " " << eps_p << std::endl;
            }
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
	if (mpi::rank == 0) {
      writeOutput(simulation::current_time, simulation::delt);
	}
    // }
  }
  if (mpi::rank == 0) {
    std::cout << "Execution time = " << (std::clock() - start_time) / (double)CLOCKS_PER_SEC << std::endl;
  }

  if (mpi::rank == 0) {
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
  }

  opensd_simulation_finalize();
  opensd::simulation::time_total.stop();

  return 0;

}



int opensd_simulation_init()
{
  using namespace opensd;

  // Skip if simulation has already been initialized
  if (simulation::initialized)
    return 0;


  // Determine how much work each process should do
  calculate_work();
  
  opensd_reset();

/*   // If this is a restart run, load the state point data and binary source
  // file
  if (settings::restart_run) {
    load_state_point();
    write_message("Resuming simulation...", 6);
  } else {
    // Only initialize primary source bank for eigenvalue simulations
    if (settings::run_mode == RunMode::EIGENVALUE &&
        settings::solver_type == SolverType::MONTE_CARLO) {
      initialize_source();
    }
  }
 */

/*   // Display header
  if (mpi::master) {
    if (settings::run_mode == RunMode::FIXED_SOURCE) {
      if (settings::solver_type == SolverType::MONTE_CARLO) {
        header("FIXED SOURCE TRANSPORT SIMULATION", 3);
      } else if (settings::solver_type == SolverType::RANDOM_RAY) {
        header("FIXED SOURCE TRANSPORT SIMULATION (RANDOM RAY SOLVER)", 3);
      }
    } else if (settings::run_mode == RunMode::EIGENVALUE) {
      if (settings::solver_type == SolverType::MONTE_CARLO) {
        header("K EIGENVALUE SIMULATION", 3);
      } else if (settings::solver_type == SolverType::RANDOM_RAY) {
        header("K EIGENVALUE SIMULATION (RANDOM RAY SOLVER)", 3);
      }
      if (settings::verbosity >= 7)
        print_columns();
    }
  }
 */

  // Set flag indicating initialization is done
  simulation::initialized = true;
  return 0;
}

int opensd_simulation_finalize()
{
  using namespace opensd;

  // Skip if simulation was never run
  if (!simulation::initialized)
    return 0;

  // Start finalization timer
  simulation::time_finalize.start();

for (auto& circuit : model::circuits) {
auto &A = circuit->A;  // alias
auto &b = circuit->b;  // alias
auto &pc = circuit->pc;  // alias
auto &m = circuit->m;  // alias
auto &ksp = circuit->ksp;  // alias

KSPDestroy(&ksp);
MatDestroy(&A);
VecDestroy(&b);
VecDestroy(&pc);
VecDestroy(&m);
}

// #ifdef OPENMC_MPI
  // broadcast_results();
// #endif

  // Stop timers and show timing statistics
  simulation::time_finalize.stop();
  simulation::time_total.stop();
  if (mpi::master) {
    // if (settings::solver_type != SolverType::RANDOM_RAY) {
      // if (settings::verbosity >= 6)
        print_runtime();
      // if (settings::verbosity >= 4)
        // print_results();
    // }
  }

  // Reset flags
  simulation::initialized = false;
  return 0;
}



namespace opensd {

//==============================================================================
// Global variables
//==============================================================================

namespace simulation {

double current_time; //!< current time
double delt {1.E8}; //!< time step
bool initialized {false};

} // namespace simulation

//==============================================================================
// Non-member functions
//==============================================================================

void calculate_work()
{
	
  // Build a map from Node* to contiguous METIS vertex ID
  std::unordered_map<std::shared_ptr<Node>, idx_t> node_to_vertex;
  std::vector<std::shared_ptr<Node>> vertex_to_node;
  idx_t vertex_count = 0;
  
  for (auto& circuit : model::circuits) {
    for (auto& node : circuit->nodes) {
      node_to_vertex[node] = vertex_count++;
      vertex_to_node.push_back(node);
    }
  
    std::cout << "vertex_to_node:\n";
    for (size_t i = 0; i < vertex_to_node.size(); ++i)
      std::cout << "  vertex " << i << " -> node " << vertex_to_node[i]->identifier << "\n";
    
    // Adjacency graph (CSR format)
    std::vector<idx_t> xadj(vertex_count + 1, 0);
    
    for (auto& face : circuit->faces) {
      idx_t u = node_to_vertex[face->unode];
      idx_t v = node_to_vertex[face->dnode];
      xadj[u + 1]++;
      xadj[v + 1]++;
    }
    
    // Convert xadj to cumulative sum
    for (size_t i = 1; i < xadj.size(); ++i) {
      xadj[i] += xadj[i - 1];
    }
    
    std::vector<idx_t> adjncy(xadj.back());
    std::vector<idx_t> current = xadj;  // track where to insert next neighbor
  
    for (auto& face : circuit->faces) {
      idx_t u = node_to_vertex[face->unode];
      idx_t v = node_to_vertex[face->dnode];
      
      adjncy[current[u]++] = v;
      adjncy[current[v]++] = u;
    }
  
/*     std::cout << "xadj:\n";
    for (size_t i = 0; i < xadj.size(); ++i)
        std::cout << "  xadj[" << i << "] = " << xadj[i] << "\n";
    
    
    std::cout << "Adjacency list per vertex:\n";
    for (size_t i = 0; i < vertex_to_node.size(); ++i) {
        std::cout << "  vertex " << i << " (node " << vertex_to_node[i] << "): ";
        for (int j = xadj[i]; j < xadj[i + 1]; ++j)
            std::cout << adjncy[j] << " ";
        std::cout << "\n";
    }
    
    for (idx_t i = 0; i < vertex_count; ++i) {
        for (idx_t j = xadj[i]; j < xadj[i+1]; ++j) {
            std::cout << "Edge: " << i << " -- " << adjncy[j] << "\n";
        }
    }
 */    
    idx_t nvtxs = vertex_count;
    idx_t ncon = 1;
    idx_t nparts = mpi::n_procs;  // Set this to number of partitions
    std::vector<idx_t> part(vertex_count);  // Output
    
    idx_t objval;
    if (nparts > 1) {
        int status = METIS_PartGraphKway(&nvtxs, &ncon,
                                         xadj.data(), adjncy.data(),
                                         NULL, NULL, NULL,
                                         &nparts, NULL, NULL, NULL,
                                         &objval, part.data());
    
        if (status != METIS_OK) {
            fatal_error("METIS partitioning failed");
        }
    } else {
        // Assign everything to part 0
        std::fill(part.begin(), part.end(), 0);
    }


std::vector<PetscInt> counts(mpi::n_procs, 0);
for (PetscInt i = 0; i < nvtxs; ++i) {
    counts[part[i]]++;
}

// 2. Compute starting offsets for each rank
std::vector<PetscInt> offsets(mpi::n_procs, 0);
for (int r = 1; r < mpi::n_procs; ++r) {
    offsets[r] = offsets[r-1] + counts[r-1];
}
// 3. Map old index → new contiguous index
circuit->old2new.resize(nvtxs);
std::vector<PetscInt> position = offsets; // running positions
for (PetscInt i = 0; i < nvtxs; ++i) {
    PetscInt r = part[i];
    circuit->old2new[i] = position[r]++;
}


    std::ofstream fout("partition_rank_" + std::to_string(mpi::rank) + ".txt");
    for (int i = 0; i < nvtxs; ++i) {
        fout << "Node " << i << " -> Part " << part[i] << " Petsc " << part[circuit->old2new[i]] << "\n";
    }
    fout.close();
    
    MPI_Barrier(mpi::intracomm);
    if (mpi::rank == 0) {
        std::cerr << "METIS partition debug print complete.\n";
    }
    
    size_t i = 0;
    for (auto& face : circuit->faces) {
      int u_rank = part[node_to_vertex[face->unode]];
      int v_rank = part[node_to_vertex[face->dnode]];
            face->owner = u_rank;
			if (face->owner == mpi::rank) {
			  circuit->faces_owned.push_back(face);
              circuit->face_indices_owned.push_back(i);
            }

  	if ((u_rank == mpi::rank || v_rank == mpi::rank) && face->owner != mpi::rank) {
              circuit->ghost_faces_owned[face->owner].push_back(face);
              circuit->ghost_face_indices_owned.push_back(i);
          }
  		
  	if (v_rank != mpi::rank && u_rank == mpi::rank) {
              circuit->ghost_nodes_owned[v_rank].push_back(face->dnode);
              circuit->ghost_indices_owned.push_back(circuit->old2new[face->dnode->node_ind]); // global index
          }
      ++i;
  	}
  
    // for (auto& face : circuit->faces) {
        // std::cout << "Face " << face->faceno
                  // << " connects nodes " << face->unode->identifier
                  // << " and " << face->dnode->identifier
                  // << " => Owner: " << face->owner << "\n";
    // }
  
    std::ofstream ghost_debug("ghosts_rank_" + std::to_string(mpi::rank) + ".txt");
    for (const auto& [rank, nodes] : circuit->ghost_nodes_owned) {
        ghost_debug << "Need nodes from rank " << rank << ": ";
        for (auto node : nodes)
        ghost_debug << node->identifier << " ";
        ghost_debug << "\n";
    }
    for (const auto& [rank, faces] : circuit->ghost_faces_owned) {
        ghost_debug << "Need faces from rank " << rank << ": ";
        for (auto face : faces)
        ghost_debug << face->faceno << " ";
    
        ghost_debug << "\n";
    }
    ghost_debug.close();
    
  for (auto& node : circuit->nodes) {
    if (part[node_to_vertex[node]] == mpi::rank) {
      circuit->nodes_owned.push_back(node);
      circuit->indices_owned.push_back(circuit->old2new[node->node_ind]);
    }
  }
PetscInt local_nrows = counts[mpi::rank];

// Debug print faces_owned per rank
{
  std::ofstream fout("faces_owned_rank_" + std::to_string(mpi::rank) + ".txt");

  fout << "Rank " << mpi::rank << " owns " << circuit->faces_owned.size() << " faces\n";
  for (auto& face : circuit->faces_owned) {
    fout << "Face " << face->faceno
         << " upstream node: " << face->unode->identifier
         << " (rank " << part[node_to_vertex[face->unode]] << ")"
         << " downstream node: " << face->dnode->identifier
         << " (rank " << part[node_to_vertex[face->dnode]] << ")"
         << " => Owner: " << face->owner << "\n";
  }
}




// --- PETSc create matrix and vectors with METIS partition sizes ---
PetscInt global_nrows = vertex_count; // same as nvtxs

    PetscInt n = circuit->nodes.size();

    auto &A = circuit->A; 
    MatCreate(mpi::intracomm, &A);
    MatSetSizes(A, local_nrows, local_nrows, global_nrows, global_nrows);
    MatSetFromOptions(A);
    MatSetUp(A);
    
    auto &b = circuit->b; 
    VecCreate(mpi::intracomm, &b);
    VecSetSizes(b, local_nrows, global_nrows);
    VecSetFromOptions(b);
    
    auto &pc = circuit->pc; 
    VecCreate(mpi::intracomm, &pc);
    VecSetSizes(pc, local_nrows, global_nrows);
    VecSetFromOptions(pc);
    
    auto &m = circuit->m; 
    VecCreate(mpi::intracomm, &m);
    VecSetSizes(m, local_nrows, global_nrows);
    VecSetFromOptions(m);


    
  }

}

} // namespace opensd
