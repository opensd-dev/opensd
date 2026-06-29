//! \file post.h

#ifndef POST_H
#define POST_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <memory>
#include <iomanip>
#include <algorithm>
#include <functional>
#include <pugixml.hpp>
#include <pybind11/pybind11.h>

namespace opensd {

class Calculate {
public:
  static std::vector<std::unique_ptr<Calculate>> registry;

  explicit Calculate(pugi::xml_node node);
  void update(double time, double delt);

  const std::string& identifier() const { return identifier_; }

private:
  std::string identifier_;
  std::string function_name_;
  std::vector<std::string> component_ids_;
  pybind11::object callable_;

public:
  std::vector<double> timeSeries;
  double val {0.0};
};

void openFile(const std::string& outputFile);
void writeOutput(double time, double delt);
void writeHeader();
void writeValue(double time, double delt);
void updateCalcs(double time, double delt);
void read_post_xml();
void read_post_xml(pugi::xml_node root);

}

#endif // POST_H
