//! \file node.h

#ifndef OPENSD_NODE_H
#define OPENSD_NODE_H

#include "pugixml.hpp"

// #include "opensd/circuit.h"
#include "opensd/vector.h"
#include "CoolProp.h"
#include "AbstractState.h"
#include "crossplatform_shared_ptr.h"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/string.hpp>

#include <boost/serialization/serialization.hpp>

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

  explicit Node(pugi::xml_node flnode_node);
  Node(std::string identifier, double volume, double heat_input, double elevation, double tpres_old = 0.0, double ttemp_old = 0.0, double tenth_old = 0.0); //Circuit* circuit, 
  Node() = default;
  // virtual ~Node() = default;
  // ~Node() {
  // delete ther_old;
  // delete ther_gues;
  // }

  double eqn_cont(double alpha_mom);
  void update_gues();
  void assign_staticvar();
  void update_staticvar();
  void assign_prop();

private:  
friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & identifier;
    ar & mresidue;
    ar & msource;
    // ar & ifaces;
    // ar & ofaces;
    ar & mflow_in;
    ar & mflow_out;
    ar & node_ind;

    ar & tpres_old;
    ar & ttemp_old;
    ar & tenth_old;
    ar & tpres_gues;
    ar & ttemp_gues;
    ar & tenth_gues;

    ar & spres_old;
    ar & stemp_old;
    ar & senth_old;
    ar & spres_gues;
    ar & stemp_gues;
    ar & senth_gues;

    ar & heat_input;
    ar & elevation;
    ar & fixed_var;
    ar & esource;
    ar & hresidue;
    ar & volume;
  }
  
};

//==============================================================================
// Non-member functions
//==============================================================================


} // namespace opensd

#endif // OPENSD_NODE_H
