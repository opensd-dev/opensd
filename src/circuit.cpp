//! \file circuit.cpp

#include "opensd/circuit.h"

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

} // namespace opensd
