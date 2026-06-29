#ifndef OPENSD_ACTION_H
#define OPENSD_ACTION_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <pugixml.hpp>
#include <pybind11/pybind11.h>

#include "opensd/bc.h"
#include "opensd/circuit.h"
#include "opensd/node.h"

namespace py = pybind11;

namespace opensd {

class Action;

namespace model {
extern std::vector<std::unique_ptr<Action>> actions;
}  // namespace model

class Action {
public:
  struct Point {
    double time;
    double value;
  };

  explicit Action(pugi::xml_node node);

  void update(double t, double dt) const;

  const std::string& identifier() const { return identifier_; }
  const std::string& target() const { return target_; }
  const std::string& variable() const { return variable_; }
  const std::string& interp() const { return interp_; }

private:
  enum class DistributionType {
    TABULAR,
    FUNCTION
  };

  struct TargetBinding {
    std::string target;
    std::string variable;
    std::function<void(double)> setter;
  };

  std::string identifier_;
  std::string target_;
  std::string variable_;
  std::string interp_;
  DistributionType distribution_type_ {DistributionType::TABULAR};
  std::vector<Point> points_;
  std::vector<TargetBinding> targets_;
  py::object py_callable_;

  double value_at(double t) const;
  TargetBinding make_target_binding(const std::string& target,
                                    const std::string& variable,
                                    bool required) const;
  void apply_function_result(const py::object& result) const;
};

void read_actions_xml();
void read_actions_xml(pugi::xml_node root);

}  // namespace opensd

#endif  // OPENSD_ACTION_H
