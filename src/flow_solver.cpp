//! \file flow_solver.cpp
#include "opensd/flow_solver.h"

#include <algorithm>
#include <array>
#include <cmath>         // For std::isinf and other math functions
#include <iostream>
#include <iomanip>
#include <Eigen/Dense>   // For matrix manipulations
#include <Eigen/SVD>
#include <limits>
#include <numeric>
#include <utility>
#include "opensd/hslab.h"
#include "opensd/orifice.h"
#include "opensd/pump.h"
#include "opensd/vector.h"
#include "opensd/message_passing.h"
#include "opensd/simulation.h"
#include "opensd/settings.h"
#include "opensd/timer.h"

#include "opensd/circuit.h"
// #include <numeric>     // For std::accumulate
// #include <copy>          // For std::copy in Arow and brow
// #include <petscksp.h>
// #include <petscsnes.h>
#include <fstream>
#include <cstdlib>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_roots.h>
#include <gsl/gsl_multiroots.h>

namespace opensd {

namespace {

void apply_choked_orifice_state(const std::shared_ptr<Orifice>& orifice)
{
  const double sign = orifice->vflow_gues < 0.0 ? -1.0 : 1.0;
  orifice->spres_gues = orifice->pcr;
  orifice->stemp_gues = orifice->cr_ttemp;
  orifice->ther_gues->set_state(orifice->rhocr, orifice->cr_cpmass,
                                orifice->cr_viscosity, orifice->cr_conductivity,
                                orifice->cr_hmass, 0.0);
  orifice->velocity = orifice->Gcr / orifice->rhocr;
  orifice->tpres_gues = orifice->spres_gues
                      + 0.5 * orifice->rhocr * orifice->velocity * orifice->velocity;
  orifice->ttemp_gues = orifice->stemp_gues
                      + 0.5 * orifice->velocity * orifice->velocity / orifice->cr_cpmass;
  orifice->G = sign * orifice->Gcr;
  orifice->vflow_gues = orifice->G * orifice->cfarea * orifice->opening / orifice->rhocr;
}

void apply_choked_pipe_state(const std::shared_ptr<PFace>& face)
{
  const double sign = face->vflow_gues < 0.0 ? -1.0 : 1.0;
  face->spres_gues = face->pcr;
  face->stemp_gues = face->cr_ttemp;
  face->ther_gues->set_state(face->rhocr, face->cr_cpmass,
                             face->cr_viscosity, face->cr_conductivity,
                             face->cr_hmass, 0.0);
  face->velocity = face->Gcr / face->ther_gues->rhomass();
  face->tpres_gues = face->spres_gues
                   + 0.5 * face->ther_gues->rhomass() * face->velocity * face->velocity;
  face->ttemp_gues = face->stemp_gues
                   + 0.5 * face->velocity * face->velocity / face->ther_gues->cpmass();
  face->vflow_gues = sign * face->Gcr * face->cfarea / face->ther_gues->rhomass();
}

} // namespace

//==============================================================================
// Global variables
//==============================================================================

//==============================================================================
// Non-member functions
//==============================================================================

// Wrapper for a single face equation
struct FaceWrapper {
  std::shared_ptr<Face> face;
  double time;
  double delt;
  bool trans_sim;
  double alpha_mom;
  int main_iter;
};

// Define the residual f(x) for one face
int face_residual_vec(const gsl_vector* x, void* params,
                      gsl_vector* f) {
  auto* fw = static_cast<FaceWrapper*>(params);

  double v = gsl_vector_get(x, 0);
  double r = fw->face->eqn_mom(v, fw->time, fw->delt,
                               fw->trans_sim, fw->alpha_mom);

  gsl_vector_set(f, 0, r);
  return GSL_SUCCESS;
}

// Define the residual f(x) for one face
double face_residual(double x, void* params) {
  auto* fw = static_cast<FaceWrapper*>(params);

  // Example: call your existing residual calculation here
  return fw->face->eqn_mom(x, fw->time, fw->delt,
                                    fw->trans_sim, fw->alpha_mom);
}

std::pair<double, double> solve_face_unbracketed_once(FaceWrapper& fw, double x_guess) {
  const gsl_multiroot_fsolver_type* solver_type = gsl_multiroot_fsolver_hybrids;
  gsl_multiroot_fsolver* solver = gsl_multiroot_fsolver_alloc(solver_type, 1);

  gsl_multiroot_function F;
  F.f = &face_residual_vec;
  F.n = 1;
  F.params = &fw;

  gsl_vector* x = gsl_vector_alloc(1);
  gsl_vector_set(x, 0, x_guess);
  gsl_multiroot_fsolver_set(solver, &F, x);

  int status = GSL_CONTINUE;
  int iter = 0;
  const int max_iter = 100;
  do {
    ++iter;
    status = gsl_multiroot_fsolver_iterate(solver);
    if (status) break;
    status = gsl_multiroot_test_residual(solver->f, 1.0e-8);
  } while (status == GSL_CONTINUE && iter < max_iter);

  double root = gsl_vector_get(solver->x, 0);
  gsl_vector_free(x);
  gsl_multiroot_fsolver_free(solver);

  return {root, std::abs(face_residual(root, &fw))};
}

double solve_face_unbracketed(FaceWrapper& fw, double x_guess) {
  const double seed = 1.0e-4;
  std::array<double, 5> guesses = {
    x_guess,
    std::copysign(std::max(std::abs(x_guess), seed), x_guess == 0.0 ? 1.0 : x_guess),
    -std::copysign(std::max(std::abs(x_guess), seed), x_guess == 0.0 ? 1.0 : x_guess),
    10.0 * seed,
    -10.0 * seed
  };

  auto best = solve_face_unbracketed_once(fw, guesses[0]);
  for (size_t i = 1; i < guesses.size(); ++i) {
    if (best.second < 1.0e-8) break;
    auto candidate = solve_face_unbracketed_once(fw, guesses[i]);
    if (candidate.second < best.second) {
      best = candidate;
    }
  }

  return best.first;
}

double solve_vspump_face_bracketed(FaceWrapper& fw, double x_guess) {
  constexpr double x_lo_init = -0.01;
  constexpr double x_hi_init = 0.01;
  constexpr int max_iter = 100;

  gsl_function F;
  F.function = &face_residual;
  F.params = &fw;

  gsl_root_fsolver* solver = gsl_root_fsolver_alloc(gsl_root_fsolver_brent);
  int status = gsl_root_fsolver_set(solver, &F, x_lo_init, x_hi_init);
  if (status != GSL_SUCCESS) {
    gsl_root_fsolver_free(solver);
    return solve_face_unbracketed(fw, x_guess);
  }

  int iter = 0;
  double root = x_guess;
  double x_lo = x_lo_init;
  double x_hi = x_hi_init;

  do {
    ++iter;
    status = gsl_root_fsolver_iterate(solver);
    root = gsl_root_fsolver_root(solver);
    x_lo = gsl_root_fsolver_x_lower(solver);
    x_hi = gsl_root_fsolver_x_upper(solver);
    status = gsl_root_test_interval(x_lo, x_hi, 1.0e-12, 0.0);
  } while (status == GSL_CONTINUE && iter < max_iter);

  gsl_root_fsolver_free(solver);

  if (status != GSL_SUCCESS) {
    throw std::runtime_error(
      "VSPump root solver did not converge within " +
      std::to_string(max_iter) + " iterations. Last approximate root: " +
      std::to_string(root));
  }

  return root;
}

// PINET uses an unbracketed fsolve for normal faces. VSPump is kept bounded.
double solve_face(FaceWrapper& fw, double x_guess) {
  if (std::dynamic_pointer_cast<VSPump>(fw.face)) {
    return solve_vspump_face_bracketed(fw, x_guess);
  }

  return solve_face_unbracketed(fw, x_guess);
}

void solve_energy_like_pinet(std::shared_ptr<Circuit> circuit, Mat A, Vec b, Vec x) {
  const PetscInt n = static_cast<PetscInt>(circuit->nodes.size());
  const double omega = 0.8;
  const double tolerance = 1.0e-8;
  const int max_iterations = 25;
  std::vector<PetscScalar> phi(n, 0.0);
  std::vector<PetscScalar> rhs(n, 0.0);
  Eigen::MatrixXd dense = Eigen::MatrixXd::Zero(n, n);
  Eigen::VectorXd rhs_vec = Eigen::VectorXd::Zero(n);
  std::vector<PetscInt> nocal_ind;

  for (auto& node : circuit->nodes) {
    PetscInt row = circuit->old2new[node->node_ind];
    phi[row] = node->tenth_gues;
    VecGetValues(b, 1, &row, &rhs[row]);
    rhs_vec(row) = rhs[row];
  }

  for (PetscInt i = 0; i < n; ++i) {
    PetscInt ncols = 0;
    const PetscInt* cols = nullptr;
    const PetscScalar* vals = nullptr;

    MatGetRow(A, i, &ncols, &cols, &vals);
    for (PetscInt j = 0; j < ncols; ++j) {
      dense(i, cols[j]) = vals[j];
    }
    MatRestoreRow(A, i, &ncols, &cols, &vals);
  }

  for (PetscInt col = 0; col < n; ++col) {
    bool any = false;
    for (PetscInt row = 0; row < n; ++row) {
      if (dense(row, col) != 0.0) {
        any = true;
        break;
      }
    }
    if (!any) {
      nocal_ind.push_back(col);
    }
  }

  std::sort(nocal_ind.begin(), nocal_ind.end());
  nocal_ind.erase(std::unique(nocal_ind.begin(), nocal_ind.end()), nocal_ind.end());

  std::vector<PetscInt> active;
  active.reserve(n);
  for (PetscInt i = 0; i < n; ++i) {
    if (!std::binary_search(nocal_ind.begin(), nocal_ind.end(), i)) {
      active.push_back(i);
    }
  }

  const PetscInt nr = static_cast<PetscInt>(active.size());
  Eigen::MatrixXd solve_mat = Eigen::MatrixXd::Zero(nr, nr);
  Eigen::VectorXd solve_rhs = Eigen::VectorXd::Zero(nr);
  std::vector<PetscScalar> solve_phi(nr, 0.0);

  for (PetscInt i = 0; i < nr; ++i) {
    solve_rhs(i) = rhs_vec(active[i]);
    solve_phi[i] = phi[active[i]];
    for (PetscInt j = 0; j < nr; ++j) {
      solve_mat(i, j) = dense(active[i], active[j]);
    }
  }

  if (nr == 0) {
    VecZeroEntries(x);
    VecAssemblyBegin(x);
    VecAssemblyEnd(x);
    return;
  }

  Eigen::JacobiSVD<Eigen::MatrixXd> svd(solve_mat);
  auto singular = svd.singularValues();
  double cond = std::numeric_limits<double>::infinity();
  if (singular.size() > 0 && singular(singular.size() - 1) > 0.0) {
    cond = singular(0) / singular(singular.size() - 1);
  }

  if (std::isfinite(cond) && cond < 1.0e8) {
    Eigen::VectorXd sol = solve_mat.partialPivLu().solve(solve_rhs);
    for (PetscInt i = 0; i < nr; ++i) {
      phi[active[i]] = sol(i);
    }
  } else {
    for (int iter = 0; iter < max_iterations; ++iter) {
      for (PetscInt i = 0; i < nr; ++i) {
        double diag = solve_mat(i, i);
        double sigma = 0.0;
        for (PetscInt j = 0; j < nr; ++j) {
          if (j != i) {
            sigma += solve_mat(i, j) * solve_phi[j];
          }
        }

        if (diag != 0.0) {
          solve_phi[i] = (1.0 - omega) * solve_phi[i] + (omega / diag) * (solve_rhs(i) - sigma);
        }
      }

      double residual = (solve_mat * Eigen::Map<Eigen::VectorXd>(solve_phi.data(), nr) - solve_rhs).norm();
      if (residual < tolerance) {
        break;
      }
    }
    for (PetscInt i = 0; i < nr; ++i) {
      phi[active[i]] = solve_phi[i];
    }
  }

  for (auto i : nocal_ind) {
    auto& node = circuit->nodes[i];
    phi[i] = node->tenth_gues;
  }

  VecZeroEntries(x);
  for (PetscInt i = 0; i < n; ++i) {
    VecSetValue(x, i, phi[i], INSERT_VALUES);
  }
  VecAssemblyBegin(x);
  VecAssemblyEnd(x);
}

void guess_flow(double time, double delt, bool trans_sim, double alpha_mom, int main_iter, std::shared_ptr<Circuit> circuit) {
  std::ofstream fout;
  if (settings::verbosity >= 6)
    std::ofstream fout("vflow_rank" + std::to_string(mpi::rank) + ".txt");
  for (auto& face : circuit->faces_owned) {
    auto orifice = std::dynamic_pointer_cast<Orifice>(face);
    if (!orifice) continue;

    orifice->choked = false;
    if (circuit->fllib == "CoolProp" && circuit->flname != "Air" && circuit->flname != "Nitrogen") {
      orifice->update_Gcr();
      if (orifice->dnode->spres_gues < orifice->pcr) {
        orifice->choked = true;
        apply_choked_orifice_state(orifice);
      }
    }
  }
  // for (auto& branch : circuit->branches) { // Guess flow rate calculation
  // for (auto& face : circuit->faces_owned) {
    // branch.choked = false;
    // for (auto face = branch.faces.rbegin(); face != branch.faces.rend(); ++face) { // Reverse iteration
      // face.choked = false;
      // if (circuit->fllib == "CoolProp" && circuit->flname != "Air" && circuit->flname != "Nitrogen") {
        // face.update_Gcr();
      // }
      // if (!branch.choked && face.dnode->spres_gues < face.pcr && dynamic_cast<turbo_comp::Turbine*>(&face) == nullptr) {
        // branch.choked = true;
        // face.choked = true;
        // face.spres_gues = face.pcr;
        // face.stemp_gues = face.ther_cr.T();
        // face.ther_gues.update(face.ther_cr);
        // face.velocity = face.Gcr / face.ther_gues.rhomass();
        // face.tpres_gues = face.spres_gues + 0.5 * face.ther_gues.rhomass() * std::pow(face.velocity, 2);
        // face.ttemp_gues = face.stemp_gues + 0.5 * std::pow(face.velocity, 2) / face.ther_gues.cpmass();
        // for (int i = 0; i < 100; ++i) {
          // face.unode->update_staticvar();
          // face.unode->ther_gues.update(CoolProp::HmassP_INPUTS, face.unode->senth_gues, face.unode->spres_gues);
          // face.dnode->update_staticvar();
          // face.dnode->ther_gues.update(CoolProp::HmassP_INPUTS, face.dnode->senth_gues, face.dnode->spres_gues);
        // }
        // face.G = face.Gcr;
        // face.vflow_gues = face.G * face.cfarea * face.opening / face.ther_gues.rhomass();
      // }
  // }
// }
// }
  

  PetscInt n_faces_owned = circuit->face_indices_owned.size();

  #pragma omp parallel for
  for (PetscInt i = 0; i < n_faces_owned; ++i) {
    FaceWrapper fw {circuit->faces_owned[i], time, delt, trans_sim, alpha_mom,main_iter};

    if (auto pface = std::dynamic_pointer_cast<PFace>(circuit->faces_owned[i])) {
      pface->choked = false;
      if (circuit->fllib == "CoolProp" && circuit->flname != "Air" && circuit->flname != "Nitrogen") {
        pface->update_Gcr();
        if (pface->dnode->spres_gues < pface->pcr) {
          pface->choked = true;
          apply_choked_pipe_state(pface);
          pface->update_abcoef(time, delt, trans_sim, alpha_mom);
          continue;
        }
      }
    }

    double guess = circuit->faces_owned[i]->vflow_gues;
    double root;
    // try {
      root = solve_face(fw, guess);

    // } catch (const std::runtime_error& e) {
    //   std::cerr << "Solver error at face " << fw.face->faceno << ": " << e.what() << "\n";
    //   // handle error: reduce timestep, skip this face, etc.
    // }

    circuit->faces_owned[i]->vflow_gues = root;
	// std::cout << std::defaultfloat << std::setprecision(10) << "faceno= " << circuit->faces_owned[i]->faceno << " delz = "   << circuit->faces_owned[i]->delz << std::endl;

//        auto pface = std::static_pointer_cast<PFace>(face);
//        std::cout << std::defaultfloat << std::setprecision(10) << "pdnode = "   << pface->dnode->tpres_gues << std::endl;
  
        // if (face.opening == 0.0) continue;
        // if (branch.isolated && !trans_sim) {
          // face.vflow_gues = 0.0;
          // continue;
        // }

    // Apply your small-value correction. Tune the value 1.E-8 as needed
    if (std::abs(circuit->faces_owned[i]->vflow_gues) < 1.E-8 && main_iter == 0) {
      auto& g = circuit->faces_owned[i]->vflow_gues;
      g = 1.E-8 * std::copysign(1.0, g);
      if (g == 0.0) g = 1.E-8;
    }
        // std::cout << face.vflow_gues << std::endl;
        // if (dynamic_cast<PFace*>(&face) != nullptr || dynamic_cast<or_comp::Orifice*>(&face) != nullptr) {
          // face.G = face.vflow_gues * face.ther_gues.rhomass() / (face.cfarea * face.opening);
        // }
      // }
      circuit->faces_owned[i]->update_abcoef(time, delt, trans_sim, alpha_mom);
      if (settings::verbosity >= 6)
        fout << "face " << circuit->faces_owned[i]->faceno << " " << std::setprecision(12) << std::fixed
       << " vflow_gues " << circuit->faces_owned[i]->vflow_gues << " vflow_old " << circuit->faces_owned[i]->vflow_old << "\n";

      // std::cout << face->vflow_gues << std::endl;
}
  if (settings::verbosity >= 6)
    fout.close();

}

void update_ghost_face(std::shared_ptr<Circuit> circuit) {
  PetscInt n_faces_owned = circuit->face_indices_owned.size();
  PetscInt n_faces_ghost = circuit->ghost_face_indices_owned.size();
  
  auto &vflow_gues_local = circuit->vflow_gues_local;
  auto &aminus_local = circuit->aminus_local;
  auto &aplus_local  = circuit->aplus_local;
  auto &bminus_local = circuit->bminus_local;
  auto &bplus_local  = circuit->bplus_local;
  
  // ---------------------
  // Fill owned values
  // ---------------------
  PetscScalar* vflow_array;
  VecGetArray(vflow_gues_local, &vflow_array);
  for (PetscInt i = 0; i < n_faces_owned; ++i) {
      vflow_array[i] = circuit->faces_owned[i]->vflow_gues;
  }
  VecRestoreArray(vflow_gues_local, &vflow_array);
  
  PetscScalar* aminus_array;
  VecGetArray(aminus_local, &aminus_array);
  for (PetscInt i = 0; i < n_faces_owned; ++i) {
      aminus_array[i] = circuit->faces_owned[i]->aminus;
  }
  VecRestoreArray(aminus_local, &aminus_array);
  
  PetscScalar* aplus_array;
  VecGetArray(aplus_local, &aplus_array);
  for (PetscInt i = 0; i < n_faces_owned; ++i) {
      aplus_array[i] = circuit->faces_owned[i]->aplus;
  }
  VecRestoreArray(aplus_local, &aplus_array);
  
  PetscScalar* bplus_array;
  VecGetArray(bplus_local, &bplus_array);
  for (PetscInt i = 0; i < n_faces_owned; ++i) {
      bplus_array[i] = circuit->faces_owned[i]->bplus;
  }
  VecRestoreArray(bplus_local, &bplus_array);
  
  PetscScalar* bminus_array;
  VecGetArray(bminus_local, &bminus_array);
  for (PetscInt i = 0; i < n_faces_owned; ++i) {
      bminus_array[i] = circuit->faces_owned[i]->bminus;
  }
  VecRestoreArray(bminus_local, &bminus_array);
  
  // ---------------------
  // Ghost updates
  // ---------------------
  VecGhostUpdateBegin(vflow_gues_local, INSERT_VALUES, SCATTER_FORWARD);
  VecGhostUpdateEnd(vflow_gues_local, INSERT_VALUES, SCATTER_FORWARD);
  
  VecGhostUpdateBegin(aminus_local, INSERT_VALUES, SCATTER_FORWARD);
  VecGhostUpdateEnd(aminus_local, INSERT_VALUES, SCATTER_FORWARD);
  
  VecGhostUpdateBegin(aplus_local, INSERT_VALUES, SCATTER_FORWARD);
  VecGhostUpdateEnd(aplus_local, INSERT_VALUES, SCATTER_FORWARD);
  
  VecGhostUpdateBegin(bplus_local, INSERT_VALUES, SCATTER_FORWARD);
  VecGhostUpdateEnd(bplus_local, INSERT_VALUES, SCATTER_FORWARD);
  
  VecGhostUpdateBegin(bminus_local, INSERT_VALUES, SCATTER_FORWARD);
  VecGhostUpdateEnd(bminus_local, INSERT_VALUES, SCATTER_FORWARD);
  
  // ---------------------
  // Read ghost values
  // ---------------------
  const PetscScalar* vflow_array_read;
  VecGetArrayRead(vflow_gues_local, &vflow_array_read);
  
  const PetscScalar* aminus_array_read;
  VecGetArrayRead(aminus_local, &aminus_array_read);
  
  const PetscScalar* aplus_array_read;
  VecGetArrayRead(aplus_local, &aplus_array_read);
  
  const PetscScalar* bplus_array_read;
  VecGetArrayRead(bplus_local, &bplus_array_read);
  
  const PetscScalar* bminus_array_read;
  VecGetArrayRead(bminus_local, &bminus_array_read);
  
  for (size_t j = 0; j < circuit->ghost_face_indices_owned.size(); ++j) {
    auto idx = circuit->ghost_face_indices_owned[j];
    auto& face = circuit->faces[idx];
    face->vflow_gues = vflow_array_read[n_faces_owned + j];
    face->aminus     = aminus_array_read[n_faces_owned + j];
    face->aplus      = aplus_array_read[n_faces_owned + j];
    face->bplus      = bplus_array_read[n_faces_owned + j];
    face->bminus     = bminus_array_read[n_faces_owned + j];
  
    // std::cout << "flag1 " << mpi::rank
    // << std::setprecision(12) << std::fixed
              // << " face=" << face->faceno
              // << " aminus=" << face->aminus
              // << " aplus=" << face->aplus
              // << " bplus=" << face->bplus
              // << " bminus=" << face->bminus
              // << " vflow_gues=" << face->vflow_gues
              // << " vflow_old=" << face->vflow_old
              // << std::endl;
  }
  
  VecRestoreArrayRead(vflow_gues_local, &vflow_array_read);
  VecRestoreArrayRead(aminus_local, &aminus_array_read);
  VecRestoreArrayRead(aplus_local, &aplus_array_read);
  VecRestoreArrayRead(bplus_local, &bplus_array_read);
  VecRestoreArrayRead(bminus_local, &bminus_array_read);

}
  
void exec_massmom(double time, double delt, bool trans_sim, double alpha_mom, int main_iter, int flow_iter) {
  
  simulation::time_massmom.start();

  for (auto& circuit : model::circuits_owned) {
    // if (!trans_sim && !circuit->solveSS) continue;
    // std::cout << circuit->identifier << std::endl;
	simulation::time_guess_flow.start();
    guess_flow(time, delt, trans_sim, alpha_mom, main_iter, circuit);
    update_ghost_face(circuit);
	simulation::time_guess_flow.stop();
    // std::ofstream fout("abcoef_rank_" + std::to_string(mpi::rank) + ".txt");

	simulation::time_pressure_correction.start();
    simulation::time_pc_assembly.start();
    // Pressure corrections
    auto &A = circuit->A; 
    auto &b = circuit->b; 
    auto &pc = circuit->pc; 

    MatZeroEntries(A);
    VecZeroEntries(b);
    VecZeroEntries(pc);

// #pragma omp parallel
// {
//   if (omp_get_thread_num() == 0)
//     std::cout << "Threads used: " << omp_get_num_threads() << std::endl;
// }
// #pragma omp parallel for
// for (int i = 0; i < 8; ++i) {
//   printf("Thread %d processing %d\n", omp_get_thread_num(), i);
// }

    // #pragma omp parallel for
    for (size_t n = 0; n < circuit->nodes_owned.size(); ++n) {
      auto& node = circuit->nodes_owned[n];
      int i = circuit->old2new[node->node_ind];  // global row index
	  bool pbound = node->fixed_var.count("P");
      double A_local_node = 0, A_local_iface, A_local_oface;
      double b_local;

      double B, D;
      if (circuit->fltype != FluidType::INCOMPRESSIBLE && node->ther_old->phase() == 6) {
        B = node->B1 + node->volume * node->ther_old->first_two_phase_deriv(CoolProp::iDmass, CoolProp::iP, CoolProp::iHmass) / node->ther_old->rhomass();
        if (!pbound)
          A_local_node = trans_sim * B * node->ther_old->rhomass() / delt;
        D = trans_sim * node->volume * node->ther_old->first_two_phase_deriv(CoolProp::iDmass, CoolProp::iHmass, CoolProp::iP);
      } else {
        B = node->B1 + node->volume * node->ther_old->first_partial_deriv(CoolProp::iDmass, CoolProp::iP, CoolProp::iHmass) / node->ther_old->rhomass();
        if (!pbound)
          A_local_node = trans_sim * B * node->ther_old->rhomass() / delt;
        D = trans_sim * node->volume * node->ther_old->first_partial_deriv(CoolProp::iDmass, CoolProp::iHmass, CoolProp::iP);
      }
      b_local = -trans_sim * B * node->ther_old->rhomass() / delt * (node->tpres_gues - node->ther_gues->rhomass() * std::pow(node->velocity, 2) / 2.0 - node->spres_old)
           - trans_sim * D * (node->senth_gues - node->senth_old) / delt;

      for (auto& iface : node->ifaces) {
        if (!pbound) {
          A_local_iface = -alpha_mom * (iface->aminus * iface->ther_gues->rhomass() + iface->bminus * iface->vflow_gues);
          int j = circuit->old2new[iface->unode->node_ind];
          MatSetValue(A, i, j, A_local_iface, INSERT_VALUES);
          if (A_local_iface > 0.0) {
            // if ((show_warn && trans_sim) || !trans_sim) {
              std::cout << "Warning: upstream coef negative. " << node->identifier << std::endl;
            // }
          }
          A_local_node = A_local_node - alpha_mom * (-iface->aplus * iface->ther_gues->rhomass() + iface->bplus * iface->vflow_gues);
        }
        b_local += alpha_mom * (iface->ther_gues->rhomass() * iface->vflow_gues) + (1.0 - alpha_mom) * (iface->ther_old->rhomass() * iface->vflow_old);
        // std::cout << "b_local " << iface->faceno << " " << iface->vflow_gues << std::endl;
      }

      for (auto& oface : node->ofaces) {
    	if (!pbound) {
          A_local_oface = -alpha_mom * (oface->aplus * oface->ther_gues->rhomass() - oface->bplus * oface->vflow_gues);
          int j = circuit->old2new[oface->dnode->node_ind];
          MatSetValue(A, i, j, A_local_oface, INSERT_VALUES);
          if (A_local_oface > 1.E-6) { // Pending check if 0
            // if ((show_warn && trans_sim) || !trans_sim) {
              std::cout << "Warning: downstream coef negative. " << node->identifier << std::endl;
            // }
          }
          A_local_node = A_local_node + alpha_mom * (oface->aminus * oface->ther_gues->rhomass() + oface->bminus * oface->vflow_gues);
        }
        b_local = b_local - alpha_mom * (oface->ther_gues->rhomass() * oface->vflow_gues) - (1.0 - alpha_mom) * (oface->ther_old->rhomass() * oface->vflow_old);
      }

      // if (node.fixed_var.count("P") && !dynamic_cast<cont.Reservoir*>(node)) {
      if (node->fixed_var.count("P")) {
        double isum_gues = 0.0;
        double osum_gues = 0.0;
        for (auto& iface : node->ifaces) {
          isum_gues += iface->ther_gues->rhomass() * iface->vflow_gues;
        }
        for (auto& oface : node->ofaces) {
          osum_gues += oface->ther_gues->rhomass() * oface->vflow_gues;
        }
        node->msource = osum_gues - isum_gues;
        A_local_node = 1.0;
		b_local = 0.0;
        
      } else if (node->fixed_var.count("msource")) {
  //       if (time <= 20) {
  //           node->msource = -753.6*(20.-time)/20.;
		// }
  //       else {
  //           node->msource = 0.;
		// }
        b_local += node->msource;
      }

      if (A_local_node < -1.E-6) { // Pending check if 0
        // if ((show_warn && trans_sim) || !trans_sim) {
          std::cout << "Warning: negative A coef. " << node->identifier << " " << A_local_node << std::endl;
        // }
      }

      MatSetValue(A, i, i, A_local_node, INSERT_VALUES);
      VecSetValue(b, i, b_local, INSERT_VALUES);

    }

    // fout.close();


    MatAssemblyBegin(A, MAT_FINAL_ASSEMBLY);
    MatAssemblyEnd(A, MAT_FINAL_ASSEMBLY);
    
    VecAssemblyBegin(b);
    VecAssemblyEnd(b);
    simulation::time_pc_assembly.stop();



     // Print matrix A
    // PetscViewer viewerA;
    // PetscViewerASCIIOpen(PETSC_COMM_WORLD, "matrix_A.txt", &viewerA);
    // PetscViewerPushFormat(viewerA, PETSC_VIEWER_ASCII_DENSE); // optional: DENSE format
    // MatView(A, viewerA);
    // PetscViewerPopFormat(viewerA);
    // PetscViewerDestroy(&viewerA);
    
    // Print vector b
    // PetscViewer viewerB;
    // PetscViewerASCIIOpen(PETSC_COMM_WORLD, "vector_b.txt", &viewerB);
    // VecView(b, viewerB);
    // PetscViewerDestroy(&viewerB);

    simulation::time_pc_solve.start();
    KSPSetOperators(circuit->ksp, A, A);
    KSPSetType(circuit->ksp, KSPPREONLY);
    PC pc_solver;
    KSPGetPC(circuit->ksp, &pc_solver);
    PCSetType(pc_solver, PCLU);
    KSPSetUp(circuit->ksp);
    KSPSolve(circuit->ksp, b, pc);
    simulation::time_pc_solve.stop();

    // PetscViewer viewer;
    // PetscViewerASCIIOpen(PETSC_COMM_WORLD, "pc_output.txt", &viewer);
    // VecView(pc, viewer);
    // PetscViewerDestroy(&viewer);
    //
    // MPI_Abort(mpi::intracomm, 0);
    // std::exit(0);
       // if (flow_iter == 1) {
       //   MPI_Abort(mpi::intracomm, 0);
       //   std::exit(0);
       // }

    simulation::time_pc_update.start();
    simulation::time_pc_update_a.start();
    // Prepare file for writing (one file per rank)
    std::ofstream fout;
    if (settings::verbosity >= 6)
      std::ofstream fout("vflow_rank_" + std::to_string(mpi::rank) + ".txt");
    
    // Create ghost vector
    PetscInt n_local = circuit->indices_owned.size();
    PetscInt nghost  = circuit->ghost_indices_owned.size();
    
    auto &pc_local = circuit->pc_local;
    
    PetscScalar* loc = nullptr;
    VecGetArray(pc_local, &loc);
    
    // fill owned slots [0..n_local-1] from global 'pc'
    std::vector<PetscScalar> vals_owned(n_local);
    VecGetValues(pc, n_local, circuit->indices_owned.data(), vals_owned.data());
    for (PetscInt i = 0; i < n_local; ++i) loc[i] = vals_owned[i];
    
    VecRestoreArray(pc_local, &loc);
    
    // Scatter from global solution 'pc' to ghosted vector
    VecGhostUpdateBegin(pc_local, INSERT_VALUES, SCATTER_FORWARD);
    VecGhostUpdateEnd(pc_local, INSERT_VALUES, SCATTER_FORWARD);
    
    const PetscScalar* pc_array;
    VecGetArrayRead(pc_local, &pc_array);
    
    std::unordered_map<PetscInt, PetscInt> global_to_local;
    
    for (PetscInt i = 0; i < n_local; ++i)
      global_to_local[circuit->indices_owned[i]] = i;
    
    for (PetscInt j = 0; j < nghost; ++j)
      global_to_local[circuit->ghost_indices_owned[j]] = n_local + j;

    // Flow rate corrections
    for (auto& face : circuit->faces_owned) {
      if (!face->choked) {

      PetscInt i_u = global_to_local[circuit->old2new[face->unode->node_ind]]; // could be owned or ghost
      PetscInt i_v = global_to_local[circuit->old2new[face->dnode->node_ind]];


      double vc = face->aminus * pc_array[i_u] - face->aplus * pc_array[i_v];
      face->vflow_gues += vc;
      // fout << "face " << face->faceno << " " << std::setprecision(8) << std::fixed << face->vflow_gues << "\n";

      }
      face->update_velocity();
    }
    simulation::time_pc_update_a.stop();

    simulation::time_pc_update_b.start();

    PetscInt n_faces_owned = circuit->face_indices_owned.size();
    PetscInt n_faces_ghost = circuit->ghost_face_indices_owned.size();

    auto &vflow_gues_local = circuit->vflow_gues_local;

    PetscScalar* vflow_array;
    VecGetArray(vflow_gues_local, &vflow_array);
    for (PetscInt i = 0; i < n_faces_owned; ++i) {
        vflow_array[i] = circuit->faces_owned[i]->vflow_gues;
    }
    VecRestoreArray(vflow_gues_local, &vflow_array);

    VecGhostUpdateBegin(vflow_gues_local, INSERT_VALUES, SCATTER_FORWARD);
    VecGhostUpdateEnd(vflow_gues_local, INSERT_VALUES, SCATTER_FORWARD);

    const PetscScalar* vflow_array_read;
    VecGetArrayRead(vflow_gues_local, &vflow_array_read);

    for (size_t j = 0; j < circuit->ghost_face_indices_owned.size(); ++j) {
      auto idx = circuit->ghost_face_indices_owned[j];
      auto& face = circuit->faces[idx];
      face->vflow_gues = vflow_array_read[n_faces_owned + j];
      face->update_velocity();
    }

    VecRestoreArrayRead(vflow_gues_local, &vflow_array_read);
    simulation::time_pc_update_b.stop();

    simulation::time_pc_update_cd.start();

    // Pressure and density corrections
    #pragma omp parallel for
    for (size_t n = 0; n < circuit->nodes_owned.size(); ++n) {
      auto& node = circuit->nodes_owned[n];
      double relax = settings::relax_pres;
      node->tpres_gues += relax * pc_array[n];
      // if (node->identifier == "node2") {
      //  std::cout << "rank " << mpi::rank << " tpres " << node->tpres_gues << std::endl;
      // }
    
      // if (node.flowreg == "Slug") continue;
      // node->msource = m_array[i];
      // fout << "node " << node->identifier << "msource " << node->msource << std::endl;
      // fout << "node " << i
      //      << " tpres " << node->tpres_gues << "\n";
      if (node->tpres_gues < 0.0) {
        std::cerr << "Negative tpres " << node->identifier << " " << node->tpres_gues << " " << node->tpres_old << std::endl;
        std::cerr << pc << std::endl;
        std::exit(EXIT_FAILURE);
      }
    
      node->update_staticvar();
    
      // if (main_iter == 0) node->pc_flag = false;
      node->ther_gues->update(CoolProp::HmassP_INPUTS, node->senth_gues, node->spres_gues);
      // if (circuit.flag_tp || dynamic_cast<cont::TPTank*>(&node) != nullptr) {
        // node.ther_gues.update_sat();
      // }
      // if (trans_sim && (node.pc_flag || node.ther_gues.phase() != node.ther_old.phase())) {
        // node.pc_flag = true;
      // }
    }
    // if (trans_sim) {
      // std::exit(1);
    // }
    simulation::time_pc_update_cd.stop();

    simulation::time_pc_update_ef.start();
    auto &velocity_local = circuit->velocity_local;

    PetscScalar* velocity_arr = nullptr;
    VecGetArray(velocity_local, &velocity_arr);

    for (PetscInt i = 0; i < n_local; ++i)
      velocity_arr[i] = circuit->nodes_owned[i]->velocity;

    VecRestoreArray(velocity_local, &velocity_arr);

    VecGhostUpdateBegin(velocity_local, INSERT_VALUES, SCATTER_FORWARD);
    VecGhostUpdateEnd(velocity_local, INSERT_VALUES, SCATTER_FORWARD);

    const PetscScalar* velocity_arr_read;
    VecGetArrayRead(velocity_local, &velocity_arr_read);

    PetscInt offset = n_local;
    for (PetscInt j = 0; j < nghost; ++j)
      circuit->ghost_nodes_owned1[j]->velocity = velocity_arr_read[offset + j];

    VecRestoreArrayRead(velocity_local, &velocity_arr_read);


    // #pragma omp parallel for
    for (size_t i = 0; i < circuit->ghost_nodes_owned1.size(); ++i) {
      auto& node = circuit->ghost_nodes_owned1[i];
      double relax = settings::relax_pres;
      node->tpres_gues = node->tpres_gues + relax * pc_array[circuit->nodes_owned.size()+i];
      // std::cout << std::defaultfloat << std::setprecision(15) << "flag2 rank " << mpi::rank << " tpres_gues " << node->tpres_gues << " tpres_old " << node->tpres_old << std::endl;
      node->update_staticvar(node->velocity);
      node->ther_gues->update(CoolProp::HmassP_INPUTS, node->senth_gues, node->spres_gues);
    }


    // fout.close();

    if (settings::verbosity >= 6) {
      std::ofstream fout1("tpres_rank_" + std::to_string(mpi::rank) + ".txt");
      for (auto& node : circuit->nodes_owned) {
        fout1 << node->identifier << " " << std::setprecision(8) << std::fixed << node->tpres_gues << " " << node->tpres_old << "\n";
      }
      for (auto& node : circuit->ghost_nodes_owned1) {
        fout1 << "(ghost) " << node->identifier << " " << node->tpres_gues << " " << node->tpres_old << "\n";
      }
      fout1.close();
    }

    VecRestoreArrayRead(pc_local, &pc_array);
    simulation::time_pc_update_ef.stop();

    simulation::time_pc_update_g.start();
    #pragma omp parallel for
    for (size_t i = 0; i < circuit->faces_owned.size(); ++i) {
      auto& face = circuit->faces_owned[i];
      // if (!face->choked) {
        face->update_statevar();
        face->ther_gues->update();
        // if (circuit.flag_tp) face->ther_gues->update_sat();
        face->update_heat_input(); //(time, delt)
        face->update_fricfact();
      // } else {
        // face->update_Gcr();
        // face->G = std::copysign(face->Gcr, face->vflow_gues);
        // face->vflow_gues = face->G * face->cfarea / face->ther_gues.rhomass();
      // }
    }
    simulation::time_pc_update_g.stop();

    simulation::time_pc_update_h.start();
    
    auto &rhomass_local = circuit->rhomass_local;
    
    PetscScalar* rhomass_array;
    VecGetArray(rhomass_local, &rhomass_array);
    for (PetscInt i = 0; i < n_faces_owned; ++i) {
        rhomass_array[i] = circuit->faces_owned[i]->ther_gues->rhomass();
    }
    VecRestoreArray(rhomass_local, &rhomass_array);
    
    VecGhostUpdateBegin(rhomass_local, INSERT_VALUES, SCATTER_FORWARD);
    VecGhostUpdateEnd(rhomass_local, INSERT_VALUES, SCATTER_FORWARD);
    
    const PetscScalar* rhomass_array_read;
    VecGetArrayRead(rhomass_local, &rhomass_array_read);
    
    for (size_t j = 0; j < circuit->ghost_face_indices_owned.size(); ++j) {
      auto idx = circuit->ghost_face_indices_owned[j];
      auto& face = circuit->faces[idx];
      face->ther_gues->set_rhomass(rhomass_array_read[n_faces_owned + j]);
    
      // std::cout << "flag1 " << mpi::rank
      //           << " face=" << face->faceno
      //           << " vflow_gues=" << face->vflow_gues
      //           << " rhomass=" << face->ther_gues->rhomass()
      //           << std::endl;

    }
    
    if (settings::verbosity >= 6) {
      for (auto& face : circuit->faces_owned) {
        fout << "face " << face->faceno << " " << std::setprecision(12) << std::fixed << face->velocity << std::endl;
      }
      fout.close();
    }

    VecRestoreArrayRead(rhomass_local, &rhomass_array_read);

    simulation::time_pc_update_h.stop();

    simulation::time_pc_update.stop();

    simulation::time_pressure_correction.stop();
  }
  // std::exit(0);
  simulation::time_massmom.stop();
}

void exec_energy(double time, double delt, bool trans_sim, double alpha_ener, int main_iter) {
  
  simulation::time_fluid_energy.start();
  for (auto& circuit : model::circuits_owned) {
    // if (!trans_sim && !circuit.solveSS) continue;

    int n = circuit->nodes.size();

    // vector<int> nocal_ind;
    auto &Ah = circuit->Ah;
    auto &bh = circuit->bh;
    auto &enth = circuit->enth;

    MatZeroEntries(Ah);
    VecZeroEntries(bh);
    VecZeroEntries(enth);
    for (auto& node : circuit->nodes_owned) {
      int i = circuit->old2new[node->node_ind];  // global row index
	  // bool pbound = node->fixed_var.count("P");
      double A_local_node = 0, A_local_iface, A_local_oface;
      double b_local;

      double C = node->volume * node->ther_old->rhomass();
      double E = node->volume;
      A_local_node = trans_sim * C / delt;
      b_local = node->tenth_old * (trans_sim * C / delt)
                + trans_sim * E * (node->spres_gues - node->spres_old) / delt
                + node->heat_input
                + std::accumulate(node->heat_hslab.begin(), node->heat_hslab.end(), 0.0);
      b_local = b_local - node->tenth_old * node->msource * trans_sim;

      for (auto& iface : node->ifaces) {
        if (iface->dnode->is_reservoir && iface->dfrac >= 0.0
            && iface->dnode->ther_gues->phase() == 6) {
          b_local -= alpha_ener * iface->downstream->tenth_gues
                   * std::max(-iface->ther_gues->rhomass() * iface->vflow_gues, 0.0);
        } else {
          A_local_node = A_local_node + alpha_ener * std::max(-iface->ther_gues->rhomass() * iface->vflow_gues, 0.0);
        }

        if (iface->unode->is_reservoir && iface->ufrac >= 0.0
            && iface->unode->ther_gues->phase() == 6) {
          b_local += alpha_ener * iface->upstream->tenth_gues
                   * std::max(iface->ther_gues->rhomass() * iface->vflow_gues, 0.0);
        } else {
          A_local_iface = -alpha_ener * std::max(iface->ther_gues->rhomass() * iface->vflow_gues, 0.0);
		  int j = circuit->old2new[iface->unode->node_ind];
		  MatSetValue(Ah, i, j, A_local_iface, INSERT_VALUES);
        }
		
        b_local = (b_local 
                  - iface->downstream->tenth_old * (1.0 - alpha_ener) 
                    * std::max(-iface->ther_old->rhomass() * iface->vflow_old, 0.0)
                  + iface->upstream->tenth_old * (1.0 - alpha_ener) 
                    * std::max(iface->ther_old->rhomass() * iface->vflow_old, 0.0));
        
        b_local = (b_local
                  + alpha_ener * (iface->heat_input + std::accumulate(iface->heat_hslab.begin(), iface->heat_hslab.end(), 0.0))
                    * (iface->vflow_gues > 0.0 ? 1.0 : 0.0)
                  + (1.0 - alpha_ener) * (iface->heat_input_old + std::accumulate(iface->heat_hslab_old.begin(), iface->heat_hslab_old.end(), 0.0))
                    * (iface->vflow_old > 0.0 ? 1.0 : 0.0));
        
        b_local = (b_local 
                  - node->tenth_old * alpha_ener * iface->ther_gues->rhomass() 
                    * iface->vflow_gues * trans_sim
                  - node->tenth_old * (1.0 - alpha_ener) 
                    * iface->ther_old->rhomass() * iface->vflow_old * trans_sim);
        
      }

      for (auto& oface : node->ofaces) {
        if (oface->unode->is_reservoir && oface->ufrac >= 0.0
            && oface->unode->ther_gues->phase() == 6) {
          b_local -= alpha_ener * oface->upstream->tenth_gues
                   * std::max(oface->ther_gues->rhomass() * oface->vflow_gues, 0.0);
        } else {
          A_local_node = A_local_node + alpha_ener * std::max(oface->ther_gues->rhomass() * oface->vflow_gues, 0.0);
        }

        if (oface->dnode->is_reservoir && oface->dfrac >= 0.0
            && oface->dnode->ther_gues->phase() == 6) {
          b_local += alpha_ener * oface->downstream->tenth_gues
                   * std::max(-oface->ther_gues->rhomass() * oface->vflow_gues, 0.0);
        } else {
          A_local_oface = -alpha_ener * std::max(-oface->ther_gues->rhomass() * oface->vflow_gues, 0.0);
		  int j = circuit->old2new[oface->dnode->node_ind];
		  MatSetValue(Ah, i, j, A_local_oface, INSERT_VALUES);

        }

        
        b_local = (b_local 
                  - oface->upstream->tenth_old * (1.0 - alpha_ener) 
                    * std::max(oface->ther_old->rhomass() * oface->vflow_old, 0.0)
                  + oface->downstream->tenth_old * (1.0 - alpha_ener) 
                    * std::max(-oface->ther_old->rhomass() * oface->vflow_old, 0.0));
        
        b_local = (b_local 
                  + alpha_ener * (oface->heat_input + std::accumulate(oface->heat_hslab.begin(), oface->heat_hslab.end(), 0.0))
                    * (oface->vflow_gues < 0.0 ? 1.0 : 0.0)
                  + (1.0 - alpha_ener) * (oface->heat_input_old + std::accumulate(oface->heat_hslab_old.begin(), oface->heat_hslab_old.end(), 0.0))
                    * (oface->vflow_old < 0.0 ? 1.0 : 0.0));
        
        b_local = (b_local 
                  + node->tenth_old * alpha_ener * oface->ther_gues->rhomass() 
                    * oface->vflow_gues * trans_sim
                  + node->tenth_old * (1.0 - alpha_ener) 
                    * oface->ther_old->rhomass() * oface->vflow_old * trans_sim);
        
      }
	  
	  MatSetValue(Ah, i, i, A_local_node, INSERT_VALUES);
      VecSetValue(bh, i, b_local, INSERT_VALUES);
	  
    }

    MatAssemblyBegin(Ah, MAT_FINAL_ASSEMBLY);
    MatAssemblyEnd(Ah, MAT_FINAL_ASSEMBLY);
    
    VecAssemblyBegin(bh);
    VecAssemblyEnd(bh);
    
    for (auto& node : circuit->nodes_owned) {
      int i = circuit->old2new[node->node_ind];
    
      // --- Case 1: Temperature fixed ---
      if (node->fixed_var.count("T")) {
        // Update thermodynamic state based on T
        node->update_statictemp();
        node->ther_gues->update(CoolProp::PT_INPUTS,
                               node->spres_gues,
                               node->stemp_gues);
        node->senth_gues = node->ther_gues->hmass();
        node->ther_gues->update(CoolProp::HmassP_INPUTS,
                               node->senth_gues,
                               node->spres_gues);
    
        // if (circuit->flag_tp || node->isTPTank()) {
          // node->ther_gues->update_sat();
        // }
    
        node->update_totalenth();
    
        // Save row info (optional, for debugging or post-processing)
        node->brow = 0.0;
        {
          PetscInt row = i;
          PetscInt ncols;
          const PetscInt *cols;
          const PetscScalar *vals;
          MatGetRow(Ah, row, &ncols, &cols, &vals);

          node->Acols.clear();
          node->Avals.clear();
          node->Acols.reserve(ncols);
          node->Avals.reserve(ncols);

          for (int k = 0; k < ncols; ++k) {
            node->Acols.push_back(cols[k]);
            node->Avals.push_back(vals[k]);
          }
          MatRestoreRow(Ah, row, &ncols, &cols, &vals);
    
          PetscScalar bi;
          VecGetValues(bh, 1, &row, &bi);
          node->brow = bi;
        }
    
        // Apply Dirichlet BC
        PetscInt row = i;
        PetscScalar diag = 1.0;
        MatZeroRows(Ah, 1, &row, diag, bh, nullptr);
        VecSetValue(bh, row, node->tenth_gues, INSERT_VALUES);
        MatAssemblyBegin(Ah, MAT_FINAL_ASSEMBLY);
        MatAssemblyEnd(Ah, MAT_FINAL_ASSEMBLY);
        VecAssemblyBegin(bh);
        VecAssemblyEnd(bh);
      }
    
      // --- Case 2: Enthalpy fixed ---
      else if (node->fixed_var.count("H")) {
        // Update thermodynamic state based on H
        node->update_staticenth();
        node->ther_gues->update(CoolProp::HmassP_INPUTS,
                               node->senth_gues,
                               node->spres_gues);
        node->stemp_gues = node->ther_gues->T();
    
        // if (circuit->flag_tp || node->isTPTank()) {
          // node->ther_gues->update_sat();
        // }
    
        node->update_totaltemp();
        node->update_staticpres();
    
        // Save row info (optional)

        node->brow = 0.0;
        {
          PetscInt row = i;
          PetscInt ncols;
          const PetscInt *cols;
          const PetscScalar *vals;
          MatGetRow(Ah, row, &ncols, &cols, &vals);

          node->Acols.clear();
          node->Avals.clear();
          node->Acols.reserve(ncols);
          node->Avals.reserve(ncols);

          for (int k = 0; k < ncols; ++k) {
            node->Acols.push_back(cols[k]);
            node->Avals.push_back(vals[k]);
          }
          MatRestoreRow(Ah, row, &ncols, &cols, &vals);
    
          PetscScalar bi;
          VecGetValues(bh, 1, &row, &bi);
          node->brow = bi;
        }
    
        // Apply Dirichlet BC
        PetscInt row = i;
        PetscScalar diag = 1.0;
        MatZeroRows(Ah, 1, &row, diag, bh, nullptr);
        VecSetValue(bh, row, node->tenth_gues, INSERT_VALUES);
        MatAssemblyBegin(Ah, MAT_FINAL_ASSEMBLY);
        MatAssemblyEnd(Ah, MAT_FINAL_ASSEMBLY);
        VecAssemblyBegin(bh);
        VecAssemblyEnd(bh);
      }
	  else if (node->fixed_var.count("msource") || node->fixed_var.count("P")) {
		if (node->msource > 0.0) {
          if (node->msource > 1.E-6) {
            std::cout << "warning: positive mass source condition assumed based on previous circuit condition "
                      << node->identifier << " tenth=" << node->tenth_gues << " " << node->msource << std::endl;
          }
          VecSetValue(bh, i, node->tenth_old * node->msource, ADD_VALUES);
        } else {
	      PetscScalar Aii;
		  PetscInt row = i, col = i;
          MatGetValues(Ah, 1, &row, 1, &col, &Aii);
          Aii = Aii - node->msource;
		  MatSetValue(Ah, row, col, Aii, INSERT_VALUES);
          MatAssemblyBegin(Ah, MAT_FINAL_ASSEMBLY);
          MatAssemblyEnd(Ah, MAT_FINAL_ASSEMBLY);
          if (Aii < 0.0) {
            std::cerr << "negative coef. in energy solver. stopping" << std::endl;
            exit(EXIT_FAILURE);
          }
        }
	  }
    }

    MatAssemblyBegin(Ah, MAT_FINAL_ASSEMBLY);
    MatAssemblyEnd(Ah, MAT_FINAL_ASSEMBLY);

    VecAssemblyBegin(bh);
    VecAssemblyEnd(bh);


    
/*     for (size_t i = 0; i < circuit->nodes.size(); ++i) {
      auto node = circuit->nodes[i];
      if (node->flowreg == "Homogeneous") {
        for (size_t j = 0; j < circuit->nodes.size(); ++j) {
          auto node1 = circuit->nodes[j];
          if (node1->flowreg == "Slug") {
            b(i) -= A(i, j) * node1->senth_gues;
          }
        }
      }
    }
 */    



    // Print matrix Ah
    // PetscViewer viewerAh;
    // PetscViewerASCIIOpen(PETSC_COMM_WORLD, "matrix_Ah.txt", &viewerAh);
    // PetscViewerPushFormat(viewerAh, PETSC_VIEWER_ASCII_DENSE); // optional: DENSE format
    // MatView(Ah, viewerAh);
    // PetscViewerPopFormat(viewerAh);
    // PetscViewerDestroy(&viewerAh);
    
    // Print vector bh
    // PetscViewer viewerbh;
    // PetscViewerASCIIOpen(PETSC_COMM_WORLD, "vector_bh.txt", &viewerbh);
    // VecView(bh, viewerbh);
    // PetscViewerDestroy(&viewerbh);


	
/*     std::vector<int> nocal_ind;
    for (int i = 0; i < A.cols(); ++i) {
      if (A.col(i).sum() == 0.0) {
        nocal_ind.push_back(i);
      }
    }
 */    
/*     // Removing disconnected nodes
    for (auto it = nocal_ind.rbegin(); it != nocal_ind.rend(); ++it) {
      A = removeRowCol(A, *it);  // removeRowCol is a custom function to remove row and column
      b = removeElement(b, *it); // removeElement is a custom function to remove elements from b
    }
 */    
// Assume A (Mat), b (Vec), and circuit->nodes already filled.
// enth (Vec) will store the solution.

  // PC pc;
  // PetscReal emin, emax, cond;
  
  // Create KSP solver
  // KSPSetOperators(circuit->ksph, A, A);
  
  // (1) Estimate condition number
  // KSPSetUp(ksp);  // must set up before computing eigenvalues
  // KSPComputeExtremeSingularValues(ksp, &emin, &emax);
  
  // if (emin > 0.0 && emax > 0.0) {
    // cond = emax / emin;
  // } else {
    // std::cerr << "Infinite condition number. Check boundary conditions." << std::endl;
    // PetscFinalize();
    // exit(EXIT_FAILURE);
  // }
  
  // (2) If condition number is reasonable -> direct LU solve
  // if (cond < 1.0e8) {
    // KSPSetType(ksp, KSPPREONLY);
    // KSPGetPC(ksp, &pc);
    // PCSetType(pc, PCLU);  // direct LU
    // KSPSetFromOptions(ksp);
  
    solve_energy_like_pinet(circuit, Ah, bh, enth);
  
  // } else {
    // (3) Else -> fallback to SOR iteration
    // KSPSetType(ksp, KSPRICHARDSON);   // basic iterative solver
    // KSPGetPC(ksp, &pc);
    // PCSetType(pc, PCSOR);
  
    // KSPSetTolerances(ksp, 1.e-8, PETSC_DEFAULT, PETSC_DEFAULT, 25);
  
    // Build initial guess enth_old from nodes
    // Vec enth_old;
    // VecDuplicate(b, &enth_old);
    // for (size_t i = 0; i < circuit->nodes.size(); ++i) {
      // double val = circuit->nodes[i]->tenth_gues;      
      // if (std::find(nocal_ind.begin(), nocal_ind.end(), i) != nocal_ind.end()) { // removeElements(enth_old, nocal_ind) equivalent:
        // continue; // skip removed indices
      // }
      // VecSetValue(enth_old, i, val, INSERT_VALUES);
    // }
    // VecAssemblyBegin(enth_old);
    // VecAssemblyEnd(enth_old);
  
    // VecDuplicate(b, &enth);
    // VecCopy(enth_old, enth); // use old guess as initial guess
  
    // KSPSolve(ksp, b, enth);
  
    // VecDestroy(&enth_old);
  // }
  

    PetscViewer viewer;
    PetscViewerASCIIOpen(PETSC_COMM_WORLD, "enth_output.txt", &viewer);
    VecView(enth, viewer);
    PetscViewerDestroy(&viewer);
	


    std::ofstream fout;
    if (settings::verbosity >= 6)
      std::ofstream fout2("tenth_rank_" + std::to_string(mpi::rank) + ".txt");
    
    // Create ghost vector
    // PetscInt n_local = circuit->indices_owned.size();
    // PetscInt nghost  = circuit->ghost_indices_owned.size();
    
    // std::vector<PetscScalar> vals_owned(n_local);
    // VecGetValues(enth, n_local, circuit->indices_owned.data(), vals_owned.data());
	
    double relax = settings::relax_enth;
    if (!model::hslabs.empty() && !trans_sim) {
      relax = std::min(relax, 0.25);
    }
	PetscScalar enth_i;
    for (auto& node : circuit->nodes_owned) {
      int i = circuit->old2new[node->node_ind];
	  
	  if (node->fixed_var.count("T") ||
          node->fixed_var.count("H") ) {
        continue;
      }
	  
      VecGetValues(enth, 1, (PetscInt*)&i, &enth_i);
	  node->tenth_gues = (1.0 - relax) * node->tenth_gues + relax * enth_i;
	  // if (settings::verbosity >= 6)
	  // fout2 << "node " << node->identifier << " " << std::setprecision(8) << std::fixed << node->tenth_gues << "\n";
    }
	
	
	for (auto& node : circuit->nodes_owned) {
      int i = circuit->old2new[node->node_ind];
	
	  if (node->fixed_var.count("T") ||
          node->fixed_var.count("H") ) {
        double sum_A_tenth = 0.0;
        for (size_t k = 0; k < node->Avals.size(); ++k) {
          int col = node->Acols[k];
          sum_A_tenth += node->Avals[k] * circuit->nodes[col]->tenth_gues;
//          std::cout << "flag4 " << node->identifier << " " << sum_A_tenth << std::endl;
        }
      
        node->esource = sum_A_tenth - node->brow - node->tenth_gues * node->msource;
        // std::cout << "node " << node->identifier << " " << std::setprecision(8) << std::fixed << node->esource << "\n";
      }

	}
	// if (settings::verbosity >= 6)
	  // fout2.close();

	for (auto& node : circuit->nodes_owned) {
      int i = circuit->old2new[node->node_ind];

      // Skip T/H fixed nodes for static enthalpy update
      bool fixedT = node->fixed_var.count("T");
      bool fixedH = node->fixed_var.count("H");

      if (!fixedT && !fixedH) {
        node->update_staticenth();
        node->ther_gues->update(CoolProp::HmassP_INPUTS,
                                node->senth_gues,
                                node->spres_gues);

        // if (circuit->flag_tp || dynamic_cast<TPTank*>(node.get()))
        //   node->ther_gues.update_sat();
      }

      // if (trans_sim && (node->pc_flag ||
      //                   node->ther_gues.phase() != node->ther_old.phase())) {
      //   node->pc_flag = true;

          // optional phase-change relaxation (commented as in Python)
          /*
          if (!fixedT && !fixedH) {
            double relax = 0.5;
            node->ther_gues.update(CoolProp::HmassP_INPUTS,
                                  (1.0 - relax) * node->senth_old + relax * node->senth_gues,
                                  (1.0 - relax) * node->spres_old + relax * node->spres_gues);
            node->update_Qth();
          }
          */
      // }

      if (!fixedT && !fixedH) {
        double relax = 1.0;
        node->stemp_gues = relax * node->ther_gues->T() + (1.0 - relax) * node->stemp_gues;
        node->update_totaltemp();
      }
    }


    for (auto& face : circuit->faces_owned) {
      if (!face->choked) {
        face->update_statevar();
        face->ther_gues->update();
        // if (circuit->flag_tp) face->ther_gues->update_sat();
        face->update_heat_input();
        face->update_fricfact();
        face->update_velocity(); //may not be required for incomp solver. check pending
      }
    }


	for (auto& node : circuit->nodes_owned) {
      int i = circuit->old2new[node->node_ind];

      bool fixedT = node->fixed_var.count("T");
      bool fixedH = node->fixed_var.count("H");

      if (!fixedT && !fixedH) {
        node->update_staticpres();
      }

      if (!trans_sim && node->is_reservoir) {
        node->update_level();
      }
    }


    for (auto& face : circuit->faces_owned) {
      if (!face->choked) {
        face->update_staticpres();
      } else {
        auto orifice = std::dynamic_pointer_cast<Orifice>(face);
        if (orifice && orifice->choked) {
          orifice->update_Gcr();
          apply_choked_orifice_state(orifice);
        }
      }
      // std::cout << "face " << face->faceno << " " << std::setprecision(8) << std::fixed << face->stemp_gues << "\n";
    }


    // MPI_Abort(mpi::intracomm, 0);
    // std::exit(0);
  }


  simulation::time_fluid_energy.stop();
  
}

}
