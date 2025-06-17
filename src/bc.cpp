#include "opensd/bc.h"

#include <iostream>

#include "opensd/error.h"
#include "opensd/xml_interface.h"

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================

//==============================================================================
// BC implementation
//==============================================================================

BC::BC(pugi::xml_node bc_node)
{
  if (check_for_node(bc_node, "identifier")) {
    identifier = get_node_value(bc_node, "identifier");
  } else {
    fatal_error("Must specify identifier of BC in geometry XML file.");
  }

  val_  = stod(get_node_value(bc_node, "val"));
  var_  = get_node_value(bc_node, "var");
  node_ = get_node_value(bc_node, "node");

}

void BC::save_to_hdf5(hid_t group_id) const {
  write_string(group_id, "identifier", identifier);
  write_string(group_id, "node", node_);
  write_string(group_id, "var", var_);
  write_scalar(group_id, "val", val_);
}

void BC::load_from_hdf5(hid_t group_id) {
  identifier = read_string(group_id, "identifier");
  node_ = read_string(group_id, "node");
  var_ = read_string(group_id, "var");
  val_ = read_scalar(group_id, "val");
}

} // namespace opensd
