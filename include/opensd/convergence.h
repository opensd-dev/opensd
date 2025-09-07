//! \file convergence.h

#ifndef CONVERGENCE_H
#define CONVERGENCE_H

#include <tuple>
#include <string>

namespace opensd {
    
std::tuple<bool, std::tuple<double, double>> check_conv(double time, double delt, bool trans_sim, double alpha_mom, std::string opt = "all");
void update_old();

}

#endif // CONVERGENCE_H
