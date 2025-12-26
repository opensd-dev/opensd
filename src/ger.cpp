//! \file ger.cpp
#include "opensd/ger.h"

// #include <iostream>

#include "opensd/error.h"
#include "opensd/xml_interface.h"
// #include "opensd/vector.h"
#include "opensd/constants.h"

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

GER::GER(pugi::xml_node ger_node)
{
  if (check_for_node(ger_node, "identifier")) {
    this->identifier = get_node_value(ger_node, "identifier");
  } else {
    fatal_error("Must specify identifier of ger in geometry XML file.");
  }

  this->Ck        = stod(get_node_value(ger_node, "Ck"));
  this->m         = stod(get_node_value(ger_node, "m"));
  this->n         = stod(get_node_value(ger_node, "n"));
  this->dnode_str = get_node_value(ger_node, "dnode");
  this->unode_str = get_node_value(ger_node, "unode");
  // this->heat_input= stod(get_node_value(pipe_node, "heat_input"));
  this->unode = nullptr;
  this->dnode = nullptr;
  // double ufrac;
  // double dfrac;
  // double delz;

}

double GER::eqn_mom(double x, double time, double delt, bool trans_sim, double alpha_mom) {

  double y;

  if (!choked) {

    delp_fr = Ck
              * std::pow(ther_gues->rhomass(), m)
              * x * std::abs(x) * std::pow(std::abs(x), n - 2.0);

    delp_gr = ther_gues->rhomass() * grav * delz;

    double Term_old =
      (1.0 - alpha_mom) *
      (
        (downstream->tpres_old - upstream->tpres_old)
        + ther_old->rhomass() * grav * delz
        + Ck
          * std::pow(ther_old->rhomass(), m)
          * vflow_old * std::abs(vflow_old)
          * std::pow(std::abs(vflow_old), n - 2.0)
      );

    y =
      alpha_mom *
      (
        (downstream->tpres_gues - upstream->tpres_gues)
        + delp_gr
        + delp_fr
      )
      + Term_old;
  }
  else {
  //
  //   double G = branch.faces.back()->G;
  //   double vflow_gues = G * cfarea / ther_gues->rhomass();
  //   y = x - vflow_gues;
  }

  return y;
}

void GER::update_abcoef(double time, double delt, double trans_sim, double alpha_mom)
{
  // branch->isolated = false;
  //
  if (choked) {

  //   aplus  = 0.0;
  //   bplus  = 0.0;
  //
  //   double delta = 0.1;
  //
  //   double y1 = Gcr / rhocr;
  //
  //   auto* flstate = circuit->flstate;
  //   flstate->update(CoolProp::HmassP_INPUTS,
  //                    upstream->tenth_gues,
  //                    upstream->tpres_gues + delta);
  //
  //   double Gcr2, pcr2, rhocr2;
  //   std::tie(Gcr2, pcr2, rhocr2) = GcrHEM(flstate);
  //
  //   double y2 = Gcr2 / rhocr2;
  //
  //   aminus = (y2 - y1) / delta * 0.1;
  //   bminus = (rhocr2 - rhocr) / delta * 0.1;
  }
  else {

    double dr =
      n * Ck
      * std::pow(ther_gues->rhomass(), m)
      * std::pow(std::abs(vflow_gues), n - 1.0);

    aplus =
      (
        1.0
        + (spres_gues / tpres_gues
           * 0.5 * ther_gues->drho_dp_consth()
           * (
               grav * delz
               + m * Ck
                 * std::pow(ther_gues->rhomass(), m - 1.0)
                 * vflow_gues
                 * std::pow(std::abs(vflow_gues), n - 1.0)
             )
          )
      ) / dr;

    aminus =
      (
        1.0
        - (spres_gues / tpres_gues
           * 0.5 * ther_gues->drho_dp_consth()
           * (
               grav * delz
               + m * Ck
                 * std::pow(ther_gues->rhomass(), m - 1.0)
                 * vflow_gues
                 * std::pow(std::abs(vflow_gues), n - 1.0)
             )
          )
      ) / dr;

    if (aplus < 0.0 || aminus < 0.0) {
      // if (solver.show_warn) {
        std::cout << "warning. acoef negative "
                  << identifier << " "
                  << aplus << " "
                  << aminus << " "
                  << unode->ther_gues->rhomass() << " "
                  << dnode->ther_gues->rhomass() << " "
                  << dr << std::endl;
      // }
    }

    bplus = bminus =
      spres_gues / tpres_gues
      * 0.5 * ther_gues->drho_dp_consth();

    if (bplus < 0.0) {
      std::cout << spres_gues << " "
                << tpres_gues << " "
                << ther_gues->drho_dp_consth() << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }
}

// void GER::save_to_hdf5(hid_t group_id) const {
//   write_string_attribute(group_id, "identifier", identifier);
//   write_double_attribute(group_id, "diameter", diameter);
//   write_double_attribute(group_id, "length", length);
//   write_double_attribute(group_id, "ufrac", ufrac);
//   write_double_attribute(group_id, "dfrac", dfrac);
// }
// void GER::load_from_hdf5(hid_t group_id) {
//   // identifier = read_string(group_id, "identifier");
//   length = read_double_attribute(group_id, "length");
//   diameter = read_double_attribute(group_id, "diameter");
// }

} // namespace opensd
