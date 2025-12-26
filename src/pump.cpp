//! \file pump.cpp
#include "opensd/pump.h"

// #include <iostream>

#include "opensd/error.h"
#include "opensd/xml_interface.h"
// #include "opensd/vector.h"
// #include "opensd/constants.h"

// #include <string>
// #include <cstdlib>  // for std::exit
// #include <dlfcn.h>  // for dynamic loading of libraries (Linux)
// #include <filesystem> // C++17 for current directory

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================

//==============================================================================
// GER implementation
//==============================================================================

Pump::Pump(const std::string& identifier,
           std::shared_ptr<Node> unode, double ufrac,
           std::shared_ptr<Node> dnode, double dfrac,
           int Nop, int flowreg)
  : Face(0, unode, ufrac, dnode, dfrac),
    Nop(Nop),
    flowreg(flowreg)
{
  delz = dnode->elevation - unode->elevation;

  // unode->ofaces.push_back(this);
  // dnode->ifaces.push_back(this);

  // circuit = unode->circuit;
  // circuit->faces.push_back(this);
  //
  // opening = 1.0;
}

void Pump::update_abcoef(double time, double delt,
                         bool trans_sim, double alpha_mom)
{
  double relax = 1.0;

  double drho_dp = ther_gues->drho_dp_consth();

  // aplus =
  //   (-1.0
  //    - spres_gues / tpres_gues
  //      * 0.5 * drho_dp * delz * consts::grav)
  //   / (relax * dHdQfuncs(vflow_gues));
  //
  // aminus =
  //   (-1.0
  //    + spres_gues / tpres_gues
  //      * 0.5 * drho_dp * delz * consts::grav)
  //   / (relax * dHdQfuncs(vflow_gues));

  bplus = spres_gues / tpres_gues * 0.5 * drho_dp;
  bminus = bplus;
}


double Pump::eqn_mom(double x, double time, double delt,
                     bool trans_sim, double alpha_mom)
{
  double z = 0;
  // double z =
  //   QHfuncs(x)
  //   - (dnode->tpres_gues - unode->tpres_gues)
  //   - ther_gues->rhomass() * consts::grav * delz;

  return z;
}

// void Pump::save_to_hdf5(hid_t group_id) const {
//   write_string_attribute(group_id, "identifier", identifier);
//   write_double_attribute(group_id, "diameter", diameter);
//   write_double_attribute(group_id, "length", length);
//   write_double_attribute(group_id, "ufrac", ufrac);
//   write_double_attribute(group_id, "dfrac", dfrac);
// }
// void Pump::load_from_hdf5(hid_t group_id) {
//   // identifier = read_string(group_id, "identifier");
//   length = read_double_attribute(group_id, "length");
//   diameter = read_double_attribute(group_id, "diameter");
// }

} // namespace opensd
