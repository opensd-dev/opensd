//! \file circuit.cpp

#include "opensd/circuit.h"
using namespace H5;

#include <iostream>
#include <cmath>
#include <cstdlib>

#include "opensd/node.h"
#include "opensd/pipe.h"
#include "opensd/bc.h"
#include "opensd/error.h"
#include "opensd/vector.h"
#include "opensd/xml_interface.h"
#include "opensd/constants.h"

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================

namespace model {
vector<std::shared_ptr<Circuit>> circuits;
} // namespace model

//==============================================================================
// Circuit implementation
//==============================================================================

Circuit::Circuit(pugi::xml_node cir_node)
{
  if (check_for_node(cir_node, "identifier")) {
    this->identifier = get_node_value(cir_node, "identifier");
  } else {
    fatal_error("Must specify identifier of circuit in geometry XML file.");
  }
        
  // Read the circuit fluid
  this->flname = get_node_value(cir_node, "flname");
  
  // Read the fluid nodes, pipes and bcs
  for (pugi::xml_node flnode_node : cir_node.children("node")) {
    this->nodes.push_back(std::make_shared<Node>(flnode_node));
    this->nodes.back()->node_ind = this->nodes.size() - 1;
  }

  for (pugi::xml_node pipe : cir_node.children("pipe")) {
    this->pipes.push_back(std::make_shared<Pipe>(pipe));
  }

  // for (pugi::xml_node face : cir_node.children("face")) {
    // this->faces.push_back(std::make_shared<Face>(face));
  // }

  for (pugi::xml_node bc : cir_node.children("bc")) {
    this->bcs.push_back(BC(bc));
  }


  std::string pbound_ind_str = cir_node.attribute("Pbound_ind").value();
  if (!pbound_ind_str.empty()) {
    std::istringstream iss(pbound_ind_str);
    std::string index_str;
    while (std::getline(iss, index_str, ',')) {
      int node_index = std::stoi(index_str);
      this->Pbound_ind.push_back(node_index);
    }
  }

  
  this->eps_m = this->mean_flow = this->eps_h = this->eps_p = 0;

}

void read_circuits(pugi::xml_node node)
{
  // Count the number of nodes
  for (pugi::xml_node cir_node : node.children("circuit")) {
    model::circuits.push_back(std::make_shared<Circuit>(cir_node));
  }

}

void discretize_pipes() {
  for (auto& circuit : model::circuits) {
	for (auto& pipe : circuit->pipes) {
      pipe->circuit = circuit;

	  for (auto& node : circuit->nodes) {
	    if (pipe->unode_str == node->identifier) {
		  pipe->unode = node;
		  break;
	    }
      }
	  for (auto& node : circuit->nodes) {
	    if (pipe->dnode_str == node->identifier) {
		  pipe->dnode = node;
		  break;
	    }
      }
	  
      vector<std::shared_ptr<Node>> nodes;
      //for (int i = 0; i < pipe->ncell-1; ++i) {
        // Node node = Node();
        // node->identifier = identifier + "_node" + std::to_string(i);
        // node->height = unode->height + (dnode->height - unode->height) * (i + 1) / ncell;
        // nodes.push_back(node);
      //}
      double ufrac;
      double dfrac;
      double cfarea = PI*std::pow(pipe->diameter,2)/4.;
      double delx = pipe->length/pipe->ncell;
      double delz = 0.;
      double fricopt;

      for (int i = 0; i < pipe->ncell; ++i) {
        if (i == 0 && pipe->ncell == 1) {
          pipe->faces.push_back(std::make_shared<PFace>(i, pipe, pipe->unode, ufrac, pipe->dnode, dfrac, pipe->diameter, cfarea, delx, delz, fricopt, pipe->roughness));
        } else if (i == 0) {
          pipe->faces.push_back(std::make_shared<PFace>(i, pipe, pipe->unode, ufrac, circuit->nodes[2], -1, pipe->diameter, cfarea, delx, delz, fricopt, pipe->roughness));
        } else if (i == pipe->ncell-1) {
          pipe->faces.push_back(std::make_shared<PFace>(i, pipe, circuit->nodes[pipe->ncell-2+2], -1, pipe->dnode, dfrac, pipe->diameter, cfarea, delx, delz, fricopt, pipe->roughness));
        } else {
          pipe->faces.push_back(std::make_shared<PFace>(i, pipe, circuit->nodes[i+2-1], -1, circuit->nodes[i+2], -1, pipe->diameter, cfarea, delx, delz, fricopt, pipe->roughness));
        }
        circuit->faces.push_back(pipe->faces.back());
      }

      for (int i = 0; i < pipe->ncell-1; ++i) {
        circuit->nodes[i+2]->ifaces.push_back(pipe->faces[i]);
        circuit->nodes[i+2]->ofaces.push_back(pipe->faces[i+1]);
      }
     

      pipe->unode->ofaces.push_back(pipe->faces[0]);
    
      pipe->dnode->ifaces.push_back(pipe->faces[pipe->ncell-1]);
	}
  }
}

void initialize_circuits() {

for (auto& circuit : model::circuits) {
  for (auto& node : circuit->nodes) {
    node->assign_staticvar();
    node->assign_prop();
    node->update_gues();
  }
  for (auto& face : circuit->faces) {
    face->assign_statevar();
    face->assign_prop();
    face->update_gues();

  }
}

}


void Circuit::save_to_hdf5(H5::Group& parent, size_t index) const {
  // Create a group named "circuit_0", "circuit_1", etc.
  std::string group_name = "circuit_" + std::to_string(index);
  H5::Group g = parent.createGroup(group_name);

  // Save string attributes
  H5::StrType str_type(H5::PredType::C_S1, H5T_VARIABLE);
  H5::DataSpace scalar_space = H5::DataSpace(H5S_SCALAR);

  H5::DataSet id_ds = g.createDataSet("identifier", str_type, scalar_space);
  id_ds.write(identifier, str_type);

  H5::DataSet fname_ds = g.createDataSet("flname", str_type, scalar_space);
  fname_ds.write(flname, str_type);

  // Save scalars
  g.createDataSet("eps_m", H5::PredType::NATIVE_DOUBLE, scalar_space).write(&eps_m, H5::PredType::NATIVE_DOUBLE);
  g.createDataSet("mean_flow", H5::PredType::NATIVE_DOUBLE, scalar_space).write(&mean_flow, H5::PredType::NATIVE_DOUBLE);
  g.createDataSet("eps_h", H5::PredType::NATIVE_DOUBLE, scalar_space).write(&eps_h, H5::PredType::NATIVE_DOUBLE);
  g.createDataSet("eps_p", H5::PredType::NATIVE_DOUBLE, scalar_space).write(&eps_p, H5::PredType::NATIVE_DOUBLE);

  // Save Pbound_ind
  if (!Pbound_ind.empty()) {
    hsize_t dims[1] = {Pbound_ind.size()};
    H5::DataSpace space(1, dims);
    H5::DataSet pb_ds = g.createDataSet("Pbound_ind", H5::PredType::NATIVE_INT, space);
    pb_ds.write(Pbound_ind.data(), H5::PredType::NATIVE_INT);
  }
}

void Circuit::load_from_hdf5(const H5::Group& parent, size_t index) {
  std::string group_name = "circuit_" + std::to_string(index);
  H5::Group g = parent.openGroup(group_name);

  // String attributes
  H5::StrType str_type(H5::PredType::C_S1, H5T_VARIABLE);
  H5::DataSpace scalar_space(H5S_SCALAR);

  H5::DataSet id_ds = g.openDataSet("identifier");
  id_ds.read(identifier, str_type);

  H5::DataSet fname_ds = g.openDataSet("flname");
  fname_ds.read(flname, str_type);

  // Scalars
  g.openDataSet("eps_m").read(&eps_m, H5::PredType::NATIVE_DOUBLE);
  g.openDataSet("mean_flow").read(&mean_flow, H5::PredType::NATIVE_DOUBLE);
  g.openDataSet("eps_h").read(&eps_h, H5::PredType::NATIVE_DOUBLE);
  g.openDataSet("eps_p").read(&eps_p, H5::PredType::NATIVE_DOUBLE);

  // Pbound_ind
  if (g.nameExists("Pbound_ind")) {
    H5::DataSet pb_ds = g.openDataSet("Pbound_ind");
    H5::DataSpace pb_space = pb_ds.getSpace();
    hsize_t dims[1];
    pb_space.getSimpleExtentDims(dims);
    Pbound_ind.resize(dims[0]);
    pb_ds.read(Pbound_ind.data(), H5::PredType::NATIVE_INT);
  }
}

} // namespace opensd
