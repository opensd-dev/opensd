//! \file hslab.cpp

#include "opensd/hdf5_interface.h"

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <numeric>

#include "opensd/hslab.h"
#include "opensd/circuit.h"
// #include "opensd/snode.h"
// #include "opensd/layer.h"
// #include "opensd/bc.h"
#include "opensd/error.h"
// #include "opensd/vector.h"
#include "opensd/xml_interface.h"
// #include "opensd/constants.h"
#include "opensd/custom_fun.h"

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

  // Read the solid layers. The Python input uses nlayers as a discretization
  // mode flag: nlayers=1 is distinct from the default one-layer slab.
  int layer_count = 0;
  for (pugi::xml_node layer_node : hslab_node.children("layer")) {
    this->layers.push_back(std::make_shared<Layer>(layer_node));
 // this->layers.back()->layer_ind = this->layers.size() - 1;
    this->layers.back()->hslab = this;
    layer_count++;
  }
  nlayers = hslab_node.attribute("nlayers")
              ? hslab_node.attribute("nlayers").as_int()
              : layer_count;
  uvar = get_node_value(hslab_node, "uvar");
  dvar = get_node_value(hslab_node, "dvar");
  ucompid = get_node_value(hslab_node, "ucomp");
  dcompid = get_node_value(hslab_node, "dcomp");
  config = hslab_node.attribute("config").as_string("");


  std::string type = get_node_value(hslab_node, "utype");

  if (type == "constant") {
    uval.type = InputType::CONSTANT;
    uval.constant_value = stod(get_node_value(hslab_node, "uval"));

  } else if (type == "function") {
    uval.type = InputType::FUNCTION;
    std::string fname = get_node_value(hslab_node, "uval");

    py::gil_scoped_acquire gil;
    py::module sys = py::module::import("sys");

    // get executable path
    std::filesystem::path exe = std::filesystem::canonical("/proc/self/exe");
    std::filesystem::path exe_dir = exe.parent_path();

    // add that directory
    sys.attr("path").attr("insert")(0, exe_dir.string());

    py::module scripts = py::module::import("scripts");
    py::module bindings = py::module::import("bindings");
    std::cout << "[HSlab] scripts module loaded from "
              << py::str(scripts.attr("__file__")).cast<std::string>()
              << std::endl;

    py::object f = scripts.attr(fname.c_str());

    if (!PyCallable_Check(f.ptr())) {
      fatal_error("uval function '" + fname + "' is not callable");
    }
    std::cout << "[HSlab] uval read successfully (function): "
              << fname << std::endl;

    uval.py_callable = f;

  } else {
    fatal_error("Unknown utype: " + type);
  }


  type = get_node_value(hslab_node, "dtype");

  if (type == "constant") {
    dval.type = InputType::CONSTANT;
    dval.constant_value = stod(get_node_value(hslab_node, "dval"));

  } else if (type == "function") {
    dval.type = InputType::FUNCTION;
    std::string fname = get_node_value(hslab_node, "dval");

    py::gil_scoped_acquire gil;
    py::module sys = py::module::import("sys");

    sys.attr("path").attr("insert")(0, ".");

    py::module scripts = py::module::import("scripts");
    std::cout << "[HSlab] scripts module loaded from "
              << py::str(scripts.attr("__file__")).cast<std::string>()
              << std::endl;

    py::object f = scripts.attr(fname.c_str());

    if (!PyCallable_Check(f.ptr())) {
      fatal_error("dval function '" + fname + "' is not callable");
    }
    std::cout << "[HSlab] dval read successfully (function): "
              << fname << std::endl;

    dval.py_callable = f;

  if (!PyCallable_Check(f.ptr())) {
    fatal_error("dval function '" + fname + "' is not callable");
  }

  dval.py_callable = f;

  } else {
    fatal_error("Unknown utype: " + type);
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

  if (hslab->uvar == "pipe"){
    hslab->upipe = get_comp<Pipe>(hslab->ucompid);

  if (hslab->upipe) {
    std::cout << "Found pipe with identifier: " << hslab->upipe->identifier << std::endl;
  } else {
    std::cout << "Upstream Pipe not found." << std::endl;
  }

  hslab->uval1.assign(hslab->upipe->faces.begin(),
             hslab->upipe->faces.begin() + hslab->upipe->ncell);
  }

  if (hslab->dvar == "pipe"){
  hslab->dpipe = get_comp<Pipe>(hslab->dcompid);

  if (hslab->dpipe) {
    std::cout << "Found pipe with identifier: " << hslab->dpipe->identifier << std::endl;
  } else {
    std::cout << "Downstream Pipe not found." << std::endl;
  }

  if (hslab->config == "counter") {
    hslab->dval1.assign(hslab->dpipe->faces.rbegin(),
                hslab->dpipe->faces.rbegin() + hslab->dpipe->ncell);
  } else {
    hslab->dval1.assign(hslab->dpipe->faces.begin(),
                 hslab->dpipe->faces.begin() + hslab->dpipe->ncell);
  }
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

      std::vector<double> AFF = layer->AFF;
      if (AFF.empty()) {
        AFF.assign(ninc, 1.0 / ninc);
      } else if (static_cast<int>(AFF.size()) != ninc) {
        fatal_error("Layer AFF length does not match hslab ninc: " + hslab->identifier);
      }

      layer->snodes.clear();
      hslab->dwnodes.clear();
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
		  snode->heat_input = heat_frac*layer->heat_input;
		  snode->heat_input_old = heat_frac*layer->heat_input;

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
    }

    std::vector<double> reference_temps;
    for (size_t i = 0; i < hslab->uwnodes.size(); ++i) {
      auto& snode = hslab->uwnodes[i];
      if ((hslab->uvar == "pipe" || hslab->uvar == "pipenl") && i < hslab->uval1.size()) {
        snode->temp_old = hslab->uval1[i]->stemp_gues;
      } else {
        snode->temp_old = settings::T_ambient;
      }
      reference_temps.push_back(snode->temp_old);
    }

    for (size_t i = 0; i < hslab->dwnodes.size(); ++i) {
      auto& snode = hslab->dwnodes[i];
      if ((hslab->dvar == "pipe" || hslab->dvar == "pipenl") && i < hslab->dval1.size()) {
        snode->temp_old = hslab->dval1[i]->stemp_gues;
      } else {
        snode->temp_old = settings::T_ambient;
      }
      reference_temps.push_back(snode->temp_old);
    }

    double tref = settings::T_ambient;
    if (!reference_temps.empty()) {
      tref = std::accumulate(reference_temps.begin(), reference_temps.end(), 0.0)
             / reference_temps.size();
    }

    for (auto& layer : hslab->layers) {
      for (auto& snode : layer->snodes) {
        if (snode->temp_old == 0.0) {
          snode->temp_old = tref;
        }
        snode->temp_gues = snode->temp_old;
        snode->heat_transfer = 0.0;
        snode->heat_transfer_old = 0.0;
        snode->ther_old->update(snode->temp_old);
        snode->ther_gues->update(snode->temp_gues);
      }
      for (auto& sface : layer->ifaces) {
        sface->ther_old->update_old();
        sface->ther_gues->update();
        sface->update_temp();
      }
      for (auto& face : layer->jfaces) {
        face->ther_old->update_old();
        face->ther_gues->update();
        face->update_temp();
      }
    }
  }

}

void HSlab::save_to_hdf5(hid_t group_id) const {
  write_string(group_id, "identifier", identifier);
  write_scalar(group_id, "mean_ht", mean_ht);
  write_scalar(group_id, "uheat_transfer", uheat_transfer);
  write_scalar(group_id, "dheat_transfer", dheat_transfer);

  hid_t layers_group = H5Gcreate(group_id, "layers", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
  for (size_t i = 0; i < layers.size(); ++i) {
    std::string layer_name = "layer_" + std::to_string(i);
    hid_t layer_group = H5Gcreate(layers_group, layer_name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    write_scalar(layer_group, "layerno", layers[i]->layerno);

    hid_t snodes_group = H5Gcreate(layer_group, "snodes", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    for (size_t j = 0; j < layers[i]->snodes.size(); ++j) {
      std::string snode_name = "snode_" + std::to_string(j);
      hid_t snode_group = H5Gcreate(snodes_group, snode_name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
      auto& snode = layers[i]->snodes[j];
      write_string(snode_group, "identifier", snode->identifier);
      write_scalar(snode_group, "temp_gues", snode->temp_gues);
      write_scalar(snode_group, "temp_old", snode->temp_old);
      write_scalar(snode_group, "heat_transfer", snode->heat_transfer);
      write_scalar(snode_group, "heat_transfer_old", snode->heat_transfer_old);
      write_scalar(snode_group, "heat_input", snode->heat_input);
      write_scalar(snode_group, "heat_input_old", snode->heat_input_old);
      H5Gclose(snode_group);
    }
    H5Gclose(snodes_group);
    H5Gclose(layer_group);
  }
  H5Gclose(layers_group);
}

void HSlab::load_from_hdf5(hid_t group_id) {
  if (H5Aexists(group_id, "mean_ht") > 0) mean_ht = read_scalar(group_id, "mean_ht");
  if (H5Aexists(group_id, "uheat_transfer") > 0) uheat_transfer = read_scalar(group_id, "uheat_transfer");
  if (H5Aexists(group_id, "dheat_transfer") > 0) dheat_transfer = read_scalar(group_id, "dheat_transfer");
  if (H5Lexists(group_id, "layers", H5P_DEFAULT) <= 0) return;

  hid_t layers_group = H5Gopen(group_id, "layers", H5P_DEFAULT);
  for (size_t i = 0; i < layers.size(); ++i) {
    std::string layer_name = "layer_" + std::to_string(i);
    if (H5Lexists(layers_group, layer_name.c_str(), H5P_DEFAULT) <= 0) continue;

    hid_t layer_group = H5Gopen(layers_group, layer_name.c_str(), H5P_DEFAULT);
    if (H5Lexists(layer_group, "snodes", H5P_DEFAULT) <= 0) {
      H5Gclose(layer_group);
      continue;
    }

    hid_t snodes_group = H5Gopen(layer_group, "snodes", H5P_DEFAULT);
    for (size_t j = 0; j < layers[i]->snodes.size(); ++j) {
      std::string snode_name = "snode_" + std::to_string(j);
      if (H5Lexists(snodes_group, snode_name.c_str(), H5P_DEFAULT) <= 0) continue;

      hid_t snode_group = H5Gopen(snodes_group, snode_name.c_str(), H5P_DEFAULT);
      auto& snode = layers[i]->snodes[j];
      snode->temp_gues = read_scalar(snode_group, "temp_gues");
      snode->temp_old = read_scalar(snode_group, "temp_old");
      snode->heat_transfer = read_scalar(snode_group, "heat_transfer");
      snode->heat_transfer_old = read_scalar(snode_group, "heat_transfer_old");
      snode->heat_input = read_scalar(snode_group, "heat_input");
      snode->heat_input_old = read_scalar(snode_group, "heat_input_old");
      if (snode->ther_old) snode->ther_old->update(snode->temp_old);
      if (snode->ther_gues) snode->ther_gues->update(snode->temp_gues);
      H5Gclose(snode_group);
    }
    H5Gclose(snodes_group);
    H5Gclose(layer_group);
  }
  H5Gclose(layers_group);
}

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
