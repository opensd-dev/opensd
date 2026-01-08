//! \file custom_fun.h

#ifndef OPENSD_CUSTOM_FUN_H
#define OPENSD_CUSTOM_FUN_H

#include "opensd/constants.h"
#include "opensd/face.h"
#include "opensd/snode.h"
#include <pybind11/pybind11.h>
namespace py = pybind11;
namespace opensd {

//==============================================================================
// Global variables
//==============================================================================

struct Input {

  InputType type{InputType::CONSTANT};

  double constant_value{0.0};   // valid if CONSTANT
  py::object py_callable;       // valid if FUNCTION

  // Convenience constructors (strongly recommended)
  static Input constant(double val) {
    Input i;
    i.type = InputType::CONSTANT;
    i.constant_value = val;
    return i;
  }

  static Input function(py::object func) {
    Input i;
    i.type = InputType::FUNCTION;
    i.py_callable = func;
    return i;
  }
};

//==============================================================================
// Non-member functions
//==============================================================================

double eval(const Input& input,
            std::shared_ptr<Face> flow_elem=nullptr,
            SNode* wall_node=nullptr);


} // namespace opensd

#endif // OPENSD_CUSTOM_FUN_H
