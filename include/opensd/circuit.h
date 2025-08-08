//! \file circuit.h

#ifndef OPENSD_CIRCUIT_H
#define OPENSD_CIRCUIT_H

#include "pugixml.hpp"
#include "opensd/node.h"
#include "opensd/pipe.h"
#include "opensd/bc.h"
#include "opensd/face.h"
#include "opensd/vector.h"
#include "hdf5_interface.h"

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================

namespace model {
extern vector<std::shared_ptr<Circuit>> circuits;
} // namespace model

class Circuit;

//==============================================================================
//! \class Circuit
//==============================================================================

class Circuit {
public:
  std::string identifier; //!< User-defined identifier
  std::string flname;
  explicit Circuit(pugi::xml_node cir_node);
  Circuit() = default;
  double eps_m;
  double mean_flow;
  double eps_h;
  double eps_p;
  vector<std::shared_ptr<Node>> nodes;
  vector<std::shared_ptr<Node>> nodes_owned;
  // vector<int> indices_owned;
  vector<std::shared_ptr<Pipe>> pipes;
  vector<BC> bcs;
  vector<std::shared_ptr<Face>> faces;
  vector<int> Pbound_ind;

  void save_to_hdf5(hid_t group_id) const;
  void load_from_hdf5(hid_t group_id);

protected:

};

//==============================================================================
// Non-member functions
//==============================================================================

void read_circuits(pugi::xml_node node);
void initialize_circuits();

} // namespace opensd

#endif // OPENSD_CIRCUIT_H
