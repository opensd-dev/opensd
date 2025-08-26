//! \file node.h

#ifndef OPENSD_NODE_H
#define OPENSD_NODE_H

#include <optional>
#include "pugixml.hpp"

// #include "opensd/circuit.h"
#include "opensd/vector.h"
#include "CoolProp.h"
#include "AbstractState.h"
#include "crossplatform_shared_ptr.h"
#include "hdf5_interface.h"

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================

class Face;

//==============================================================================
//! \class Node
//==============================================================================

class Node {
public:
  std::string identifier; //!< User-defined identifier
  double mresidue;
  double msource;
  vector<std::shared_ptr<Face>> ifaces;
  vector<std::shared_ptr<Face>> ofaces;
  double mflow_in;
  double mflow_out;
  // Circuit* circuit;
  int node_ind;
  
  double tpres_old;
  double ttemp_old;
  double tenth_old;
  double tpres_gues;
  double ttemp_gues;
  double tenth_gues;

  double spres_old;
  double stemp_old;
  double senth_old;
  double spres_gues;
  double stemp_gues;
  double senth_gues;
  
  double heat_input;
  double elevation;
  
  std::set<std::string> fixed_var;
  double esource;
  double hresidue;
  double volume;
  shared_ptr<CoolProp::AbstractState> ther_gues;
  shared_ptr<CoolProp::AbstractState> ther_old;
  double velocity;
  double B1;

  explicit Node(pugi::xml_node flnode_node);
  Node(std::string identifier, double volume, double heat_input, double elevation, double tpres_old = 0.0, double ttemp_old = 0.0, double tenth_old = 0.0); //Circuit* circuit, 
  Node() = default;
  // virtual ~Node() = default;
  // ~Node() {
  // delete ther_old;
  // delete ther_gues;
  // }

  double eqn_cont(double time, double delt, bool trans_sim, double alpha_mom);
  void update_gues();
  void assign_staticvar();
  void update_staticvar(std::optional<double> velocity_in = std::nullopt);
  void assign_prop();
  void update_old();

  void save_to_hdf5(hid_t group_id) const;
  void load_from_hdf5(hid_t group_id);


};

//==============================================================================
// Non-member functions
//==============================================================================


} // namespace opensd

#endif // OPENSD_NODE_H
