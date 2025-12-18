//! \file hslab.cpp

#include "opensd/hdf5_interface.h"

#include <iostream>
#include <cmath>
#include <cstdlib>

#include "opensd/hslab.h"
#include "opensd/circuit.h"
// #include "opensd/snode.h"
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
    this->layers.back()->hslab = this;
    nlayers++;
  }
  uvar = get_node_value(hslab_node, "uvar");
  dvar = get_node_value(hslab_node, "dvar");
  ucompid = get_node_value(hslab_node, "ucomp");
  dcompid = get_node_value(hslab_node, "dcomp");
  uval = stod(get_node_value(hslab_node, "uval"));
  dval = stod(get_node_value(hslab_node, "dval"));

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


  hslab->upipe = get_comp<Pipe>(hslab->ucompid);

  if (hslab->upipe) {
    std::cout << "Found pipe with identifier: " << hslab->upipe->identifier << std::endl;
  } else {
    std::cout << "Upstream Pipe not found." << std::endl;
  }

  hslab->uval1.assign(hslab->upipe->faces.begin(),
             hslab->upipe->faces.begin() + hslab->upipe->ncell);

  hslab->dpipe = get_comp<Pipe>(hslab->dcompid);

  if (hslab->dpipe) {
    std::cout << "Found pipe with identifier: " << hslab->dpipe->identifier << std::endl;
  } else {
    std::cout << "Downstream Pipe not found." << std::endl;
  }


  std::string config = "counter";
  if (config == "counter") {
    hslab->dval1.assign(hslab->dpipe->faces.rbegin(),
                hslab->dpipe->faces.rbegin() + hslab->dpipe->ncell);
  } else {
    hslab->dval1.assign(hslab->dpipe->faces.begin(),
                 hslab->dpipe->faces.begin() + hslab->dpipe->ncell);
  }



	for (auto& layer : hslab->layers) {
      // layer->hslab = hslab;
      int nnodes = layer->nnodes;
      int ninc   = hslab->ninc;
      
      double uarea = hslab->uarea;
      double darea = layer->darea;

	  double thk_elem = layer->thk_elem;
	  double thk_cros = layer->thk_cros;
	  
      if (hslab->nlayers == 1) {
        layer->delx = thk_elem/(nnodes-1);
      }
      else if (layer->layerno == 0) { //first layer
        layer->delx = thk_elem/(nnodes-1+0.5);
	  }
	  else if (layer->layerno < hslab->nlayers-1) {
        layer->delx = thk_elem/nnodes;
	  }
      else { //last layer
        layer->delx = thk_elem/(nnodes-1+0.5);
	  }
	  	  
	  double dely = thk_cros/ninc;

      std::vector<double> AFF(ninc, 1.0 / ninc);

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
      
          double delz = Ai / dely;
          double Aj   = layer->delx * delz;
          double vol  = layer->delx * Ai;
      
          std::string name =
            "layer" + std::to_string(layer->layerno) +
            "_node" + std::to_string(i) + std::to_string(j);
      
          // ---- Heat fraction and volume corrections ----
          double heat_frac = 0.0;
      
          if (hslab->nlayers == 1) {
            if (i == 0 || i == nnodes - 1) {
              vol *= 0.5;
              heat_frac = 0.5 * AFF[j] / (nnodes - 1);
            } else {
              heat_frac = AFF[j] / (nnodes - 1);
            }
          }
          else if (layer->layerno == 0) {
            if (i == 0) {
              vol *= 0.5;
              heat_frac = 0.5 * AFF[j] / (nnodes - 1 + 0.5);
            } else {
              heat_frac = AFF[j] / (nnodes - 1 + 0.5);
            }
          }
          else if (layer->layerno < hslab->nlayers - 1) {
            heat_frac = AFF[j] / nnodes;
          }
          else {
            if (i == nnodes - 1) {
              vol *= 0.5;
              heat_frac = 0.5 * AFF[j] / (nnodes - 1 + 0.5);
            } else {
              heat_frac = AFF[j] / (nnodes - 1 + 0.5);
            }
          }
      
          // ---- Create node ----
          // auto node = add_SNode(name, Ai, Aj, vol, layer->solname,
                                // layer->sollib, heat_frac, layer);
          auto snode = std::make_shared<SNode>(name);
          // def __init__(self,identifier,Ai=None,Aj=None,vol=None,solname=None,sollib=None,heat_frac=np.array(1.),layer=None):
          snode->Ai = Ai;
          snode->Aj = Aj;
          snode->vol = vol;
          snode->heat_frac = heat_frac;
		  snode->solname = layer->solname;
		  snode->layer = layer.get();

          // ---- Upwind/downwind registration ----
          if (i == 0 && layer->layerno == 0) {
            snode->AFF = AFF[j];
			hslab->uwnodes.push_back(snode);
          }
      
          if (i == nnodes - 1) {
            hslab->dwnodes.push_back(snode);
            snode->AFF = AFF[j];
            hslab->darea = darea;
          }
          snode->node_ind = layer->snodes.size();
          layer->snodes.push_back(snode);

        }
      }

      for (int i = 0; i < nnodes - 1; ++i) { //ifaces creation
        for (int j = 0; j < ninc; ++j) {

          std::string name =
            "layer" + std::to_string(layer->layerno) +
            "_iface" + std::to_string(i) + std::to_string(j);

          double area;

          if (hslab->nlayers == 1) {
            area = (uarea - (i + 0.5)*(uarea - darea)/(nnodes - 1)) / ninc;
          }
          else if (layer->layerno == 0) {
            area = (uarea - (i + 0.5)*(uarea - darea)/(nnodes - 1 + 0.5)) / ninc;
          }
          else if (layer->layerno < hslab->nlayers - 1) {
            area = (uarea - (i + 1)*(uarea - darea)/nnodes) / ninc;
          }
          else {
            area = (uarea - (i + 1)*(uarea - darea)/(nnodes - 1 + 0.5)) / ninc;
          }

          auto f = std::make_shared<SFace>(
            name,
            layer->snodes[j + ninc*i],
            layer->snodes[j + ninc*(i + 1)],
            area
          );

          layer->ifaces.push_back(f);
        }
      }

      // ---- Layer interface faces ----
      if (layer->layerno > 0) {
        auto& pre = hslab->layers[layer->layerno - 1];
        for (int j = 0; j < ninc; ++j) {
          std::string name =
            "interface" + std::to_string(layer->layerno - 1) +
            "_iface" + std::to_string(j);

          double area = uarea / ninc;

          auto f = std::make_shared<SFace>(
            name, pre->snodes[pre->snodes.size() - ninc + j],
            layer->snodes[j], area);

          layer->ifaces.push_back(f);
        }
      }



      for (int i = 0; i < nnodes; ++i) { //jfaces creation
        for (int j = 0; j < ninc - 1; ++j) {
          std::string name =
            "layer" + std::to_string(layer->layerno) +
            "_jface" + std::to_string(i) + std::to_string(j);

          double area =
            0.5 * (layer->snodes[j + ninc*i]->Aj +
                  layer->snodes[j + ninc*i + 1]->Aj);

          auto f = std::make_shared<SFace>(
            name,
            layer->snodes[j + ninc*i],
            layer->snodes[j + ninc*i + 1],
            area);

          layer->jfaces.push_back(f);
        }
      }


      // ifaces attachment to nodes
      for (int i = 0; i < nnodes; ++i) {
        for (int j = 0; j < ninc; ++j) {

          auto& snode = layer->snodes[j + ninc*i];

          if (i == nnodes - 1) {  // downstream boundary
            snode->eface = nullptr;
            snode->wface = layer->ifaces[j + ninc*(i - 1)];
          }
          else if (i == 0) {      // upstream boundary
            snode->eface = layer->ifaces[j + ninc*i];
            snode->wface = nullptr;
          }
          else {                  // central nodes
            snode->eface = layer->ifaces[j + ninc*i];
            snode->wface = layer->ifaces[j + ninc*(i - 1)];
          }
        }
      }

      if (layer->layerno > 0) { //layer interface faces
        auto& pre = hslab->layers[layer->layerno - 1];
        for (int j = 0; j < ninc; ++j) {
          pre->snodes[pre->snodes.size() - ninc + j]->eface =
            layer->ifaces[j + ninc*(nnodes - 1)];
          layer->snodes[j]->wface =
            layer->ifaces[j + ninc*(nnodes - 1)];
        }
      }


      for (int i = 0; i < nnodes; ++i) { //jfaces attachment to nodes
        for (int j = 0; j < ninc; ++j) {
          auto& snode = layer->snodes[j + ninc*i];

          if (ninc == 1) {
            snode->nface = nullptr;
            snode->sface = nullptr;
          }
          else if (j == ninc - 1) {          // top boundary
            snode->nface = nullptr;
            snode->sface = layer->jfaces[j + (ninc - 1)*i - 1];
          }
          else if (j == 0) {                  // bottom boundary
            snode->nface = layer->jfaces[j + (ninc - 1)*i];
            snode->sface = nullptr;
          }
          else {                              // central nodes
            snode->nface = layer->jfaces[j + (ninc - 1)*i];
            snode->sface = layer->jfaces[j + (ninc - 1)*i - 1];
          }
        }
      }




	}
  }
}

void initialize_hslabs() {

  for (auto& hslab : model::hslabs) {
    for (auto &layer : hslab->layers) {
      for (auto& snode : layer->snodes) {
  		// bool trans_sim = settings::run_mode == RunMode::TRANSIENT;
  		// if (not trans_sim) {
  		  // node->assign_staticvar();
  		// }
  		snode->assign_prop();
  		// node->update_gues();
  	  }
  	  for (auto& sface : layer->ifaces) {
  		// face->assign_statevar();
  		// face->assign_prop();
  		// face->update_gues();
        sface->ther_gues->update();
  
  	  }
  	  for (auto& face : layer->jfaces) {

      }
    }
  }

}

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
template<typename T>
std::shared_ptr<T> find_in_vector(
  const std::vector<std::shared_ptr<T>>& vec,
  const std::string& obj)
{
  for (const auto& item : vec) {
    if (item->identifier == obj) {
      return item;
    }
  }
  return nullptr;
}

template<typename T>
std::shared_ptr<T> get_comp(const std::string& obj)
{
  for (const auto& circuit : model::circuits) {
    if (auto item = find_in_vector(circuit->pipes, obj))
      return item;
    // same for nodes, bcs, pumps, ...
  }

  std::cerr << "Object not found in project. Stopping: " << obj << std::endl;
  std::exit(EXIT_FAILURE);
}

} // namespace opensd
