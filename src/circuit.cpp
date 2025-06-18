//! \file circuit.cpp

#include "opensd/hdf5_interface.h"

#include <iostream>
#include <cmath>
#include <cstdlib>

#include "opensd/circuit.h"
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


void Circuit::save_to_hdf5(hid_t group_id) const {
  write_string(group_id, "identifier", identifier);
  write_string(group_id, "flname", flname);
  write_scalar(group_id, "eps_m", eps_m);
  write_scalar(group_id, "mean_flow", mean_flow);
  write_scalar(group_id, "eps_h", eps_h);
  write_scalar(group_id, "eps_p", eps_p);
  write_vector(group_id, "Pbound_ind", Pbound_ind);

   // Save Nodes
  hid_t node_group = H5Gcreate(group_id, "nodes", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  for (size_t i = 0; i < nodes.size(); ++i) {
    std::string name = "node_" + std::to_string(i);
    hid_t ngrp = H5Gcreate(node_group, name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    nodes[i]->save_to_hdf5(ngrp);
    H5Gclose(ngrp);
  }
  H5Gclose(node_group);

  // Save Pipes
  hid_t pipe_group = H5Gcreate(group_id, "pipes", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  for (size_t i = 0; i < pipes.size(); ++i) {
    std::string name = "pipe_" + std::to_string(i);
    hid_t pgrp = H5Gcreate(pipe_group, name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    pipes[i]->save_to_hdf5(pgrp);
    H5Gclose(pgrp);
  }
  H5Gclose(pipe_group);

  // Save BCs
  hid_t bc_group = H5Gcreate(group_id, "bcs", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  for (size_t i = 0; i < bcs.size(); ++i) {
    std::string name = "bc_" + std::to_string(i);
    hid_t bcgrp = H5Gcreate(bc_group, name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    bcs[i].save_to_hdf5(bcgrp);
    H5Gclose(bcgrp);
  }
  H5Gclose(bc_group);

  // Save Faces
  hid_t face_group = H5Gcreate(group_id, "faces", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  for (size_t i = 0; i < faces.size(); ++i) {
    std::string name = "face_" + std::to_string(i);
    hid_t fgrp = H5Gcreate(face_group, name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    faces[i]->save_to_hdf5(fgrp);
    H5Gclose(fgrp);
  }
   H5Gclose(face_group);

}

void Circuit::load_from_hdf5(hid_t group_id) {
  identifier = read_string(group_id, "identifier");
  flname = read_string(group_id, "flname");
  eps_m = read_scalar(group_id, "eps_m");
  mean_flow = read_scalar(group_id, "mean_flow");
  eps_h = read_scalar(group_id, "eps_h");
  eps_p = read_scalar(group_id, "eps_p");
  Pbound_ind = read_vector_int(group_id, "Pbound_ind");

   // Load Nodes
  hid_t node_group = H5Gopen(group_id, "nodes", H5P_DEFAULT);
  for (size_t i = 0; i < nodes.size(); ++i) {
    std::string name = "node_" + std::to_string(i);
    if (H5Lexists(node_group, name.c_str(), H5P_DEFAULT) > 0) {
      hid_t ngrp = H5Gopen(node_group, name.c_str(), H5P_DEFAULT);
      if (nodes[i]) nodes[i]->load_from_hdf5(ngrp);
      H5Gclose(ngrp);
    }
  }
  H5Gclose(node_group);

  // Load Pipes
  hid_t pipe_group = H5Gopen(group_id, "pipes", H5P_DEFAULT);
  for (size_t i = 0; i < pipes.size(); ++i) {
    std::string name = "pipe_" + std::to_string(i);
    if (H5Lexists(pipe_group, name.c_str(), H5P_DEFAULT) <= 0) {
      std::cerr << "Warning: pipe group '" << name << "' missing in HDF5\n";
      continue;
    }
    hid_t pgrp = H5Gopen(pipe_group, name.c_str(), H5P_DEFAULT);
    if (pipes[i]) {
      pipes[i]->load_from_hdf5(pgrp);
    } else {
      std::cerr << "Warning: pipes[" << i << "] is null\n";
    }
    H5Gclose(pgrp);
  }
  H5Gclose(pipe_group);

  // Load BCs
  if (bcs.size() > 0)
    std::cerr << "Warning: Overwriting existing BC values\n";
  hid_t bc_group = H5Gopen(group_id, "bcs", H5P_DEFAULT);
  for (size_t i = 0; i < bcs.size(); ++i) {
    std::string name = "bc_" + std::to_string(i);
    if (H5Lexists(bc_group, name.c_str(), H5P_DEFAULT) <= 0) {
      std::cerr << "Warning: bc group '" << name << "' missing in HDF5\n";
      continue;
    }
    hid_t bcgrp = H5Gopen(bc_group, name.c_str(), H5P_DEFAULT);
    bcs[i].load_from_hdf5(bcgrp);
    H5Gclose(bcgrp);
  }
  H5Gclose(bc_group);

  // Load Faces
  hid_t face_group = H5Gopen(group_id, "faces", H5P_DEFAULT);
  for (size_t i = 0; i < faces.size(); ++i) {
    std::string name = "face_" + std::to_string(i);
    if (H5Lexists(face_group, name.c_str(), H5P_DEFAULT) <= 0) {
      std::cerr << "Warning: face group '" << name << "' missing in HDF5\n";
      continue;
    }
    hid_t fgrp = H5Gopen(face_group, name.c_str(), H5P_DEFAULT);
    if (faces[i]) {
      faces[i]->load_from_hdf5(fgrp);
    } else {
      std::cerr << "Warning: faces[" << i << "] is null\n";
    }
    H5Gclose(fgrp);
  }
  H5Gclose(face_group);


}
  

} // namespace opensd
