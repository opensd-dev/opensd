//! \file circuit.h

#ifndef OPENSD_CIRCUIT_H
#define OPENSD_CIRCUIT_H

#include "pugixml.hpp"
#include "opensd/node.h"
#include "opensd/pipe.h"
#include "opensd/bc.h"
#include "opensd/face.h"
#include "opensd/vector.h"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/string.hpp>

#include <boost/serialization/serialization.hpp>

#include <boost/serialization/set.hpp>

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
  vector<std::shared_ptr<Pipe>> pipes;
  vector<BC> bcs;
  vector<std::shared_ptr<Face>> faces;
  vector<int> Pbound_ind;

protected:

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & identifier;
    ar & flname;
    ar & eps_m;
    ar & mean_flow;
    ar & eps_h;
    ar & eps_p;
    ar & nodes;
    ar & pipes;
//    ar & bcs;
//    ar & faces;
    ar & Pbound_ind;
  }

};

//==============================================================================
// Non-member functions
//==============================================================================

void read_circuits(pugi::xml_node node);
void initialize_circuits();

} // namespace opensd

#endif // OPENSD_CIRCUIT_H
