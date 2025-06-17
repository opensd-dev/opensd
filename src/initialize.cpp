//! \file initialize.cpp
#include "opensd/initialize.h"

#include <iostream>

// #include "opensd/capi.h"
// #include "opensd/constants.h"
#include "opensd/settings.h"
#include "opensd/geometry.h"

int opensd_init(int argc, char* argv[])
{
  using namespace opensd;

  // Read XML input files
  // if (!read_model_xml())
  read_separate_xml_files();
  discretize_pipes();
  
  
  try {
  hid_t file_id = H5Fopen("circuits.h5", H5F_ACC_RDONLY, H5P_DEFAULT);

  size_t index = 0;
  for (auto& circuit : model::circuits) {
    std::string group_name = "/circuits/circuit_" + std::to_string(index);
    if (H5Lexists(file_id, group_name.c_str(), H5P_DEFAULT) <= 0) {
      std::cerr << "Warning: Expected circuit group not found: " << group_name << "\n";
      break;
    }

    hid_t group_id = H5Gopen(file_id, group_name.c_str(), H5P_DEFAULT);
    if (!circuit) {
      std::cerr << "Error: model::circuits[" << index << "] is null. Skipping.\n";
    } else {
      circuit->load_from_hdf5(group_id);  // update existing object
    }
    H5Gclose(group_id);
    ++index;
  }

  H5Fclose(file_id);

  std::cout << "Loaded data into " << index << " existing circuit(s) from HDF5.\n";

} catch (const std::exception& e) {
  std::cerr << "HDF5 error during load: " << e.what() << std::endl;
}

initialize_circuits(); //assign properties


// Print to verify
for (size_t i = 0; i < model::circuits.size(); ++i) {
  const auto& circuit = model::circuits[i];
  std::cout << "Circuit [" << i << "] ID: " << circuit->identifier << "\n";
  std::cout << "  mean_flow: " << circuit->mean_flow << "\n";
  std::cout << "  eps_h: " << circuit->eps_h << "\n";

  // Nodes
  std::cout << "  Nodes (" << circuit->nodes.size() << "):\n";
  for (size_t j = 0; j < circuit->nodes.size(); ++j) {
    const auto& node = circuit->nodes[j];
    if (node) {
      std::cout << "    Node [" << j << "] ID: " << node->identifier << "\n";
      std::cout << "      volume: " << node->volume << "\n";
      std::cout << "      mflow_in: " << node->mflow_in << "\n";
      std::cout << "      mflow_out: " << node->mflow_out << "\n";
    } else {
      std::cout << "    Node [" << j << "] is null\n";
    }
  }

  // Pipes
  std::cout << "  Pipes (" << circuit->pipes.size() << "):\n";
  for (size_t j = 0; j < circuit->pipes.size(); ++j) {
    const auto& pipe = circuit->pipes[j];
    if (pipe) {
      std::cout << "    Pipe [" << j << "] ID: " << pipe->identifier << "\n";
      std::cout << "      length: " << pipe->length << "\n";
      std::cout << "      diameter: " << pipe->diameter << "\n";
    } else {
      std::cout << "    Pipe [" << j << "] is null\n";
    }
  }

  // BCs
  std::cout << "  BCs (" << circuit->bcs.size() << "):\n";
  for (size_t j = 0; j < circuit->bcs.size(); ++j) {
    const auto& bc = circuit->bcs[j];
    std::cout << "    BC [" << j << "] ID: " << bc.identifier << "\n";
    std::cout << "      node: " << bc.node_ << "\n";
    std::cout << "      var: " << bc.var_ << "\n";
    std::cout << "      val: " << bc.val_ << "\n";
  }
  
    // Faces
  std::cout << "  Faces (" << circuit->faces.size() << "):\n";
  for (size_t j = 0; j < circuit->faces.size(); ++j) {
    const auto& face = circuit->faces[j];
    if (face) {
      std::cout << "    Face [" << j << "] faceno: " << face->faceno << "\n";

      if (face->unode) {
        std::cout << "      unode ID: " << face->unode->identifier << "\n";
        std::cout << "      ufrac: " << face->ufrac << ", uheight: " << face->uheight << "\n";
      } else {
        std::cout << "      unode is null\n";
      }

      if (face->dnode) {
        std::cout << "      dnode ID: " << face->dnode->identifier << "\n";
        std::cout << "      dfrac: " << face->dfrac << ", dheight: " << face->dheight << "\n";
      } else {
        std::cout << "      dnode is null\n";
      }

      std::cout << "      mflow: " << face->mflow << ", velocity: " << face->velocity << "\n";
      std::cout << "      choked: " << std::boolalpha << face->choked << "\n";
      std::cout << "      heat_input: " << face->heat_input << "\n";
      // std::cout << "      heat_hslab size: " << face->heat_hslab.size() << "\n";

    } else {
      std::cout << "    Face [" << j << "] is null\n";
    }
  }

  
}

	
	
	
  return 0;
}

namespace opensd {
    
void read_separate_xml_files()
{
  read_settings_xml();
  read_geometry_xml();

}

} // namespace opensd
