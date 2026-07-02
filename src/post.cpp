//! \file post.cpp

#include "opensd/post.h"
#include "opensd/circuit.h"
#include "opensd/file_utils.h"
#include "opensd/settings.h"

#include <deque>
#include <iostream>
#include <unordered_map>

namespace opensd {

namespace py = pybind11;

std::ofstream f1;
std::vector<std::unique_ptr<Calculate>> Calculate::registry;
const std::vector<std::string> node_items = {"tpres_gues", "spres_gues", "ttemp_gues", "tenth_gues", "rhomass", "msource", "esource"};
const std::vector<std::string> pipe_node_items = {"tpres_gues", "spres_gues", "ttemp_gues", "tenth_gues", "rhomass"};
const std::vector<std::string> pipe_items = {"vflow", "velocity", "mflow"};
const std::vector<std::string> face_items = {"vflow", "velocity", "mflow"};
bool output_has_header = false;

// Map to access Node attributes by string
std::unordered_map<std::string, std::function<double(const Node&)>> nodeAttributeMap = {
  {"tpres_gues", &Node::tpres_gues},
  {"spres_gues", &Node::spres_gues},
  {"ttemp_gues", &Node::ttemp_gues},
  {"tenth_gues", &Node::tenth_gues},
  {"msource", &Node::msource},
  {"esource", &Node::esource},
  // {"volume", &Node::volume},
  // {"velocity", &Node::velocity},
  // Add other mappings as needed
};

double mean_pipe_face_value(const Pipe& pipe, const std::function<double(const PFace&)>& getter)
{
  if (pipe.faces.empty()) return 0.0;

  double total = 0.0;
  for (const auto& face : pipe.faces) {
    if (face) total += getter(*face);
  }
  return total / pipe.faces.size();
}

// Map to access pipe-level output attributes by string
std::unordered_map<std::string, std::function<double(const Pipe&)>> pipeAttributeMap = {
  {"vflow", [](const Pipe& pipe) {
    return mean_pipe_face_value(pipe, [](const PFace& face) { return face.vflow_gues; });
  }},
  {"velocity", [](const Pipe& pipe) {
    return mean_pipe_face_value(pipe, [](const PFace& face) { return face.velocity; });
  }},
  {"mflow", &Pipe::mflow}
};

std::unordered_map<std::string, std::function<double(const Face&)>> faceAttributeMap = {
  {"vflow", &Face::vflow_gues},
  {"velocity", &Face::velocity},
  {"mflow", &Face::mflow}
};

double nodeAttributeValue(const Node& node, const std::string& item)
{
  if (item == "rhomass") return node.ther_gues ? node.ther_gues->rhomass() : 0.0;

  auto it = nodeAttributeMap.find(item);
  return it != nodeAttributeMap.end() ? it->second(node) : 0.0;
}

std::string faceIdentifier(const Face& face)
{
  if (const auto* pface = dynamic_cast<const PFace*>(&face)) {
    return pface->pipe ? pface->pipe->identifier + "_face" + std::to_string(face.faceno) : "face" + std::to_string(face.faceno);
  }
  return "face" + std::to_string(face.faceno);
}

void writeCell(const std::string& value)
{
  f1 << "," << value;
}

void writeCell(double value)
{
  f1 << "," << std::setprecision(7) << value;
}

std::vector<std::string> readLastNonEmptyRows(const std::string& path, std::size_t row_count)
{
  std::ifstream fin(path);
  std::deque<std::string> rows;
  std::string line;

  while (std::getline(fin, line)) {
    if (line.empty()) continue;
    rows.push_back(line);
    if (rows.size() > row_count) rows.pop_front();
  }

  return {rows.begin(), rows.end()};
}

void openFile(const std::string& outputFile) {
  (void)outputFile;
  std::string fixedOutputFile = "output.res";
  std::string bPath = std::string(getenv("PWD")) + "/" + fixedOutputFile;

  std::vector<std::string> retained_rows;
  if (settings::run_mode == RunMode::TRANSIENT) {
    retained_rows = readLastNonEmptyRows(bPath, 2);
  }

  f1.open(bPath, std::ios::out | std::ios::trunc);
  output_has_header = false;

  if (retained_rows.size() == 2 && retained_rows.front().find("time") != std::string::npos) {
    for (const auto& row : retained_rows) {
      f1 << row << '\n';
    }
    output_has_header = true;
  }
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
  if (time == 0. && !output_has_header) {
    writeHeader();
  }
  writeValue(time, delt);
}

void writeHeader() {
  f1 << " time(s)";

  for (const auto& circuit : model::circuits_owned) {
    for (const auto& ger : circuit->gers) {
      writeCell("vflow_gues:" + ger->identifier);
    }
    for (const auto& item : pipe_items) {
      for (const auto& pipe : circuit->pipes) {
        writeCell(item + ":" + pipe->identifier);
      }
    }
    for (const auto& item : face_items) {
      for (const auto& face : circuit->faces) {
        writeCell(item + ":" + faceIdentifier(*face));
      }
    }
    for (const auto& item : pipe_node_items) {
      for (const auto& pipe : circuit->pipes) {
        writeCell(item + ":" + pipe->identifier + "_upstream");
        writeCell(item + ":" + pipe->identifier + "_downstream");
      }
    }
    for (const auto& item : node_items) {
      for (const auto& node : circuit->nodes) {
        writeCell(item + ":" + node->identifier);
      }
    }
  }
  for (const auto& calc : Calculate::registry) {
    writeCell(calc->identifier());
  }
  f1 << '\n';
  output_has_header = true;
}

void writeValue(double time, double delt) {
  f1 << std::setw(10) << std::setprecision(4) << time;

  for (const auto& circuit : model::circuits_owned) {
    for (const auto& ger : circuit->gers) {
      writeCell(ger->vflow_gues * ger->ther_gues->rhomass());
    }
    for (const auto& item : pipe_items) {
      auto it = pipeAttributeMap.find(item);
      for (const auto& pipe : circuit->pipes) {
        writeCell(it != pipeAttributeMap.end() ? it->second(*pipe) : 0.0);
      }
    }
    for (const auto& item : face_items) {
      auto it = faceAttributeMap.find(item);
      for (const auto& face : circuit->faces) {
        writeCell(it != faceAttributeMap.end() ? it->second(*face) : 0.0);
      }
    }
    for (const auto& item : pipe_node_items) {
      for (const auto& pipe : circuit->pipes) {
        writeCell(pipe->unode ? nodeAttributeValue(*pipe->unode, item) : 0.0);
        writeCell(pipe->dnode ? nodeAttributeValue(*pipe->dnode, item) : 0.0);
      }
    }
    for (const auto& item : node_items) {
      for (const auto& node : circuit->nodes) {
        writeCell(nodeAttributeValue(*node, item));
      }
    }
  }
  for (const auto& calc : Calculate::registry) {
    writeCell(calc->val);
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
