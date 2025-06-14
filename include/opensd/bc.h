//! \file bc.h

#ifndef OPENSD_BC_H
#define OPENSD_BC_H

#include "pugixml.hpp"
#include "opensd/vector.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/string.hpp>

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================

class BC;

//==============================================================================
//! \class BC
//==============================================================================

class BC {
public:
  std::string identifier; //!< User-defined identifier
  std::string node_;
  std::string var_;
  double val_; //!< value in [SI]
  explicit BC(pugi::xml_node bc_node);
  BC() = default; // Required for serialization

private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & identifier;
    ar & node_;
    ar & var_;
    ar & val_;
}

};

//==============================================================================
// Non-member functions
//==============================================================================

} // namespace opensd

#endif // OPENSD_PIPE_H
