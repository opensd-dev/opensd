//! \file ht_solver.h

#ifndef HT_SOLVER_H
#define HT_SOLVER_H

// #include <omp.h>

#include "opensd/hslab.h"
// #include <Eigen/Dense>
// #include <Eigen/Core>
// #include <unsupported/Eigen/NonLinearOptimization>
// #include <unsupported/Eigen/NumericalDiff>
// #include "CoolProp.h"
#include <utility>
#include <string>

namespace opensd {
namespace solid {
void exec_energy(double time, double delt, bool trans_sim, double alpha_ener, int main_iter);
std::pair<double, double>
exec_bc(const std::string& bvar,
        double bval, //const auto& bval,
		vector<std::shared_ptr<Face>> bval1,
        double A,
        std::shared_ptr<SNode> wall_node,
        int bound_ind);

}
} // namespace opensd

#endif // HT_SOLVER_H
