//! \file hslab.cpp

#include "opensd/hdf5_interface.h"

#include <iostream>
#include <cmath>
#include <cstdlib>

#include "opensd/hslab.h"
#include "opensd/snode.h"
// #include "opensd/layer.h"
// #include "opensd/bc.h"
#include "opensd/error.h"
// #include "opensd/vector.h"
#include "opensd/xml_interface.h"
// #include "opensd/constants.h"

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================

namespace model {
vector<std::shared_ptr<HSlab>> hslabs;
vector<std::shared_ptr<HSlab>> hslabs_owned;
} // namespace model

//==============================================================================
// HSlab implementation
//==============================================================================

HSlab::HSlab(pugi::xml_node hslab_node)
{
  if (check_for_node(hslab_node, "identifier")) {
    this->identifier = get_node_value(hslab_node, "identifier");
  } else {
    fatal_error("Must specify identifier of hslab in geometry XML file.");
  }

  ninc = stod(get_node_value(hslab_node, "ninc"));
  uarea = stod(get_node_value(hslab_node, "uarea"));

  // Read the solid layers
  nlayers = 0;
  for (pugi::xml_node layer_node : hslab_node.children("layer")) {
    this->layers.push_back(std::make_shared<Layer>(layer_node));
 // this->layers.back()->layer_ind = this->layers.size() - 1;
 // this->layers.back()->hslab = this;
    nlayers++;
  }

//   this->eps_m = this->mean_flow = this->eps_h = this->eps_p = 0;

}

void read_hslabs(pugi::xml_node node)
{
  // Count the number of nodes
  for (pugi::xml_node hslab_node : node.children("hslab")) {
    model::hslabs.push_back(std::make_unique<HSlab>(hslab_node));
  }

}

void discretize_layers() {
  for (auto& hslab : model::hslabs) {
//
// 	  for (auto& node : circuit->nodes) {
// 	    if (pipe->unode_str == node->identifier) {
// 		  pipe->unode = node;
// 		  break;
// 	    }
//       }
// 	  for (auto& node : circuit->nodes) {
// 	    if (pipe->dnode_str == node->identifier) {
// 		  pipe->dnode = node;
// 		  break;
// 	    }
//       }
	for (auto& layer : hslab->layers) {
      // layer->hslab = hslab;
      int nnodes = layer->nnodes;
      int ninc   = hslab->ninc;
      
      double uarea = hslab->uarea;
      double darea = layer->darea;
      
      for (int i = 0; i < nnodes; ++i) {
        for (int j = 0; j < ninc; ++j) {
      
          double Ai;
          if (hslab->nlayers == 1) {
            Ai = (uarea - i * (uarea - darea) / (nnodes - 1)) / ninc;
          }
          else if (layer->layerno == 0) {
            Ai = (uarea - i * (uarea - darea) / (nnodes - 1 + 0.5)) / ninc;
          }
          else if (layer->layerno < hslab->nlayers - 1) {
            Ai = (uarea - (i + 0.5) * (uarea - darea) / nnodes) / ninc;
          }
          else {
            Ai = (uarea - (i + 0.5) * (uarea - darea) / (nnodes - 1 + 0.5)) / ninc;
          }
      
          // double delz = Ai / layer->dely;
          // double Aj   = layer->delx * delz;
          // double vol  = layer->delx * Ai;
      
          // std::string name =
            // "layer" + std::to_string(layer->layerno) +
            // "_node" + std::to_string(i) + std::to_string(j);
      
          // ---- Heat fraction and volume corrections ----
          // double heat_frac = 0.0;
      
          // if (hslab->nlayers == 1) {
            // if (i == 0 || i == nnodes - 1) {
              // vol *= 0.5;
              // heat_frac = 0.5 * hslab->AFF[j] / (nnodes - 1);
            // } else {
              // heat_frac = hslab->AFF[j] / (nnodes - 1);
            // }
          // }
          // else if (layer->layerno == 0) {
            // if (i == 0) {
              // vol *= 0.5;
              // heat_frac = 0.5 * hslab->AFF[j] / (nnodes - 1 + 0.5);
            // } else {
              // heat_frac = hslab->AFF[j] / (nnodes - 1 + 0.5);
            // }
          // }
          // else if (layer->layerno < hslab->nlayers - 1) {
            // heat_frac = hslab->AFF[j] / nnodes;
          // }
          // else {
            // if (i == nnodes - 1) {
              // vol *= 0.5;
              // heat_frac = 0.5 * hslab->AFF[j] / (nnodes - 1 + 0.5);
            // } else {
              // heat_frac = hslab->AFF[j] / (nnodes - 1 + 0.5);
            // }
          // }
      
          // ---- Create node ----
          // auto node = add_SNode(name, Ai, Aj, vol, layer->solname,
                                // layer->sollib, heat_frac, layer);
      
          // layer->nodes.push_back(node);
      
          // ---- Upwind/downwind registration ----
          // if (i == 0 && layer->layerno == 0) {
            // hslab->uwnodes.push_back(node);
            // node->AFF = hslab->AFF[j];
          // }
      
          // if (i == nnodes - 1) {
            // hslab->dwnodes.push_back(node);
            // node->AFF = hslab->AFF[j];
            // hslab->darea = darea;
          // }
        }
      }
	}
  }
}
//
// void initialize_circuits() {
//
// for (auto& circuit : model::circuits) {
//   for (auto& node : circuit->nodes) {
// 	bool trans_sim = settings::run_mode == RunMode::TRANSIENT;
// 	if (not trans_sim) {
//       node->assign_staticvar();
// 	}
//     node->assign_prop();
//     node->update_gues();
//   }
//   for (auto& face : circuit->faces) {
//     face->assign_statevar();
//     face->assign_prop();
//     face->update_gues();
//
//   }
// }

// }


// void Circuit::save_to_hdf5(hid_t group_id) const {
//   write_string(group_id, "identifier", identifier);
//   write_string(group_id, "flname", flname);
//   write_string(group_id, "fltype", fluid_type_to_string(fltype));
//   write_scalar(group_id, "eps_m", eps_m);
//   write_scalar(group_id, "mean_flow", mean_flow);
//   write_scalar(group_id, "eps_h", eps_h);
//   write_scalar(group_id, "eps_p", eps_p);
//   write_vector(group_id, "Pbound_ind", Pbound_ind);
//
//    // Save Nodes
//   hid_t node_group = H5Gcreate(group_id, "nodes", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
//   for (size_t i = 0; i < nodes_owned.size(); ++i) {
//     std::string name = "node_" + std::to_string(i);
//     hid_t ngrp = H5Gcreate(node_group, name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
//     nodes_owned[i]->save_to_hdf5(ngrp);
//     H5Gclose(ngrp);
//   }
//   H5Gclose(node_group);
//
//   // Save Pipes
//   hid_t pipe_group = H5Gcreate(group_id, "pipes", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
//   for (size_t i = 0; i < pipes.size(); ++i) {
//     std::string name = "pipe_" + std::to_string(i);
//     hid_t pgrp = H5Gcreate(pipe_group, name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
//     pipes[i]->save_to_hdf5(pgrp);
//     H5Gclose(pgrp);
//   }
//   H5Gclose(pipe_group);
//
//   // Save BCs
//   hid_t bc_group = H5Gcreate(group_id, "bcs", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
//   for (size_t i = 0; i < bcs.size(); ++i) {
//     std::string name = "bc_" + std::to_string(i);
//     hid_t bcgrp = H5Gcreate(bc_group, name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
//     bcs[i].save_to_hdf5(bcgrp);
//     H5Gclose(bcgrp);
//   }
//   H5Gclose(bc_group);
//
//   // Save Faces
//   hid_t face_group = H5Gcreate(group_id, "faces", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
//   for (size_t i = 0; i < faces_owned.size(); ++i) {
//     std::string name = "face_" + std::to_string(i);
//     hid_t fgrp = H5Gcreate(face_group, name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
//     faces_owned[i]->save_to_hdf5(fgrp);
//     H5Gclose(fgrp);
//   }
//    H5Gclose(face_group);
//
// }
//
// void Circuit::load_from_hdf5(hid_t group_id) {
//   identifier = read_string(group_id, "identifier");
//   flname = read_string(group_id, "flname");
//   std::string fltype_str = read_string(group_id, "fltype");
//   fltype = string_to_fluid_type(fltype_str);
//   eps_m = read_scalar(group_id, "eps_m");
//   mean_flow = read_scalar(group_id, "mean_flow");
//   eps_h = read_scalar(group_id, "eps_h");
//   eps_p = read_scalar(group_id, "eps_p");
//   Pbound_ind = read_vector_int(group_id, "Pbound_ind");
//
//    // Load Nodes
//   hid_t node_group = H5Gopen(group_id, "nodes", H5P_DEFAULT);
//   for (size_t i = 0; i < nodes.size(); ++i) {
//     std::string name = "node_" + std::to_string(i);
//     if (H5Lexists(node_group, name.c_str(), H5P_DEFAULT) > 0) {
//       hid_t ngrp = H5Gopen(node_group, name.c_str(), H5P_DEFAULT);
//       if (nodes[i]) nodes[i]->load_from_hdf5(ngrp);
//       H5Gclose(ngrp);
//     }
//   }
//   H5Gclose(node_group);
//
//   // Load Pipes
//   hid_t pipe_group = H5Gopen(group_id, "pipes", H5P_DEFAULT);
//   for (size_t i = 0; i < pipes.size(); ++i) {
//     std::string name = "pipe_" + std::to_string(i);
//     if (H5Lexists(pipe_group, name.c_str(), H5P_DEFAULT) <= 0) {
//       std::cerr << "Warning: pipe group '" << name << "' missing in HDF5\n";
//       continue;
//     }
//     hid_t pgrp = H5Gopen(pipe_group, name.c_str(), H5P_DEFAULT);
//     if (pipes[i]) {
//       pipes[i]->load_from_hdf5(pgrp);
//     } else {
//       std::cerr << "Warning: pipes[" << i << "] is null\n";
//     }
//     H5Gclose(pgrp);
//   }
//   H5Gclose(pipe_group);
//
//   // Load BCs
//   if (bcs.size() > 0)
//     std::cerr << "Warning: Overwriting existing BC values\n";
//   hid_t bc_group = H5Gopen(group_id, "bcs", H5P_DEFAULT);
//   for (size_t i = 0; i < bcs.size(); ++i) {
//     std::string name = "bc_" + std::to_string(i);
//     if (H5Lexists(bc_group, name.c_str(), H5P_DEFAULT) <= 0) {
//       std::cerr << "Warning: bc group '" << name << "' missing in HDF5\n";
//       continue;
//     }
//     hid_t bcgrp = H5Gopen(bc_group, name.c_str(), H5P_DEFAULT);
//     bcs[i].load_from_hdf5(bcgrp);
//     H5Gclose(bcgrp);
//   }
//   H5Gclose(bc_group);
//
//   // Load Faces
//   hid_t face_group = H5Gopen(group_id, "faces", H5P_DEFAULT);
//   for (size_t i = 0; i < faces.size(); ++i) {
//     std::string name = "face_" + std::to_string(i);
//     if (H5Lexists(face_group, name.c_str(), H5P_DEFAULT) <= 0) {
//       std::cerr << "Warning: face group '" << name << "' missing in HDF5\n";
//       continue;
//     }
//     hid_t fgrp = H5Gopen(face_group, name.c_str(), H5P_DEFAULT);
//     if (faces[i]) {
//       faces[i]->load_from_hdf5(fgrp);
//     } else {
//       std::cerr << "Warning: faces[" << i << "] is null\n";
//     }
//     H5Gclose(fgrp);
//   }
//   H5Gclose(face_group);
//
//
// }
//
// Node* Circuit::get_node_by_identifier(const std::string& id) const
// {
//     for (auto& n : nodes) {
//         if (n->identifier == id) return n.get();
//     }
//     return nullptr; // not found
// }

} // namespace opensd
