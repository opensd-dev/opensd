//! \file layer.h

#ifndef OPENSD_LAYER_H
#define OPENSD_LAYER_H

// #include <optional>
#include <memory>
#include "pugixml.hpp"

// #include "opensd/hslab.h"
#include "opensd/snode.h"
#include "opensd/vector.h"
// #include "hdf5_interface.h"
// #include <petscksp.h>

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================
class HSlab;

//==============================================================================
//! \class Layer
//==============================================================================

class Layer {
public:
  int layerno;
  std::string solname;
  std::string sollib;
  int nnodes;
  double thk_elem;
  double darea;

  // double mresidue;
  // vector<std::shared_ptr<Face>> ifaces;
  // vector<std::shared_ptr<Face>> ofaces;
  vector<std::shared_ptr<SNode>> nodes;
  HSlab* hslab;
  // int node_ind;
  
  // double volume;
  // shared_ptr<FluidFrame> ther_gues;
  // shared_ptr<FluidFrame> ther_old;

  explicit Layer(pugi::xml_node layer_node);
  // Layer(std::string identifier, double volume, double heat_input, double elevation, double tpres_old = 0.0, double ttemp_old = 0.0, double tenth_old = 0.0); //Circuit* circuit,
  Layer() = default;
  // virtual ~Layer() = default;
  // ~Layer() {
  // delete ther_old;
  // delete ther_gues;
  // }

  // double eqn_ener(double time, double delt, bool trans_sim, double alpha_ener);
  // void update_gues();

  // void save_to_hdf5(hid_t group_id) const;
  // void load_from_hdf5(hid_t group_id);


};

//==============================================================================
// Non-member functions
//==============================================================================


} // namespace opensd

#endif // OPENSD_LAYER_H
