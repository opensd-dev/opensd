//! \file convergence.h

#ifndef CONVERGENCE_H
#define CONVERGENCE_H

#include <tuple>
#include <string>

namespace opensd {
    
std::tuple<bool, double, double, double, double> check_conv(double time, double delt, bool trans_sim, double alpha_mom, double alpha_ener = 0., std::string opt = "all", double alpha_heat = 0.);
void update_old();

}

#endif // CONVERGENCE_H
