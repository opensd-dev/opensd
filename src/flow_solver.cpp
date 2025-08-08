//! \file flow_solver.cpp
#include "opensd/flow_solver.h"

#include <algorithm>
#include <cmath>         // For std::isinf and other math functions
#include <iostream>
#include <iomanip>
#include <Eigen/Dense>   // For matrix manipulations
#include <cstdlib>
#include "opensd/vector.h"
#include "opensd/message_passing.h"
#include "opensd/timer.h"

#include "opensd/circuit.h"
// #include <numeric>     // For std::accumulate
// #include <copy>          // For std::copy in Arow and brow
#include <petscksp.h>
#include <fstream>

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================
using MatrixR = Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;

// Define the nonlinear function for the face's momentum equation
struct FaceFunctor {
  using Scalar = double;
  using InputType = Eigen::VectorXd;
  using ValueType = Eigen::VectorXd;
  using JacobianType = Eigen::MatrixXd;
  enum { InputsAtCompileTime = 1, ValuesAtCompileTime = 1 };  
  
  // Constructor to initialize with parameters
  FaceFunctor(double time, double delt, bool trans_sim, double alpha_mom, std::shared_ptr<Face> face)
    : time(time), delt(delt), trans_sim(trans_sim), alpha_mom(alpha_mom), face(face) {}

  // Function to be solved: returns f(x)
  int operator()(const Eigen::VectorXd &x, Eigen::VectorXd &fvec) const {
    // Replace with the actual momentum equation
    double vflow_gues = x(0);
    fvec(0) = face->eqn_mom(vflow_gues,time,delt,trans_sim,alpha_mom);/* momentum equation involving vflow_gues and other parameters */;
    return 0;
  }

/* 
  // Optionally provide the Jacobian
  int df(const Eigen::VectorXd &x, Eigen::MatrixXd &fjac) const {
    // Derivative of the momentum equation with respect to vflow_gues
    fjac(0, 0) = // derivative of momentum equation ;
    return 0;
  }

 */
  int inputs() const { return 1; }
  int values() const { return 1; }

private:
  double time;
  double delt;
  bool trans_sim;
  double alpha_mom;
  std::shared_ptr<Face> face;
};


Eigen::VectorXd insertZerosAtIndices(const Eigen::VectorXd& vec, const std::vector<int>& indices) {
    // Create a new vector with the required size
    Eigen::VectorXd new_vec(vec.size() + indices.size());

    // Iterate over the original vector and the indices
    int orig_index = 0;  // Index for original vector vec
    int new_index = 0;   // Index for new vector new_vec
    int indices_index = 0; // Index for indices

    for (int i = 0; i < new_vec.size(); ++i) {
        if (indices_index < indices.size() && new_index == indices[indices_index]) {
            // Insert 0.0 at the specified index
            new_vec[i] = 0.0;
            indices_index++;
        } else {
            if (orig_index < vec.size()) {
                new_vec[i] = vec[orig_index++]; // Copy element from the original vector
            }
        }
        new_index++;
    }

    return new_vec;
}

//==============================================================================
// Non-member functions
//==============================================================================

void guess_flow(double time, double delt, bool trans_sim, double alpha_mom, int main_iter, std::shared_ptr<Circuit> circuit) {

  // for (auto& branch : circuit->branches) { // Guess flow rate calculation
  for (auto& face : circuit->faces) {
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
      // } else {
  
        Eigen::VectorXd x(1);
        x(0) = face->vflow_gues;
  
        FaceFunctor functor(time, delt, trans_sim, alpha_mom, face);
        Eigen::NumericalDiff<FaceFunctor> numDiff(functor);
        Eigen::LevenbergMarquardt<Eigen::NumericalDiff<FaceFunctor>> lm(numDiff);
  
        int info = lm.minimize(x);
        if (info <= 0) {
          std::cerr << "LM failed to converge: info = " << info << std::endl;
        }
        
        face->vflow_gues = x(0);
//        auto pface = std::static_pointer_cast<PFace>(face);
//        std::cout << std::defaultfloat << std::setprecision(10) << "pdnode = "   << pface->dnode->tpres_gues << std::endl;
  
        // if (face.opening == 0.0) continue;
        // if (branch.isolated && !trans_sim) {
          // face.vflow_gues = 0.0;
          // continue;
        // }
        if (std::abs(face->vflow_gues) < 1.E-8 && main_iter == 0) { // Tune the value 1.E-8 as needed
          face->vflow_gues = 1.E-8 * std::copysign(1.0, face->vflow_gues);
          if (face->vflow_gues == 0.0) {
            face->vflow_gues = 1.E-8;
          }
        }
        // std::cout << face.vflow_gues << std::endl;
        // if (dynamic_cast<PFace*>(&face) != nullptr || dynamic_cast<or_comp::Orifice*>(&face) != nullptr) {
          // face.G = face.vflow_gues * face.ther_gues.rhomass() / (face.cfarea * face.opening);
        // }
      // }
      face->update_abcoef(time, delt, trans_sim, alpha_mom);
      // std::cout << face->vflow_gues << std::endl;
  }
}
  
void exec_massmom(double time, double delt, bool trans_sim, double alpha_mom, int main_iter, int flow_iter) {
  
    simulation::time_massmom.start();
  
   for (auto& circuit : model::circuits) {
    // if (!trans_sim && !circuit->solveSS) continue;
    // std::cout << circuit->identifier << std::endl;
    guess_flow(time, delt, trans_sim, alpha_mom, main_iter, circuit);

    // Pressure corrections
    Mat A;
    Vec b, pc;
    KSP ksp;
    std::vector<double> msource_global(circuit->nodes.size());
    PetscInt n = circuit->nodes.size(), i;

    int start = mpi::rank * (n / mpi::n_procs);
    int end = (mpi::rank == mpi::n_procs - 1) ? n : start + (n / mpi::n_procs);

    std::vector<double> msource_local(end - start);
    MatCreate(mpi::intracomm, &A);
    MatSetSizes(A, PETSC_DECIDE, PETSC_DECIDE, n, n);
    MatSetFromOptions(A);
    MatSetUp(A);
    
    VecCreate(mpi::intracomm, &b);
    VecSetSizes(b, PETSC_DECIDE, n);
    VecSetFromOptions(b);
    
    VecCreate(mpi::intracomm, &pc);
    VecSetSizes(pc, PETSC_DECIDE, n);
    VecSetFromOptions(pc);
    
    // for (int i = start; i < end; ++i) {
      // auto& node = circuit->nodes[i];

    for (auto& node : circuit->nodes_owned) {
      int i = node->node_ind;  // global row index
      // int i_local = i - start;
	  bool pbound = node->fixed_var.count("P");
      double A_local_node = 0, A_local_iface, A_local_oface;
      double b_local;

      double B, D;
      if (node->ther_old->phase() == 6) {
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
          MatSetValue(A, i, iface->unode->node_ind, A_local_iface, INSERT_VALUES);
          if (A_local_iface > 0.0) {
            // if ((show_warn && trans_sim) || !trans_sim) {
              std::cout << "Warning: upstream coef negative. " << node->identifier << std::endl;
            // }
          }
          A_local_node = A_local_node - alpha_mom * (-iface->aplus * iface->ther_gues->rhomass() + iface->bplus * iface->vflow_gues);
        }
        b_local += alpha_mom * (iface->ther_gues->rhomass() * iface->vflow_gues) + (1.0 - alpha_mom) * (iface->ther_old->rhomass() * iface->vflow_old);
      }

      for (auto& oface : node->ofaces) {
    	if (!pbound) {
          A_local_oface = -alpha_mom * (oface->aplus * oface->ther_gues->rhomass() - oface->bplus * oface->vflow_gues);
          MatSetValue(A, i, oface->dnode->node_ind, A_local_oface, INSERT_VALUES);
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
        // node->msource = -b(i);
        // std::cout << "node" << node->identifier << " " << i_local << " " << b_local(i_local) << " " << mpi::rank << std::endl;
        node->msource = -b_local;
        // msource_local[i_local] = node->msource;

        A_local_node = 1.0;

		b_local = 0.0;
        
      } else if (node->fixed_var.count("msource")) {
        if (time <= 20) {
            node->msource = -753.6*(20.-time)/20.;
		}
        else {
            node->msource = 0.;
		}
        // msource_local[i_local] = node->msource;
        b_local += node->msource;
      }

      if (A_local_node < -1.E-6) { // Pending check if 0
        // if ((show_warn && trans_sim) || !trans_sim) {
          std::cout << "Warning: negative A coef. " << node->identifier << " " << A_local_node << std::endl;
        // }
      }

      MatSetValue(A, i, i, A_local_node, INSERT_VALUES);
      VecSetValue(b, i, b_local, INSERT_VALUES);

	  
      std::cout << "rank " << mpi::rank << " node " << node->identifier << " index " << i << 
        " A_local_node " << A_local_node << " b_local " << b_local << " A_local_iface " << 
        A_local_iface << " A_local_oface " << A_local_oface << " Pbound " << node->fixed_var.count("P") << std::endl;
	  

    }


MatAssemblyBegin(A, MAT_FINAL_ASSEMBLY);
MatAssemblyEnd(A, MAT_FINAL_ASSEMBLY);

VecAssemblyBegin(b);
VecAssemblyEnd(b);



// Print matrix A
PetscViewer viewerA;
PetscViewerASCIIOpen(PETSC_COMM_WORLD, "matrix_A.txt", &viewerA);
PetscViewerPushFormat(viewerA, PETSC_VIEWER_ASCII_DENSE); // optional: DENSE format
MatView(A, viewerA);
PetscViewerPopFormat(viewerA);
PetscViewerDestroy(&viewerA);

// Print vector b
PetscViewer viewerB;
PetscViewerASCIIOpen(PETSC_COMM_WORLD, "vector_b.txt", &viewerB);
VecView(b, viewerB);
PetscViewerDestroy(&viewerB);




KSPCreate(mpi::intracomm, &ksp);
KSPSetOperators(ksp, A, A);
KSPSetFromOptions(ksp);
KSPSolve(ksp, b, pc);

PetscViewer viewer;
PetscViewerASCIIOpen(PETSC_COMM_WORLD, "pc_output.txt", &viewer);
VecView(pc, viewer);
PetscViewerDestroy(&viewer);

MPI_Abort(mpi::intracomm, 0);
std::exit(0);

int rows_per_rank = n / mpi::n_procs;

std::vector<int> recvcounts_m(mpi::n_procs), displs_m(mpi::n_procs);

for (int r = 0; r < mpi::n_procs; ++r) {
  int r_rows = (r == mpi::n_procs - 1) ? n - r * rows_per_rank : rows_per_rank;
  recvcounts_m[r] = r_rows;
  displs_m[r] = (r == 0) ? 0 : displs_m[r - 1] + recvcounts_m[r - 1];
}

MPI_Gatherv(msource_local.data(), msource_local.size(), MPI_DOUBLE,
            (mpi::rank == 0 ? msource_global.data() : nullptr), recvcounts_m.data(), displs_m.data(), MPI_DOUBLE,
            0, mpi::intracomm);



// Broadcast pc to all ranks
// MPI_Bcast(pc.data(), pc.size() , MPI_DOUBLE, 0, mpi::intracomm);

MPI_Bcast(msource_global.data(), msource_global.size(), MPI_DOUBLE, 0, mpi::intracomm);

//    std::cout << "pc = \n" << pc << std::endl;
    
//    if (flow_iter == 1) {
//      std::exit(0);
//    }



// Get global size
VecGetSize(pc, &n);

// Create sequential vector to hold full solution on all ranks
Vec pc_full;
VecCreateSeq(PETSC_COMM_SELF, n, &pc_full);

// Create identity index sets
IS from, to;
ISCreateStride(PETSC_COMM_WORLD, n, 0, 1, &from);
ISCreateStride(PETSC_COMM_SELF,  n, 0, 1, &to);

// Create and execute scatter
VecScatter scatter;
VecScatterCreate(pc, from, pc_full, to, &scatter);
VecScatterBegin(scatter, pc, pc_full, INSERT_VALUES, SCATTER_FORWARD);
VecScatterEnd(scatter, pc, pc_full, INSERT_VALUES, SCATTER_FORWARD);

// Access full pc values on all ranks
PetscScalar* pc_array;
VecGetArray(pc_full, &pc_array);

// for (PetscInt i = 0; i < n; ++i) {
    // std::cout << "Rank " << mpi::rank << " full pc[" << i << "] = " << pc_array[i] << std::endl;
// }



     // Flow rate corrections
    for (auto& face : circuit->faces) {
      if (!face->choked) {

        double vc = face->aminus * pc_array[face->unode->node_ind] - face->aplus * pc_array[face->dnode->node_ind];
        face->vflow_gues += vc;
      }
      face->update_velocity();
    }


    // Pressure and density corrections
    for (int i = 0; i < n; ++i) {
      auto& node = circuit->nodes[i];
      // if (node.flowreg == "Slug") continue;
      node->msource = msource_global[i];
      double relax = 0.6;
      // if (solver::relax_pres) {
        // relax = solver::relax_pres;
      // }
      node->tpres_gues += relax * pc_array[i];
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


// std::ofstream fout("tpres_rank" + std::to_string(mpi::rank) + ".txt");
// for (auto& node : circuit->nodes) {
  // fout << node->identifier << " " << node->tpres_gues << " " << node->tpres_old << "\n";
// }
// fout.close();

VecRestoreArray(pc_full, &pc_array);

// Clean up
VecScatterDestroy(&scatter);
ISDestroy(&from);
ISDestroy(&to);
VecDestroy(&pc_full);

KSPDestroy(&ksp);
MatDestroy(&A);
VecDestroy(&b);
VecDestroy(&pc);

    for (auto& face : circuit->faces) {
      // if (!face->choked) {
        face->update_statevar();
        face->ther_gues->update();
        // if (circuit.flag_tp) face->ther_gues->update_sat();
        // face->update_heat_input(time, delt);
        face->update_fricfact();
      // } else {
        // face->update_Gcr();
        // face->G = std::copysign(face->Gcr, face->vflow_gues);
        // face->vflow_gues = face->G * face->cfarea / face->ther_gues.rhomass();
      // }
    }

}
simulation::time_massmom.stop();
}

void exec_energy(double time, double delt, bool trans_sim, double alpha_ener, int main_iter) {
  
  for (auto& circuit : model::circuits) {
    // if (!trans_sim && !circuit.solveSS) continue;

    int n = circuit->nodes.size();
    Eigen::MatrixXd A = Eigen::MatrixXd::Zero(n, n);
    Eigen::VectorXd b = Eigen::VectorXd::Zero(n);
    vector<int> nocal_ind;


    for (int i = 0; i < n; ++i) {
      auto& node = circuit->nodes[i];

      double C = node->volume * node->ther_old->rhomass();
      double E = node->volume;
      A(i, i) = trans_sim * C / delt;
      b(i) = node->tenth_old * (trans_sim * C / delt) 
             + trans_sim * E * (node->spres_gues - node->spres_old) / delt
             + node->heat_input; // + std::accumulate(node.heat_hslab.begin(), node.heat_hslab.end(), 0.0);
      b(i) = b(i) - node->tenth_old * node->msource * trans_sim;

      for (auto& iface : node->ifaces) {
        // if (dynamic_cast<cont::Reservoir*>(iface.dnode) && iface.dfrac != nullptr 
            // && iface.dnode->ther_gues.phase() == 6) {
          // b(i) -= alpha_ener * iface.downstream->tenth_gues * std::max(-iface.ther_gues.rhomass() * iface.vflow_gues, 0.0);
        // } else {
          A(i, i) += alpha_ener * std::max(-iface->ther_gues->rhomass() * iface->vflow_gues, 0.0);
        // }

        // if (dynamic_cast<cont::Reservoir*>(iface.unode) && iface.ufrac != nullptr 
            // && iface.unode->ther_gues.phase() == 6) {
          // b(i) += alpha_ener * iface.upstream->tenth_gues * std::max(iface.ther_gues.rhomass() * iface.vflow_gues, 0.0);
        // } else {
          A(i, iface->unode->node_ind) = -alpha_ener * std::max(iface->ther_gues->rhomass() * iface->vflow_gues, 0.0);
        // }

        b[i] = (b[i] 
                  - iface->downstream->tenth_old * (1.0 - alpha_ener) 
                    * std::max(-iface->ther_old->rhomass() * iface->vflow_old, 0.0)
                  + iface->upstream->tenth_old * (1.0 - alpha_ener) 
                    * std::max(iface->ther_old->rhomass() * iface->vflow_old, 0.0));
        
        b[i] = (b[i] 
                  + alpha_ener * (iface->heat_input) // + sum(iface->heat_hslab)) 
                    * std::max(static_cast<double>(!std::signbit(iface->vflow_gues)), 0.0)
                  + (1.0 - alpha_ener) * (iface->heat_input_old ) //+ sum(iface->heat_hslab_old)) 
                    * std::max(static_cast<double>(!std::signbit(iface->vflow_old)), 0.0));
        
        b[i] = (b[i] 
                  - node->tenth_old * alpha_ener * iface->ther_gues->rhomass() 
                    * iface->vflow_gues * trans_sim
                  - node->tenth_old * (1.0 - alpha_ener) 
                    * iface->ther_old->rhomass() * iface->vflow_old * trans_sim);
        
      }

      for (auto& oface : node->ofaces) {
        // if (dynamic_cast<cont::Reservoir*>(oface.unode) && oface.ufrac != nullptr 
            // && oface.unode->ther_gues.phase() == 6) {
          // b(i) -= alpha_ener * oface.upstream->tenth_gues * std::max(oface.ther_gues.rhomass() * oface.vflow_gues, 0.0);
        // } else {
          A(i, i) += alpha_ener * std::max(oface->ther_gues->rhomass() * oface->vflow_gues, 0.0);
        // }

        // if (dynamic_cast<cont::Reservoir*>(oface.dnode) && oface.dfrac != nullptr 
            // && oface.dnode->ther_gues.phase() == 6) {
          // b(i) += alpha_ener * oface.downstream->tenth_gues * std::max(-oface.ther_gues.rhomass() * oface.vflow_gues, 0.0);
        // } else {
          A(i, oface->dnode->node_ind) = -alpha_ener * std::max(-oface->ther_gues->rhomass() * oface->vflow_gues, 0.0);
        // }

        
        b[i] = (b[i] 
                  - oface->upstream->tenth_old * (1.0 - alpha_ener) 
                    * std::max(oface->ther_old->rhomass() * oface->vflow_old, 0.0)
                  + oface->downstream->tenth_old * (1.0 - alpha_ener) 
                    * std::max(-oface->ther_old->rhomass() * oface->vflow_old, 0.0));
        
        b[i] = (b[i] 
                  + alpha_ener * (oface->heat_input) // + std::accumulate(oface->heat_hslab.begin(), oface->heat_hslab.end(), 0.0)) 
                    * std::max(static_cast<double>(!std::signbit(-oface->vflow_gues)), 0.0)
                  + (1.0 - alpha_ener) * (oface->heat_input_old) // + std::accumulate(oface->heat_hslab_old.begin(), oface->heat_hslab_old.end(), 0.0)) 
                    * std::max(static_cast<double>(!std::signbit(-oface->vflow_old)), 0.0));
        
        b[i] = (b[i] 
                  + node->tenth_old * alpha_ener * oface->ther_gues->rhomass() 
                    * oface->vflow_gues * trans_sim
                  + node->tenth_old * (1.0 - alpha_ener) 
                    * oface->ther_old->rhomass() * oface->vflow_old * trans_sim);
        
      }
    }

    
    // using Eigen::MatrixXd;
    // using Eigen::VectorXd;
    
    for (size_t i = 0; i < circuit->nodes.size(); ++i) {
      auto node = circuit->nodes[i];
    
       if (node->fixed_var.find("T") != node->fixed_var.end()) {
        // node->update_statictemp();
        // node->ther_gues->update(CoolProp::PT_INPUTS, node->spres_gues, node->stemp_gues);
        // node->senth_gues = node->ther_gues->hmass();
        // node->ther_gues->update(CoolProp::HmassP_INPUTS, node->senth_gues, node->spres_gues);
        // if (circuit->flag_tp || dynamic_cast<TPTank*>(node)) node->ther_gues->update_sat();
        // node->update_totalenth();
        // node->Arow = A.row(i);
        // node->brow = b(i);
        // b(i) = node->tenth_gues;
        // A.row(i).setZero();
        // A(i, i) = 1.0;
      } 
      else if (node->fixed_var.find("H") != node->fixed_var.end()) {
        // node->update_staticenth();
        // node->ther_gues->update(CoolProp::HmassP_INPUTS, node->senth_gues, node->spres_gues);
        // node->stemp_gues = node->ther_gues->T();
        // if (circuit->flag_tp || dynamic_cast<TPTank*>(node)) node->ther_gues->update_sat();
        // node->update_totaltemp();
        // node->update_staticpres();
        // node->Arow = A.row(i);
        // node->brow = b(i);
        // b(i) = node->tenth_gues;
        // A.row(i).setZero();
        // A(i, i) = 1.0;
      } 
      else if (node->fixed_var.find("msource") != node->fixed_var.end() || node->fixed_var.find("P") != node->fixed_var.end()) {
        // if (node->msource > 0.0) {
          // if (node->tenth_msrc.has_value()) {
            // b(i) += node->tenth_msrc.value() * node->msource;
          // } else {
            // if (node->msource > 1.E-6) {
              // std::cout << "warning: positive mass source condition assumed based on previous circuit condition "
                        // << node->identifier << " tenth=" << node->tenth_gues << " " << node->msource << std::endl;
            // }
            // b(i) += node->tenth_old * node->msource;
          // }
        // } else {
          // A(i, i) -= node->msource;
          // if (A(i, i) < 0.0) {
            // std::cerr << "negative coef. in energy solver. stopping" << std::endl;
            // exit(EXIT_FAILURE);
          // }
        // }
      }
    }
    
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
    // double cond = A.fullPivLu().rcond();
    // if (cond < 1.E8) {
      // VectorXd enth = A.colPivHouseholderQr().solve(b);
    // } else if (std::isinf(cond)) {
      // std::cerr << "infinite condition number. check boundary conditions" << std::endl;
      // exit(EXIT_FAILURE);
    // } else {
      // VectorXd enth_old = VectorXd::Zero(circuit->nodes.size());
      // for (size_t i = 0; i < circuit->nodes.size(); ++i) {
        // enth_old[i] = circuit->nodes[i]->tenth_gues;
      // }
      // removeElements(enth_old, nocal_ind);
      // VectorXd enth = sor_solver(A, b, 0.8, enth_old, 1.E-8, 25);  // sor_solver is assumed to be defined
    // }

/*
    // Remaining logic to handle matrix operations, boundary conditions, nocal_ind, energy update, etc.

    // Solving the system and post-processing the results here...
    if (A.determinant() != 0 && A.fullPivLu().isInvertible()) {
      Eigen::VectorXd enth = A.fullPivLu().solve(b);
      // Further updates and checks for enth values...
    } else {
      std::cerr << "Matrix is singular or ill-conditioned, please check boundary conditions.\n";
      std::exit(EXIT_FAILURE);
    }
*/
    }
}

}
