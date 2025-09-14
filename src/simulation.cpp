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
    if (trans_sim) {
      simulation::delt = settings::tim_slot[i] - settings::tim_slot[i-1];
	  settings::alpha_mom = 0.6;
    }
	double alpha_mom = settings::alpha_mom;
    if (mpi::rank == 0) {
    if (settings::verbosity >= 1) std::cout << "time=" << std::setprecision(5) << simulation::current_time << " ";
    }
    // action_setup.update(time, delt);
    bool converged;
    double eps_m, eps_p, eps_h, eps_t;
    for (int main_iter = 0; main_iter < settings::no_main_iter; ++main_iter) {
      
      for (int flow_iter = 0; flow_iter < settings::no_flow_iter; ++flow_iter) {
        exec_massmom(simulation::current_time, simulation::delt, trans_sim, alpha_mom, main_iter, flow_iter);
		simulation::time_convergence.start();
        // if (mpi::rank == 0) {
          std::tuple<bool, std::tuple<double, double>> result = check_conv(simulation::current_time, simulation::delt, trans_sim, alpha_mom, "massmom");
          bool converged_local = std::get<0>(result);
          std::tie(eps_m, eps_p) = std::get<1>(result);
		// }

		if (settings::verbosity >= 6) {
          std::ofstream fout("convergence_rank_" + std::to_string(mpi::rank) + ".txt");
          fout << "massmom iteration " << flow_iter + 1 << " eps_m=" << eps_m << " eps_p=" << eps_p << std::endl;
          fout.close();
		}

        PetscReal eps_m_local = eps_m;
        PetscReal eps_p_local = eps_p;
        PetscReal eps_m_global, eps_p_global;

        MPI_Allreduce(&eps_m_local, &eps_m_global, 1, MPIU_REAL, MPIU_MAX, mpi::intracomm);
        MPI_Allreduce(&eps_p_local, &eps_p_global, 1, MPIU_REAL, MPIU_MAX, mpi::intracomm);

        eps_m = eps_m_global;
        eps_p = eps_p_global;

        int conv_local  = converged_local ? 1 : 0;
        int conv_global = 0;
        MPI_Allreduce(&conv_local, &conv_global, 1, MPI_INT, MPI_LAND, mpi::intracomm);

        converged = (conv_global != 0);
		simulation::time_convergence.stop();
		
		simulation::time_update_old.start();
		if (converged) {
			update_old();
		}
		simulation::time_update_old.stop();

        // if (flow_iter == 0) {
          // MPI_Abort(mpi::intracomm, 0);
          // std::exit(0);
        // }


        // MPI_Barrier(mpi::intracomm);
        // MPI_Bcast(&converged, 1, MPI_C_BOOL, 0, mpi::intracomm);
		
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

    if (settings::flag_write) {
	if (mpi::rank == 0) {
      writeOutput(simulation::current_time, simulation::delt);
	}
    }
  }
  if (mpi::rank == 0) {
    std::cout << "Execution time = " << (std::clock() - start_time) / (double)CLOCKS_PER_SEC << std::endl;
  }

  try {
    std::string fname = "circuits.h5";
    hid_t file_id = opensd::create_or_open_file(fname.c_str());
  
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

for (auto& circuit : model::circuits_owned) {
auto &A = circuit->A;  // alias
auto &b = circuit->b;  // alias
auto &pc = circuit->pc;  // alias
auto &ksp = circuit->ksp;  // alias

KSPDestroy(&ksp);
MatDestroy(&A);
VecDestroy(&b);
VecDestroy(&pc);
VecDestroy(&circuit->pc_local);

VecDestroy(&circuit->vflow_gues_local);
VecDestroy(&circuit->aminus_local);
VecDestroy(&circuit->aplus_local);
VecDestroy(&circuit->bplus_local);
VecDestroy(&circuit->bminus_local);
VecDestroy(&circuit->velocity_local);
VecDestroy(&circuit->rhomass_local);

// Clean up SNES
// MatDestroy(&J);
SNESDestroy(&circuit->snes);
// Use defaults (later override with -snes_fd or -snes_mf_operator from CLI)
SNESSetFromOptions(&circuit->snes);
VecDestroy(&circuit->x);
VecDestroy(&circuit->r);

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

struct Allocation {
  std::vector<int> ranks_for_c;  // number of ranks assigned to each circuit
  std::vector<int> start;        // starting rank for each circuit
};

// ------------------------------------------------------
// Weighted allocator: works even if #circuits > #procs
// ------------------------------------------------------
Allocation allocate_circuits(int P, const std::vector<long long>& work) {
  int C = (int)work.size();
  Allocation alloc;
  alloc.ranks_for_c.assign(C, 0);
  alloc.start.assign(C, 0);

  // -----------------------------------
  // Case 1: Enough ranks (C <= P)
  // -----------------------------------
  if (C <= P) {
    long double total = 0.0L;
    for (auto w : work) total += (long double)std::max(1LL, w);

    // initial floor
    for (int i = 0; i < C; ++i) {
      long double exact = (long double)P * ((long double)std::max(1LL, work[i]) / total);
      int k = (int)std::floor(exact);
      alloc.ranks_for_c[i] = std::max(1, k);
    }

    // fix sum with largest remainders
    int sum = 0; for (int k : alloc.ranks_for_c) sum += k;
    struct R { int i; long double frac; };
    std::vector<R> rem; rem.reserve(C);

    for (int i = 0; i < C; ++i) {
      long double exact = (long double)P * ((long double)std::max(1LL, work[i]) / total);
      rem.push_back({i, exact - (long double)std::floor(exact)});
    }

    if (sum < P) {
      std::sort(rem.begin(), rem.end(), [](auto& a, auto& b){ return a.frac > b.frac; });
      for (int t = 0; t < P - sum; ++t) alloc.ranks_for_c[rem[t].i] += 1;
    } else if (sum > P) {
      std::sort(rem.begin(), rem.end(), [](auto& a, auto& b){ return a.frac < b.frac; });
      for (int t = 0; t < sum - P; ++t) alloc.ranks_for_c[rem[t].i] -= 1;
    }

    // prefix sums -> [lo,hi) ranges
    for (int i = 1; i < C; ++i)
      alloc.start[i] = alloc.start[i-1] + alloc.ranks_for_c[i-1];
  }

  // -----------------------------------
  // Case 2: Not enough ranks (C > P)
  // -----------------------------------
  else {
    struct Bin { long long load = 0; std::vector<int> circuits; };
    std::vector<Bin> bins(P);

    // Sort circuits by descending work
    std::vector<int> order(C);
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(),
              [&](int a, int b){ return work[a] > work[b]; });

    for (int ci : order) {
      // pick bin with least load
      auto it = std::min_element(bins.begin(), bins.end(),
                                 [](auto& x, auto& y){ return x.load < y.load; });
      it->circuits.push_back(ci);
      it->load += work[ci];
    }

    // each circuit gets exactly 1 rank, possibly shared
    for (int r = 0; r < P; ++r) {
      for (int ci : bins[r].circuits) {
        alloc.ranks_for_c[ci] = 1;
        alloc.start[ci] = r;   // directly assign rank id
      }
    }
  }

  return alloc;
}

void calculate_work()
{
	

  // build work metric
std::vector<long long> work;
work.reserve(model::circuits.size());
for (auto& c : model::circuits) {
  long long nodes = (long long)c->nodes.size();
  long long faces = (long long)c->faces.size();
  work.push_back(10LL*faces + 2LL*nodes); //arbitrary
}

// allocate
Allocation alloc = allocate_circuits(mpi::n_procs, work);
auto& ranks_for_c = alloc.ranks_for_c;
auto& start       = alloc.start;


model::circuits_owned.clear();

  for (size_t cidx = 0; cidx < model::circuits.size(); ++cidx) {
    auto& circuit = model::circuits[cidx];
    // std::cout << circuit->identifier << std::endl;




int world_rank = mpi::rank; // 0..P-1
int color = MPI_UNDEFINED;
int key   = 0;

  int lo = start[cidx];
  int hi = start[cidx] + ranks_for_c[cidx];
  if (world_rank >= lo && world_rank < hi) {
    color = (int)cidx;                  // one color per circuit
    key   = world_rank - lo;         // rank inside that circuit
  }

MPI_Comm circuit_comm = MPI_COMM_NULL;
MPI_Comm_split(mpi::intracomm, color, key, &circuit_comm);

std::cout << "Rank " << world_rank
          << " checking circuit " << circuit->identifier
          << " lo=" << lo << " hi=" << hi
          << " color=" << color << " key=" << key
          << std::endl;

// if this process is NOT part of this circuit, skip it entirely
    if (circuit_comm == MPI_COMM_NULL) {
      // Remember to set these so other code knows this process doesn't participate.
      circuit->comm = MPI_COMM_NULL;
      continue;
    }

// store communicator and local rank/size
  circuit->comm = circuit_comm;
  int cir_rank=-1, cir_size=-1;
  MPI_Comm_rank(circuit_comm, &cir_rank);
  MPI_Comm_size(circuit_comm, &cir_size);
  circuit->rank_in_comm = cir_rank;
  circuit->comm_size    = cir_size;

std::cout << "Global rank " << mpi::rank
          << " -> Circuit " << circuit->identifier
          << " | circuit rank: " << circuit->rank_in_comm
          << " / " << (circuit->comm_size - 1)
          << " (comm_size=" << circuit->comm_size << ")"
          << std::endl;
 model::circuits_owned.push_back(circuit);
  }


  for (size_t cidx = 0; cidx < model::circuits.size(); ++cidx) {
    auto& circuit = model::circuits[cidx];

  MPI_Comm circuit_comm = circuit->comm;
  int cir_rank = circuit->rank_in_comm;
  int cir_size = circuit->comm_size;


    if (circuit_comm == MPI_COMM_NULL) {
      continue;
    }


    // Build a map from Node* to contiguous METIS vertex ID
    std::unordered_map<std::shared_ptr<Node>, idx_t> node_to_vertex;
    std::vector<std::shared_ptr<Node>> vertex_to_node;
    idx_t vertex_count = 0;

    for (auto& node : circuit->nodes) {
      node_to_vertex[node] = vertex_count++;
      vertex_to_node.push_back(node);
    }
  
    // std::cout << "vertex_to_node:\n";
    // for (size_t i = 0; i < vertex_to_node.size(); ++i)
    //   std::cout << "  vertex " << i << " -> node " << vertex_to_node[i]->identifier << "\n";
    
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
    idx_t nparts = cir_size;  // Set this to number of partitions
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


std::vector<PetscInt> counts(cir_size, 0);
for (PetscInt i = 0; i < nvtxs; ++i) {
    counts[part[i]]++;
}

// 2. Compute starting offsets for each rank
std::vector<PetscInt> offsets(cir_size, 0);
for (int r = 1; r < cir_size; ++r) {
    offsets[r] = offsets[r-1] + counts[r-1];
}
// 3. Map old index → new contiguous index
circuit->old2new.resize(nvtxs);
std::vector<PetscInt> position = offsets; // running positions
for (PetscInt i = 0; i < nvtxs; ++i) {
    PetscInt r = part[i];
    circuit->old2new[i] = position[r]++;
}


    std::ofstream fout("partition_c" + std::to_string(cidx) + "_rank_" + std::to_string(cir_rank) + ".txt");
    for (int i = 0; i < nvtxs; ++i) {
        fout << circuit->nodes[i]->identifier
     << " old=" << i
     << " part=" << part[i]
     << " petsc=" << circuit->old2new[i] << "\n";

    }
    fout.close();
    
    MPI_Barrier(circuit_comm);
    if (cir_rank == 0) {
        std::cerr << "METIS partition debug print complete.\n";
    }
    
    size_t i = 0;
    circuit->faces_owned.clear();
    circuit->face_indices_owned.clear();
    circuit->ghost_faces_owned.clear();
    circuit->ghost_face_indices_owned.clear();
    circuit->ghost_nodes_owned.clear();
    circuit->ghost_nodes_owned1.clear();
    circuit->ghost_indices_owned.clear();

    for (auto& face : circuit->faces) {
      int u_rank = part[node_to_vertex[face->unode]];
      int v_rank = part[node_to_vertex[face->dnode]];
      face->owner = u_rank;
      if (face->owner == cir_rank) {
        circuit->faces_owned.push_back(face);
        circuit->face_indices_owned.push_back(i);
      }

  	if ((u_rank == cir_rank || v_rank == cir_rank) && face->owner != cir_rank) {
              circuit->ghost_faces_owned[face->owner].push_back(face);
              circuit->ghost_face_indices_owned.push_back(i);
          }
  		
  	if (v_rank != cir_rank && u_rank == cir_rank) {
              circuit->ghost_nodes_owned[v_rank].push_back(face->dnode);
              circuit->ghost_nodes_owned1.push_back(face->dnode);
              circuit->ghost_indices_owned.push_back(circuit->old2new[face->dnode->node_ind]); // convert face->dnode->node_ind (old vertex index) to circuit-local index via old2new
          }
      ++i;
  	}

  	// for (auto& face : circuit->faces) {
        // std::cout << "Face " << face->faceno
                  // << " connects nodes " << face->unode->identifier
                  // << " and " << face->dnode->identifier
                  // << " => Owner: " << face->owner << "\n";
    // }
  
    std::ofstream ghost_debug("ghosts_circuit" + std::to_string(cidx) + "_rank_" + std::to_string(cir_rank) + ".txt");
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
    if (part[node_to_vertex[node]] == cir_rank) {
      circuit->nodes_owned.push_back(node);
      circuit->indices_owned.push_back(circuit->old2new[node->node_ind]);
    }
  }
PetscInt local_nrows = counts[cir_rank];

// Debug print faces_owned per rank
{
  std::ofstream fout("faces_owned_c" + std::to_string(cidx) + "_rank_" + std::to_string(cir_rank)+ ".txt");

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


// ---- Build face_old2new for faces (contiguous per owner rank) ----
{
  // 1) Count faces per rank
  std::vector<PetscInt> face_counts(cir_size, 0);
  for (size_t f = 0; f < circuit->faces.size(); ++f) {
    int owner = circuit->faces[f]->owner;  // set earlier as u_rank
    if (owner >= 0 && owner < cir_size) face_counts[owner]++;
  }

  // 2) Offsets per rank
  std::vector<PetscInt> face_offsets(cir_size, 0);
  for (int r = 1; r < cir_size; ++r) {
    face_offsets[r] = face_offsets[r-1] + face_counts[r-1];
  }

  // 3) Map old face index -> new contiguous index
  circuit->face_old2new.assign(circuit->faces.size(), -1);
  std::vector<PetscInt> face_position = face_offsets; // running insertion points

  for (size_t f = 0; f < circuit->faces.size(); ++f) {
    int owner = circuit->faces[f]->owner;
    if (owner >= 0 && owner < cir_size) {
      circuit->face_old2new[f] = face_position[owner]++;
    }
  }

  // 4) Convenience: local list of owned face local indices (for this rank)
  // circuit->face_local_indices_owned.clear();
  // for (size_t idx = 0; idx < circuit->face_indices_owned.size(); ++idx) {
  //   size_t f_old = circuit->face_indices_owned[idx];            // old/global-in-circuit face index
  //   PetscInt f_local = circuit->face_old2new[f_old];            // contiguous index within circuit by owner-grouping
  //   circuit->face_local_indices_owned.push_back(f_local);
  // }

  // (Optional) If you want local indices for ghost faces too:
  // circuit->ghost_face_local_indices_owned.clear();
  // for (size_t idx = 0; idx < circuit->ghost_face_indices_owned.size(); ++idx) {
  //   size_t f_old = circuit->ghost_face_indices_owned[idx];
  //   PetscInt f_local = circuit->face_old2new[f_old];
  //   circuit->ghost_face_local_indices_owned.push_back(f_local);
  // }

  // Debug dump
  {
    std::ofstream fdbg("face_reindex_c" + std::to_string(cidx) +
                       "_rank_" + std::to_string(cir_rank) + ".txt");
    fdbg << "Rank " << cir_rank << ": face_counts/offsets\n";
    for (int r = 0; r < cir_size; ++r) {
      fdbg << "  r=" << r << " count=" << face_counts[r]
           << " off=" << face_offsets[r] << "\n";
    }
    fdbg << "\nold -> new (by owner):\n";
    for (size_t f = 0; f < circuit->faces.size(); ++f) {
      const auto& face = circuit->faces[f];
      fdbg << "  face_old=" << f
           << " faceno=" << face->faceno
           << " owner=" << face->owner
           << " new=" << circuit->face_old2new[f] << "\n";
    }
  }
}


// --- PETSc create matrix and vectors with METIS partition sizes ---
PetscInt global_nrows = vertex_count; // same as nvtxs

    PetscInt n = circuit->nodes.size();

    auto &A = circuit->A; 
    MatCreate(circuit_comm, &A);
    MatSetSizes(A, local_nrows, local_nrows, global_nrows, global_nrows);
    MatSetFromOptions(A);
    MatSetUp(A);
    
    auto &b = circuit->b; 
    VecCreate(circuit_comm, &b);
    VecSetSizes(b, local_nrows, global_nrows);
    VecSetFromOptions(b);
    
    auto &pc = circuit->pc; 
    VecCreate(circuit_comm, &pc);
    VecSetSizes(pc, local_nrows, global_nrows);
    VecSetFromOptions(pc);
    
    auto &ksp = circuit->ksp; 
    
    KSPCreate(circuit->comm, &ksp);
    KSPSetOperators(ksp, A, A);
    KSPSetFromOptions(ksp);

    PetscInt n_faces_owned = circuit->face_indices_owned.size();
// ghost indices in PETSc global numbering
circuit->ghost_face_global_indices.clear();
for (auto f_old : circuit->ghost_face_indices_owned) {
  circuit->ghost_face_global_indices.push_back(circuit->face_old2new[f_old]);
}
PetscInt n_faces_ghost = circuit->ghost_face_global_indices.size();

    // Create ghost vectors
    VecCreateGhost(circuit_comm, n_faces_owned, PETSC_DECIDE, n_faces_ghost,
                   circuit->ghost_face_global_indices.data(), &circuit->vflow_gues_local);
    VecCreateGhost(circuit_comm, n_faces_owned, PETSC_DECIDE, n_faces_ghost,
                   circuit->ghost_face_global_indices.data(), &circuit->aminus_local);
    VecCreateGhost(circuit_comm, n_faces_owned, PETSC_DECIDE, n_faces_ghost,
                   circuit->ghost_face_global_indices.data(), &circuit->aplus_local);
    VecCreateGhost(circuit_comm, n_faces_owned, PETSC_DECIDE, n_faces_ghost,
                   circuit->ghost_face_global_indices.data(), &circuit->bplus_local);
    VecCreateGhost(circuit_comm, n_faces_owned, PETSC_DECIDE, n_faces_ghost,
                   circuit->ghost_face_global_indices.data(), &circuit->bminus_local);

    VecCreateGhost(circuit_comm, n_faces_owned, PETSC_DECIDE, n_faces_ghost,
               circuit->ghost_face_global_indices.data(), &circuit->rhomass_local);

	PetscInt n_local = circuit->indices_owned.size();
    PetscInt nghost  = circuit->ghost_indices_owned.size();
		   
    VecCreateGhost(circuit_comm, n_local, PETSC_DECIDE, nghost,
               circuit->ghost_indices_owned.data(), &circuit->pc_local);

    VecCreateGhost(circuit_comm, n_local, PETSC_DECIDE, nghost,
               circuit->ghost_indices_owned.data(), &circuit->velocity_local);
   
    auto &snes = circuit->snes; 
    auto &x = circuit->x;
    auto &r = circuit->r;
    SNESCreate(circuit->comm, &snes);

  // Create solution vector (owned + ghosts)
  VecDuplicate(circuit->vflow_gues_local, &x);
  VecDuplicate(circuit->vflow_gues_local, &r);

  }

}

} // namespace opensd
