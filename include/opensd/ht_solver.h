//! \file ht_solver.h

#ifndef HT_SOLVER_H
#define HT_SOLVER_H

// #include <omp.h>

// #include "opensd/hslab.h"
// #include <Eigen/Dense>
// #include <Eigen/Core>
// #include <unsupported/Eigen/NonLinearOptimization>
// #include <unsupported/Eigen/NumericalDiff>
// #include "CoolProp.h"

namespace opensd {
namespace solid {
void exec_energy(double time, double delt, bool trans_sim, double alpha_ener, int main_iter);
}
} // namespace opensd

#endif // HT_SOLVER_H
