//! \file ht_solver.cpp
#include "opensd/ht_solver.h"

// #include <algorithm>
// #include <cmath>         // For std::isinf and other math functions
// #include <iostream>
// #include <iomanip>
#include <Eigen/Dense>   // For matrix manipulations
#include <Eigen/Sparse>
#include <Eigen/SparseCholesky>
#include <Eigen/SparseLU>
#include <cstdlib>
#include <memory>
#include <unordered_map>
#include <unordered_set>
// #include "opensd/vector.h"
#include "opensd/message_passing.h"
#include "opensd/simulation.h"
// #include "opensd/settings.h"
#include "opensd/timer.h"

#include "opensd/hslab.h"
#include "opensd/circuit.h"
#include "opensd/settings.h"
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

void sync_fluid_state() {
  if (mpi::n_procs <= 1 || model::hslabs.empty()) return;

  simulation::time_hslab_sync.start();
  std::unordered_set<const Face*> faces_owned;
  for (const auto& circuit : model::circuits_owned) {
    for (const auto& face : circuit->faces_owned) faces_owned.insert(face.get());
  }

  enum BoundaryStateField {
    BoundaryStaticTemperature,
    BoundaryVelocity,
    BoundaryDensity,
    BoundarySpecificHeat,
    BoundaryConductivity,
    BoundaryFieldCount
  };
  constexpr size_t stride = BoundaryFieldCount;
  for (const auto& hslab : model::hslabs) {
    const size_t count = hslab->uval1.size() + hslab->dval1.size();
    std::vector<double> local(count * stride, 0.0), global(local.size());
    size_t i = 0;
    auto pack = [&](const std::shared_ptr<Face>& face) {
      if (faces_owned.count(face.get()) != 0) {
        local[i + BoundaryStaticTemperature] = face->stemp_gues;
        local[i + BoundaryVelocity] = face->velocity;
        local[i + BoundaryDensity] = face->ther_gues->rhomass();
        local[i + BoundarySpecificHeat] = face->ther_gues->cpmass();
        local[i + BoundaryConductivity] = face->ther_gues->conductivity();
      }
      i += stride;
    };
    for (const auto& face : hslab->uval1) pack(face);
    for (const auto& face : hslab->dval1) pack(face);
    MPI_Reduce(local.data(), global.data(), static_cast<int>(global.size()),
               MPI_DOUBLE, MPI_SUM, hslab->root_world_rank, mpi::intracomm);

    if (mpi::rank == hslab->root_world_rank) {
      i = 0;
      auto unpack = [&](const std::shared_ptr<Face>& face) {
        face->stemp_gues = global[i + BoundaryStaticTemperature];
        face->velocity = global[i + BoundaryVelocity];
        face->ther_gues->set_rhomass(global[i + BoundaryDensity]);
        face->ther_gues->set_cpmass(global[i + BoundarySpecificHeat]);
        face->ther_gues->set_conductivity(global[i + BoundaryConductivity]);
        i += stride;
      };
      for (const auto& face : hslab->uval1) unpack(face);
      for (const auto& face : hslab->dval1) unpack(face);
    }
  }
  simulation::time_hslab_sync.stop();
}

void sync_boundary_temperatures() {
  if (mpi::n_procs <= 1 || model::hslabs.empty()) return;

  std::unordered_set<const Face*> faces_owned;
  for (const auto& circuit : model::circuits_owned) {
    for (const auto& face : circuit->faces_owned) faces_owned.insert(face.get());
  }

  for (const auto& hslab : model::hslabs) {
    const size_t count = hslab->uval1.size() + hslab->dval1.size();
    std::vector<double> local(count, 0.0), global(count);
    size_t i = 0;
    for (const auto& face : hslab->uval1) {
      if (faces_owned.count(face.get()) != 0) local[i] = face->stemp_gues;
      ++i;
    }
    for (const auto& face : hslab->dval1) {
      if (faces_owned.count(face.get()) != 0) local[i] = face->stemp_gues;
      ++i;
    }
    MPI_Reduce(local.data(), global.data(), static_cast<int>(count), MPI_DOUBLE,
               MPI_SUM, hslab->root_world_rank, mpi::intracomm);
    if (mpi::rank == hslab->root_world_rank) {
      i = 0;
      for (const auto& face : hslab->uval1) face->stemp_gues = global[i++];
      for (const auto& face : hslab->dval1) face->stemp_gues = global[i++];
    }
  }
}

void exec_energy(double time, double delt, bool trans_sim, double alpha_heat, int main_iter) {

  simulation::time_solid_energy.start();

  sync_fluid_state();

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

    std::unordered_set<const SNode*> uwnodes;
    std::unordered_set<const SNode*> dwnodes;
    uwnodes.reserve(hslab->uwnodes.size());
    dwnodes.reserve(hslab->dwnodes.size());
    for (const auto& node : hslab->uwnodes) uwnodes.insert(node.get());
    for (const auto& node : hslab->dwnodes) dwnodes.insert(node.get());

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

    Eigen::VectorXd tempVec = Eigen::VectorXd::Zero(n);
    int i = -1;
    if (mpi::rank == hslab->root_world_rank) {
      simulation::time_hslab_assembly.start();
      auto pipe_htc = [](const std::string& bvar, const Input& bval,
                         const vector<std::shared_ptr<Face>>& faces,
                         const vector<std::shared_ptr<SNode>>& wall_nodes) {
        std::vector<double> values;
        if (bvar != "pipe") return values;
        values.resize(wall_nodes.size());
        if (bval.type == InputType::CONSTANT) {
          std::fill(values.begin(), values.end(), bval.constant_value);
        } else {
          py::gil_scoped_acquire gil;
          if (py::hasattr(bval.py_callable, "batch")) {
            py::list density, velocity, diameter, cpmass, conductivity;
            for (const auto& face : faces) {
              density.append(face->ther_gues->rhomass());
              velocity.append(face->velocity);
              const auto* pface = dynamic_cast<const PFace*>(face.get());
              if (!pface) {
                throw std::runtime_error("Batch pipe heat-transfer function requires pipe faces.");
              }
              diameter.append(pface->diameter);
              cpmass.append(face->ther_gues->cpmass());
              conductivity.append(face->ther_gues->conductivity());
            }
            py::sequence result = bval.py_callable.attr("batch")(
              density, velocity, diameter, cpmass, conductivity);
            if (py::len(result) != static_cast<py::ssize_t>(values.size())) {
              throw std::runtime_error("Batch heat-transfer function returned the wrong number of values.");
            }
            for (size_t j = 0; j < values.size(); ++j) {
              values[j] = result[j].cast<double>();
            }
          } else {
            for (size_t j = 0; j < wall_nodes.size(); ++j) {
              values[j] = bval.py_callable(faces[j], wall_nodes[j].get()).cast<double>();
            }
          }
        }
        return values;
      };
      const auto uhtc = pipe_htc(hslab->uvar, hslab->uval,
                                 hslab->uval1, hslab->uwnodes);
      const auto dhtc = pipe_htc(hslab->dvar, hslab->dval,
                                 hslab->dval1, hslab->dwnodes);
      Eigen::SparseMatrix<double> A(n, n);
      A.reserve(Eigen::VectorXi::Constant(n, 5));
      Eigen::VectorXd b = Eigen::VectorXd::Zero(n);

      for (auto &layer : hslab->layers) {
        for (auto &node : layer->snodes) {
          ++i;
        // East face
        if (node->eface != nullptr) {
          double aE  = alpha_heat * node->eface->A * node->eface->ther_gues->conductivity() / node->eface->delx;
          double aE0 = (1.0 - alpha_heat) * node->eface->A * node->eface->ther_old->conductivity() / node->eface->delx;
          b(i) -= (node->temp_old - node->eface->dnode->temp_old) * aE0;
          int col = i + hslab->ninc;
          if (col >= 0 && col < n) A.coeffRef(i, col) = -aE;
          A.coeffRef(i, i) += aE;
        }
  
        // West face
        if (node->wface != nullptr) {
          double aW  = alpha_heat * node->wface->A * node->wface->ther_gues->conductivity() / node->wface->delx;
          double aW0 = (1.0 - alpha_heat) * node->wface->A * node->wface->ther_old->conductivity() / node->wface->delx;
          b(i) -= (node->temp_old - node->wface->unode->temp_old) * aW0;
          int col = i - hslab->ninc;
          if (col >= 0 && col < n) A.coeffRef(i, col) = -aW;
          A.coeffRef(i, i) += aW;
        }
  
        // North face
        if (node->nface != nullptr) {
          double aN  = alpha_heat * node->nface->A * node->nface->ther_gues->conductivity() / layer->dely;
          double aN0 = (1.0 - alpha_heat) * node->nface->A * node->nface->ther_old->conductivity() / layer->dely;
          b(i) -= (node->temp_old - node->nface->dnode->temp_old) * aN0;
          int col = i + 1;
          if (col >= 0 && col < n) A.coeffRef(i, col) = -aN;
          A.coeffRef(i, i) += aN;
        }
  
        // South face
        if (node->sface != nullptr) {
          double aS  = alpha_heat * node->sface->A * node->sface->ther_gues->conductivity() / layer->dely;
          double aS0 = (1.0 - alpha_heat) * node->sface->A * node->sface->ther_old->conductivity() / layer->dely;
          b(i) -= (node->temp_old - node->sface->unode->temp_old) * aS0;
          int col = i - 1;
          if (col >= 0 && col < n) A.coeffRef(i, col) = -aS;
          A.coeffRef(i, i) += aS;
        }

        // Heat input and transient contribution
        b(i) = b(i) + alpha_heat * node->heat_input + (1.0 - alpha_heat) * node->heat_input_old;
        if (trans_sim) {
          b(i) = b(i) + node->ther_old->cpmass() * node->ther_old->rhomass() * node->vol / delt * node->temp_old;
          A.coeffRef(i, i) += node->ther_gues->cpmass() * node->ther_gues->rhomass() * node->vol / delt;
        }
  
        // Boundary conditions (uwnodes)
        if (uwnodes.count(node.get()) != 0) {
          const double* htc = uhtc.empty() ? nullptr : &uhtc[i];
          auto [Ainc, binc] = exec_bc(hslab->uvar, hslab->uval, hslab->uval1, node->Ai, node, i, htc);
          b(i) = b(i) + alpha_heat * binc - (1.0 - alpha_heat) * node->heat_transfer_old;
          A.coeffRef(i, i) += alpha_heat * Ainc;
        }

        // Boundary conditions (dwnodes) - note Python used i-n+HSlab.ninc index for bc call
        if (dwnodes.count(node.get()) != 0) {
          int bc_index = i - n + hslab->ninc; // as in Python
          const double* htc = dhtc.empty() ? nullptr : &dhtc[bc_index];
          auto [Ainc, binc] = exec_bc(hslab->dvar, hslab->dval, hslab->dval1, node->Ai, node, bc_index, htc);
          b(i) = b(i) + alpha_heat * binc - (1.0 - alpha_heat) * node->heat_transfer_old;
          A.coeffRef(i, i) += alpha_heat * Ainc;
        }
        }
      } // end building A, b
      A.makeCompressed();
      simulation::time_hslab_assembly.stop();

      // Solve the complete HSLAB system on its owner rank. Keeping the full
      // matrix here preserves axial couplings when they are enabled.
      simulation::time_hslab_solve.start();
      using LDLT = Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>>;
      struct LDLTCache {
        int rows = 0;
        Eigen::Index nonzeros = 0;
        std::unique_ptr<LDLT> solver;
      };
      static std::unordered_map<const HSlab*, LDLTCache> ldlt_cache;
      auto& cache = ldlt_cache[hslab.get()];
      if (!cache.solver || cache.rows != n ||
          cache.nonzeros != A.nonZeros()) {
        cache.rows = n;
        cache.nonzeros = A.nonZeros();
        cache.solver = std::make_unique<LDLT>();
        cache.solver->analyzePattern(A);
      }
      cache.solver->factorize(A);
      if (cache.solver->info() == Eigen::Success) {
        tempVec = cache.solver->solve(b);
      } else {
        Eigen::SparseLU<Eigen::SparseMatrix<double>> lu;
        lu.compute(A);
        if (lu.info() == Eigen::Success) {
          tempVec = lu.solve(b);
        } else {
          tempVec = Eigen::MatrixXd(A).colPivHouseholderQr().solve(b);
        }
      }
      simulation::time_hslab_solve.stop();
    }

    simulation::time_hslab_sync.start();
    MPI_Bcast(tempVec.data(), n, MPI_DOUBLE, hslab->root_world_rank,
              mpi::intracomm);
    std::vector<double> htc_values(n, 0.0);
    i = -1;
    for (auto& layer : hslab->layers) {
      for (auto& snode : layer->snodes) {
        htc_values[++i] = snode->htc;
      }
    }
    MPI_Bcast(htc_values.data(), n, MPI_DOUBLE, hslab->root_world_rank,
              mpi::intracomm);
    if (mpi::rank != hslab->root_world_rank) {
      i = -1;
      for (auto& layer : hslab->layers) {
        for (auto& snode : layer->snodes) {
          snode->htc = htc_values[++i];
        }
      }
    }
    simulation::time_hslab_sync.stop();

    // Assign temp_gues back to nodes (with relaxation where applied)
    simulation::time_hslab_update.start();
    i = -1;
    for (auto& layer : hslab->layers) {
      double relax = 1.0;
       // The original python toggles relax for some cases — kept simple here:
      if (!trans_sim) relax = 1.0;
      for (auto& snode : layer->snodes) {
        ++i;
        if (dwnodes.count(snode.get()) != 0 || uwnodes.count(snode.get()) != 0) {
          snode->temp_gues = relax * tempVec(i) + (1.0 - relax) * snode->temp_gues;
        } else {
          snode->temp_gues = tempVec(i);
        }
        // std::cout<<snode->identifier<< " " << snode->temp_gues<<std::endl;
      }
    }
    simulation::time_hslab_update.stop();
  } // end first pass over HSlabs

  // Second pass: compute heat transfers and update thermo states
  simulation::time_hslab_update.start();
  for (auto& hslab : model::hslabs) {
    std::unordered_set<const SNode*> uwnodes;
    std::unordered_set<const SNode*> dwnodes;
    uwnodes.reserve(hslab->uwnodes.size());
    dwnodes.reserve(hslab->dwnodes.size());
    for (const auto& node : hslab->uwnodes) uwnodes.insert(node.get());
    for (const auto& node : hslab->dwnodes) dwnodes.insert(node.get());
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
        if (dwnodes.count(snode.get()) != 0) {
          snode->heat_transfer = exec_ht(hslab->dvar, hslab->dval, hslab->dval1, snode->Ai, snode, i - n + hslab->ninc);
          hslab->dheat_transfer += snode->heat_transfer;
        }

        if (uwnodes.count(snode.get()) != 0) {
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
        snode->heat_input = snode->heat_frac * layer->heat_input;
      }
    }
  } // end second pass over HSlabs
  simulation::time_hslab_update.stop();

  simulation::time_solid_energy.stop();
  
}


// Return (Ainc, binc)
std::pair<double, double>
exec_bc(const std::string& bvar,
        const Input& bval,
		const vector<std::shared_ptr<Face>>& bval1,
        double A,
        const std::shared_ptr<SNode>& wall_node,
        int bound_ind,
        const double* pipe_htc)
{
  double Ainc = 0.0;
  double binc = 0.0;

  if (bvar == "hflux") {

    binc = eval(bval) * A * wall_node->AFF;

  }
  else if (bvar == "conv") {

    // bval[0] = h, bval[1] = Tf
    binc = A * eval(bval) * settings::T_ambient;
    Ainc = eval(bval) * A;

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
      double h = pipe_htc ? *pipe_htc : eval(bval, flow_elem, wall_node.get());

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
               const Input& bval,
               const vector<std::shared_ptr<Face>>& bval1,
               double A,
               const std::shared_ptr<SNode>& wall_node,
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

        heat_transfer = eval(bval) * (wall_node->temp_gues - settings::T_ambient) * A;

    } else if (bvar == "hflux") {
        heat_transfer = -eval(bval) * A * wall_node->AFF;

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
