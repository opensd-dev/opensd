//! \file pump.cpp
#include "opensd/pump.h"

#include <iostream>
#include <algorithm>

#include "opensd/error.h"
#include "opensd/xml_interface.h"
// #include "opensd/vector.h"
#include "opensd/constants.h"

// #include <string>
#include <cstdlib>  // for std::exit
// #include <dlfcn.h>  // for dynamic loading of libraries (Linux)
// #include <filesystem> // C++17 for current directory
#include <fstream>
#include <sstream>
#include <cassert>
namespace opensd {

//==============================================================================
// Global variables
//==============================================================================

//==============================================================================
// Pump implementation
//==============================================================================

Pump::Pump(const std::string& identifier,
           double ufrac,
           double dfrac,
           int Nop)
  : Face(0, nullptr, ufrac, nullptr, dfrac),
    Nop(Nop)
    // flowreg(flowreg)
{
  // delz = dnode->elevation - unode->elevation;

  // unode->ofaces.push_back(this);
  // dnode->ifaces.push_back(this);

  // circuit = unode->circuit;
  // circuit->faces.push_back(this);
  //
  // opening = 1.0;
}

static void read_csv(const std::string& file,
                     std::vector<double>& Q,
                     std::vector<double>& H)
{
  std::ifstream fin(file);
  std::string line;

  while (std::getline(fin, line)) {
    std::stringstream ss(line);
    double q, h;
    char comma;
    ss >> q >> comma >> h;
    Q.push_back(q);
    H.push_back(h);
  }
}

static std::vector<double>
gradient(const std::vector<double>& y,
         const std::vector<double>& x)
{
  std::vector<double> g(y.size());
  for (size_t i = 1; i < y.size()-1; ++i)
    g[i] = (y[i+1] - y[i-1]) / (x[i+1] - x[i-1]);

  g.front() = g[1];
  g.back()  = g[g.size()-2];
  return g;
}

//==============================================================================
// VSPump implementation
//==============================================================================

VSPump::VSPump(pugi::xml_node vsp_node)
: Pump(
      "pump1",//get_node_value(vsp_node, "identifier")
      0.0,
      0.0,
      0 //std::stod(get_node_value(vsp_node, "Nop"))
    )
{

  // if (check_for_node(vsp_node, "identifier")) {
  //   this->identifier = get_node_value(vsp_node, "identifier");
  // } else {
  //   fatal_error("Must specify identifier of vsp in geometry XML file.");
  // }

  // this->Nop         = stod(get_node_value(vsp_node, "Nop"));
  this->curve_file  = get_node_value(vsp_node, "curve_file");
  this->curve_speed = stod(get_node_value(vsp_node, "curve_speed"));
  this->dnode_str = get_node_value(vsp_node, "dnode");
  this->unode_str = get_node_value(vsp_node, "unode");
  // this->heat_input= stod(get_node_value(pipe_node, "heat_input"));
  // this->unode = nullptr;
  // this->dnode = nullptr;
  // double ufrac;
  // double dfrac;
  // double delz;

  // for (const auto& c : curves) {
    speeds.push_back(curve_speed);

    std::vector<double> Q, H;
    read_csv(curve_file, Q, H);

    auto dHdQ = gradient(H, Q);

    QH_funcs.emplace_back(Q, H);
    dHdQ_funcs.emplace_back(Q, dHdQ);
  // }
}

double VSPump::interp_speed(const std::vector<Interp1D>& funcs,
                            double Q) const
{
  const size_t n = speeds.size();

  if (n == 0) {
    std::cerr << "VSPump: no speed curves loaded\n";
    std::abort();
  }

  if (n == 1)
    return funcs[0](Q);

  size_t ul = 0;
  while (ul < n && speeds[ul] < Nop)
    ++ul;

  if (ul == 0)
    return funcs[0](Q);

  if (ul >= n)
    return funcs[n - 1](Q);

  const double N1 = speeds[ul - 1];
  const double N2 = speeds[ul];

  const double y1 = funcs[ul - 1](Q);
  const double y2 = funcs[ul](Q);

  return y1 + (Nop - N1) * (y2 - y1) / (N2 - N1);
}


double VSPump::QHfuncs(double Q) const
{
  return interp_speed(QH_funcs, Q);
}

double VSPump::dHdQfuncs(double Q) const
{
  return interp_speed(dHdQ_funcs, Q);
}

void VSPump::update_abcoef(double time, double delt,
                         bool trans_sim, double alpha_mom)
{
  double relax = 1.0;

  double drho_dp = ther_gues->drho_dp_consth();

  aplus =
    (-1.0
     - spres_gues / tpres_gues
       * 0.5 * drho_dp * delz * grav)
    / (relax * dHdQfuncs(vflow_gues));

  aminus =
    (-1.0
     + spres_gues / tpres_gues
       * 0.5 * drho_dp * delz * grav)
    / (relax * dHdQfuncs(vflow_gues));

  bplus = spres_gues / tpres_gues * 0.5 * drho_dp;
  bminus = bplus;
}


double VSPump::eqn_mom(double x, double time, double delt,
                     bool trans_sim, double alpha_mom)
{

  double z =
    QHfuncs(x)
    - (dnode->tpres_gues - unode->tpres_gues)
    - ther_gues->rhomass() * grav * delz;
  // std::cout<<"flag1 "<<x*ther_gues->rhomass()<<" "<<z<<std::endl;
  // std::exit(0);

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

//==============================================================================
// Non-member functions
//==============================================================================


} // namespace opensd
