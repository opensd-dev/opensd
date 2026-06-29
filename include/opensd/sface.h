//! \file sface.h

#ifndef OPENSD_SFACE_H
#define OPENSD_SFACE_H

#include <iostream>
// #include "pugixml.hpp"
//
#include "opensd/snode.h"
// #include "opensd/pipe.h"
#include "opensd/sfacether.h"
// #include "opensd/connection.h"
// #include "opensd/vector.h"
#include "opensd/memory.h"

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================
class SFaceTher;
// class Connection;

//==============================================================================
//! \class SFace
//==============================================================================

class SFace {
public:
//   int faceno;
  std::string identifier;
  std::shared_ptr<SNode> unode;
  std::shared_ptr<SNode> dnode;
  double A;
  double delx;
  double delx1;
  double delx2;
//   double vflow_old;
//   double vflow_gues;
//   double mflow;
//   double velocity;
//   double heat_input_old;
//   double heat_input;
//   std::vector<double> heat_hslab;
//   std::vector<double> heat_hslab_old;
//   bool choked;
//   double presidue;
//   double Gcr;
//   double pcr;
//   double tpres_old;
//   double spres_old;
//   double ttemp_old;
//   double stemp_old;
//   double tpres_gues;
//   double spres_gues;
  double temp_gues;
  SFaceTher* ther_gues;
  SFaceTher* ther_old;
//
//   Connection* upstream;
//   Connection* downstream;
//
//   double aplus;
//   double aminus;
//   double bplus;
//   double bminus;
//   int owner;
//
//
  SFace(std::string identifier, std::shared_ptr<SNode> unode, std::shared_ptr<SNode> dnode, double A);
  SFace() = default;
//   virtual ~Face() = default;
//   void assign_statevar();
//   void update_statevar();
//   void update_staticpres();
  void update_temp();
  void update_old();
//   virtual void update_gues();
//   void assign_prop();
//   virtual void update_old();
//
//   virtual void update_velocity() {}
//   virtual void update_fricfact() {}
//   virtual void update_heat_input() {}
//
//   virtual double eqn_mom(double x, double time, double delt, bool trans_sim, double alpha_mom) {return 0;}
//   virtual void update_abcoef(double time, double delt, double trans_sim, double alpha_mom) {}
//
//   virtual void save_to_hdf5(hid_t group_id) const {}
//   virtual void load_from_hdf5(hid_t group_id) {}


};

//==============================================================================
// Non-member functions
//==============================================================================


} // namespace opensd

#endif // OPENSD_SFace_H
