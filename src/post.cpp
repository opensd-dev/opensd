//! \file post.cpp

#include "opensd/post.h"
#include "opensd/circuit.h"

namespace opensd {

std::ofstream f1;
std::vector<Calculate*> Calculate::registry;

// Map to access Node attributes by string
std::unordered_map<std::string, std::function<double(const Node&)>> nodeAttributeMap = {
  {"tpres_gues", &Node::tpres_gues},
  {"spres_gues", &Node::spres_gues},
  {"ttemp_gues", &Node::ttemp_gues},
  {"tenth_gues", &Node::tenth_gues},
  {"msource", &Node::msource},
  {"esource", &Node::esource},
  {"volume", &Node::volume},
  {"velocity", &Node::velocity},
  // Add other mappings as needed
};

void openFile(const std::string& outputFile) {
  std::string fixedOutputFile = "output.res";
  std::string bPath = std::string(getenv("PWD")) + "/" + fixedOutputFile;
  f1.open(bPath, std::ios::out | std::ios::app);  // append mode
}


Calculate::Calculate(double(*func)(const std::vector<double>&), const std::vector<std::string>& comps) {
  // Assuming you have some initialization here
  registry.push_back(this);
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
    // for (const auto& pipe : circuit.pipes) {
      // for (const auto& item : pipe_items) {
        // f1 << " " << item << ":" << pipe.identifier;
      // }
    // }
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
    // for (const auto& pipe : circuit.pipes) {
      // for (const auto& item : pipe_items) {
        // f1 << " " << std::setw(7) << std::setprecision(7) << 0.0; // Replace 0.0 with the actual value
      // }
    // }
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

// void updateCalcs(double time, double delt) {
  // for (auto& calc : Calculate::registry) {
    // calc->timeSeries.push_back(time);
    // calc->timeSeries.push_back(calc->val);
  // }
// }

} // namespace opensd
