//! \file bc.h

#ifndef OPENSD_BC_H
#define OPENSD_BC_H

#include "pugixml.hpp"
#include "opensd/vector.h"
#include "hdf5_interface.h"

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
  BC() = default;
  
  void load_from_hdf5(hid_t group_id);
  void save_to_hdf5(hid_t group_id) const;
  
protected:

};

//==============================================================================
// Non-member functions
//==============================================================================

} // namespace opensd

#endif // OPENSD_PIPE_H
