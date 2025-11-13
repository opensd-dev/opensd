#include "opensd/geometry.h"
#include <pugixml.hpp>

#include <iostream>

#include "opensd/error.h"
#include "opensd/circuit.h"
#include "opensd/hslab.h"

namespace opensd {

void read_geometry_xml()
{
  // Display output message
  write_message("Reading geometry XML file...", 5);

  // Check if geometry.xml exists
  std::string filename = settings::path_input + "geometry.xml";
  // if (!file_exists(filename)) {
    // fatal_error("Geometry XML file '" + filename + "' does not exist!");
  // }

  // Parse settings.xml file
  pugi::xml_document doc;
  auto result = doc.load_file(filename.c_str());
  if (!result) {
    fatal_error("Error processing geometry.xml file.");
  }

  // Get root element
  pugi::xml_node root = doc.document_element();

  read_geometry_xml(root);
}

void read_geometry_xml(pugi::xml_node root)
{
  // Read circuits, heatslabs
  read_circuits(root);
  read_hslabs(root);

}

} // namespace opensd
