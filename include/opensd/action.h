#ifndef OPENSD_ACTION_H
#define OPENSD_ACTION_H

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <pugixml.hpp>
#include "opensd/circuit.h"
#include "opensd/bc.h"
#include "opensd/node.h"

namespace opensd {

//==============================================================================
// Forward declarations
//==============================================================================

class Action;

//==============================================================================
// Global variables
//==============================================================================

namespace model {
extern std::vector<std::unique_ptr<Action>> actions;
}  // namespace model

//==============================================================================
//! Class representing a user-defined action (e.g., ramp, step, constant input)
//==============================================================================

class Action {
public:
  struct Point {
    double time;
    double value;
  };

  // Constructor
  explicit Action(pugi::xml_node node);

  // Update function called each timestep
  void update(double t, double dt) const;

  // Accessors
  const std::string& identifier() const { return identifier_; }
  const std::string& target() const { return target_; }
  const std::string& variable() const { return variable_; }
  const std::string& interp() const { return interp_; }

private:
  // Data
  std::string identifier_;
  std::string target_;
  std::string variable_;
  std::string interp_;
  std::vector<Point> points_;

  // Resolved target object
  Node* obj_ {nullptr};
  std::function<void(Node*, double)> setter_;

  // Internal helpers
  double value_at(double t) const;
  void link_target();  // resolves target_ → obj_ + setter_
};

//==============================================================================
// XML readers
//==============================================================================

void read_actions_xml();
void read_actions_xml(pugi::xml_node root);

}  // namespace opensd

#endif  // OPENSD_ACTION_H
