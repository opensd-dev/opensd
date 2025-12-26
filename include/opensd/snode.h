//! \file snode.h

#ifndef OPENSD_SNODE_H
#define OPENSD_SNODE_H

// #include <optional>
#include <iostream>
// #include "pugixml.hpp"

// #include "opensd/layer.h"
#include "opensd/vector.h"
// #include "CoolProp.h"
#include "solidframe.h"
#include "crossplatform_shared_ptr.h"
// #include "hdf5_interface.h"
// #include <petscksp.h>
#include "opensd/memory.h"

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================
class Layer;
class SFace;

//==============================================================================
//! \class SNode
//==============================================================================

class SNode {
public:
  std::string identifier;
  std::shared_ptr<SFace> eface;
  std::shared_ptr<SFace> wface;
  std::shared_ptr<SFace> nface;
  std::shared_ptr<SFace> sface;
  Layer* layer;
  int node_ind;
  double Ai, Aj, vol, heat_frac;
  double AFF;
  std::string solname;

  double temp_gues;

  double heat_input;
  double heat_input_old;
  double heat_transfer;
  double htc;
  
  // double esource;
  // double hresidue;
  // double volume;
  shared_ptr<SolidFrame> ther_gues;
  shared_ptr<SolidFrame> ther_old;
  
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

  double eqn_ener(double time, double delt, bool trans_sim, double alpha_ener);
  // void update_gues();
  void assign_prop();
  // void update_old();

  // void save_to_hdf5(hid_t group_id) const;
  // void load_from_hdf5(hid_t group_id);


};

//==============================================================================
// Non-member functions
//==============================================================================


} // namespace opensd

#endif // OPENSD_SNODE_H
