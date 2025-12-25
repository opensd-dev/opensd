//! \file custom_fun.cpp

#include "opensd/custom_fun.h"

namespace opensd {

double eval(const Input& input,
            std::shared_ptr<Face> flow_elem,
            SNode* wall_node)
{
  switch (input.type) {

    case InputType::CONSTANT:
      return input.constant_value;

    case InputType::FUNCTION: {
      py::gil_scoped_acquire gil;

        // py::object ret =
        //   input.py_callable(flow_elem, wall_node);
        py::object ret =
          input.py_callable(flow_elem->velocity);
        // std::cout<<"flag1 "<<flow_elem->velocity<<std::endl;
        // std::exit(0);

      return ret.cast<double>();
    }

    default:
      throw std::runtime_error("Unsupported InputType in eval()");
  }
}

}
