#ifndef OPENSD_ACTION_H
#define OPENSD_ACTION_H

#include <string>
#include <memory>
#include <vector>
#include <pugixml.hpp>

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

  // Methods
  double value_at(double t) const;
  void update(double t) const;

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
};

//==============================================================================
// XML readers
//==============================================================================

void read_actions_xml();
void read_actions_xml(pugi::xml_node root);

}  // namespace opensd

#endif  // OPENSD_ACTION_H
