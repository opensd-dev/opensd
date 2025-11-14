//! \file flow_solver.h

#ifndef FLOW_SOLVER_H
#define FLOW_SOLVER_H

#include <omp.h>

#include "opensd/circuit.h"
// #include <Eigen/Dense>
// #include <Eigen/Core>
// #include <unsupported/Eigen/NonLinearOptimization>
// #include <unsupported/Eigen/NumericalDiff>
// #include "CoolProp.h"

namespace opensd {

void exec_massmom(double time, double delt, bool trans_sim, double alpha_mom, int main_iter, int flow_iter);
void exec_energy(double time, double delt, bool trans_sim, double alpha_ener, int main_iter);
void guess_flow(double time, double delt, bool trans_sim, double alpha_mom, int main_iter, std::shared_ptr<Circuit> circuit);

} // namespace opensd

#endif // FLOW_SOLVER_H
