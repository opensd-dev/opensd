//! \file post.cpp

#include "opensd/post.h"
#include "opensd/circuit.h"
#include "opensd/file_utils.h"
#include "opensd/settings.h"

#include <iostream>

namespace opensd {

namespace py = pybind11;

std::ofstream f1;
std::vector<std::unique_ptr<Calculate>> Calculate::registry;

// Map to access Node attributes by string
std::unordered_map<std::string, std::function<double(const Node&)>> nodeAttributeMap = {
  {"tpres_gues", &Node::tpres_gues},
  {"spres_gues", &Node::spres_gues},
  {"ttemp_gues", &Node::ttemp_gues},
  {"tenth_gues", &Node::tenth_gues},
  // {"msource", &Node::msource},
  // {"esource", &Node::esource},
  // {"volume", &Node::volume},
  // {"velocity", &Node::velocity},
  // Add other mappings as needed
};

void openFile(const std::string& outputFile) {
  std::string fixedOutputFile = "output.res";
  std::string bPath = std::string(getenv("PWD")) + "/" + fixedOutputFile;
  f1.open(bPath, std::ios::out | std::ios::app);  // append mode
}


Calculate::Calculate(pugi::xml_node node)
{
  identifier_ = node.attribute("identifier").as_string();
  function_name_ = node.attribute("function").as_string();

  for (auto comp : node.children("component")) {
    component_ids_.push_back(comp.attribute("id").as_string());
  }

  py::gil_scoped_acquire gil;
  py::module sys = py::module::import("sys");
  sys.attr("path").attr("insert")(0, ".");
  py::module scripts = py::module::import("scripts");
  callable_ = scripts.attr(function_name_.c_str());
  if (!PyCallable_Check(callable_.ptr())) {
    throw std::runtime_error("Post calculation function '" + function_name_ + "' is not callable");
  }
}

void Calculate::update(double time, double delt)
{
  py::gil_scoped_acquire gil;
  py::module scripts = py::module::import("scripts");
  scripts.attr("time") = time;
  scripts.attr("delt") = delt;

  py::object result;
  if (component_ids_.empty()) {
    result = callable_();
  } else {
    py::module bindings = py::module::import("bindings");
    py::tuple args(component_ids_.size());
    for (size_t i = 0; i < component_ids_.size(); ++i) {
      args[i] = bindings.attr("get_comp")(component_ids_[i]);
    }
    result = callable_(*args);
  }

  if (!result.is_none()) {
    val = result.cast<double>();
    timeSeries.push_back(val);
  }
}

void writeOutput(double time, double delt) {
  if (time == 0.) {
    writeHeader();
  }
  writeValue(time, delt);
}

void writeHeader() {
  f1 << " time(s)";
  
  // Placeholder for the items in the original code
  // std::vector<std::string> pipe_items = {"mflow"};
  std::vector<std::vector<std::string>> node_items = {{"tpres_gues"}, {"spres_gues"}, {"ttemp_gues"}, {"tenth_gues"}, {"ther_gues", "rhomass"}, {"msource"}, {"esource"} };
  
  for (const auto& circuit : model::circuits_owned) {
    for (const auto& ger : circuit->gers) {
      // for (const auto& item : ger_items) {
        f1 << "," << "vflow_gues" << ":" << ger->identifier;
      // }
    }
    for (const auto& item : node_items) {
      for (const auto& node : circuit->nodes) {
        if (item.size() == 1) {
          f1 << "," << item[0] << ":" << node->identifier;
        } else if (item.size() == 2) {
          f1 << "," << item[1] << ":" << node->identifier;
        }
      }
    }
  }
  f1 << '\n';
}

void writeValue(double time, double delt) {
  f1 << std::setw(10) << std::setprecision(4) << time << ",";

  // Placeholder for the items in the original code
  std::vector<std::string> pipe_items = {"mflow"};
  std::vector<std::vector<std::string>> node_items = {{"tpres_gues"}, {"spres_gues"}, {"ttemp_gues"}, {"tenth_gues"}, {"ther_gues", "rhomass"}, {"msource"}, {"esource"} };

  for (const auto& circuit : model::circuits_owned) {
    for (const auto& ger : circuit->gers) {
      // for (const auto& item : pipe_items) {
        f1 << "," << std::setw(7) << std::setprecision(7) << ger->vflow_gues*ger->ther_gues->rhomass(); // Replace 0.0 with the actual value
      // }
    }
    for (const auto& item : node_items) {
      for (const auto& node : circuit->nodes) {
        if (item.size() == 1) {
          auto it = nodeAttributeMap.find(item[0]);
          if (it != nodeAttributeMap.end()) {
            double value = it->second(*node);
            f1 << std::setprecision(7) << value << ",";
          }
        } else if (item.size() == 2) {
          f1 << "," << std::setw(7) << std::setprecision(7) << 0.0; // Replace 0.0 with the actual value
        }
      }
    }
  }
  f1 << '\n';
  f1.flush();
}

void updateCalcs(double time, double delt) {
  for (auto& calc : Calculate::registry) {
    calc->update(time, delt);
  }
}

void read_post_xml()
{
  std::string filename = settings::path_input + "post.xml";
  if (!file_exists(filename)) {
    return;
  }

  pugi::xml_document doc;
  if (!doc.load_file(filename.c_str())) {
    throw std::runtime_error("Error: cannot open post.xml");
  }

  read_post_xml(doc.child("post"));
}

void read_post_xml(pugi::xml_node root)
{
  Calculate::registry.clear();
  for (auto calc_node : root.children("calculate")) {
    Calculate::registry.push_back(std::make_unique<Calculate>(calc_node));
  }
}

} // namespace opensd
