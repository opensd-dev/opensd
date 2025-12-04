//! \file snode.h

#ifndef OPENSD_SNODE_H
#define OPENSD_SNODE_H

// #include <optional>
#include <iostream>
// #include "pugixml.hpp"

// #include "opensd/circuit.h"
#include "opensd/vector.h"
// #include "CoolProp.h"
// #include "fluidframe.h"
// #include "AbstractState.h"
// #include "crossplatform_shared_ptr.h"
// #include "hdf5_interface.h"
// #include <petscksp.h>
#include "opensd/memory.h"

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================
// class Circuit;
class SFace;

//==============================================================================
//! \class SNode
//==============================================================================

class SNode {
public:
  std::string identifier;
  // double mresidue;
  // double msource;
  // vector<std::shared_ptr<Face>> ifaces;
  // vector<std::shared_ptr<Face>> ofaces;
  std::shared_ptr<SFace> eface;
  std::shared_ptr<SFace> wface;
  std::shared_ptr<SFace> nface;
  std::shared_ptr<SFace> sface;
  // double mflow_in;
  // double mflow_out;
  // Circuit* circuit;
  // int node_ind;
  double Ai, Aj, vol, heat_frac;
  double AFF;

  // double tpres_old;
  // double ttemp_old;
  // double tenth_old;
  // double tpres_gues;
  // double ttemp_gues;
  // double tenth_gues;

  // double spres_old;
  // double stemp_old;
  // double senth_old;
  // double spres_gues;
  // double stemp_gues;
  // double senth_gues;
  
  double heat_input;
  double heat_input_old;
  double heat_transfer;
  // double elevation;
  
  // std::set<std::string> fixed_var;
  // double esource;
  // double hresidue;
  // double volume;
  // shared_ptr<FluidFrame> ther_gues;
  // shared_ptr<FluidFrame> ther_old;
  // double velocity;
  // double B1;
  
  // double _heat_input_esource;
  // double _heat_input_msource;
  // double _heat_input_faceconv;
  // double _heat_input_faceconv2;
  // double _heat_input_facegen;
  // std::vector<double> heat_hslab;
  // std::vector<PetscScalar> Acols;
  // std::vector<PetscScalar> Avals;
  // PetscScalar brow;

  // explicit Node(pugi::xml_node flnode_node);
  SNode(std::string identifier);
  SNode() = default;
  // virtual ~Node() = default;
  // ~Node() {
  // delete ther_old;
  // delete ther_gues;
  // }

  // double eqn_cont(double time, double delt, bool trans_sim, double alpha_mom);
  double eqn_ener(double time, double delt, bool trans_sim, double alpha_ener);
  // void update_gues();
  // void assign_staticvar();
  // void update_statictemp();
  // void update_totalenth();
  // void update_staticvar(std::optional<double> velocity_in = std::nullopt);
  // void assign_prop();
  // void update_old();
  // void update_staticpres();
  // void update_totaltemp(); 
  // void update_staticenth();

  // void save_to_hdf5(hid_t group_id) const;
  // void load_from_hdf5(hid_t group_id);


};

//==============================================================================
// Non-member functions
//==============================================================================


} // namespace opensd

#endif // OPENSD_SNODE_H
