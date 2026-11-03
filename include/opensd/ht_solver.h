//! \file ht_solver.h

#ifndef HT_SOLVER_H
#define HT_SOLVER_H

// #include <omp.h>

#include "opensd/hslab.h"
#include "opensd/custom_fun.h"
// #include <Eigen/Dense>
// #include <Eigen/Core>
// #include <unsupported/Eigen/NonLinearOptimization>
// #include <unsupported/Eigen/NumericalDiff>
// #include "CoolProp.h"
#include <utility>
#include <string>

namespace opensd {
namespace solid {
void sync_fluid_state();
void sync_solid_state();
void sync_boundary_temperatures();
void exec_energy(double time, double delt, bool trans_sim, double alpha_ener,
                 int main_iter, bool synchronize=true);
std::pair<double, double>
exec_bc(const std::string& bvar,
        const Input& bval,
		const vector<std::shared_ptr<Face>>& bval1,
        double A,
        const std::shared_ptr<SNode>& wall_node,
        int bound_ind,
        const double* pipe_htc=nullptr);

double exec_ht(const std::string& bvar,
               const Input& bval,
               const vector<std::shared_ptr<Face>>& bval1,
               double A,
               const std::shared_ptr<SNode>& wall_node,
               int bound_ind);
}
} // namespace opensd

#endif // HT_SOLVER_H
