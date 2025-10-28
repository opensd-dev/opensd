#include "opensd/action.h"

#include <iostream>
#include <stdexcept>
#include "opensd/file_utils.h"
#include "opensd/settings.h"

namespace opensd {
namespace model {

std::vector<std::unique_ptr<Action>> actions;

}  // namespace model

//==============================================================================
// Action implementation
//==============================================================================

Action::Action(pugi::xml_node node)
{
  if (auto attr = node.attribute("identifier"))
    identifier_ = attr.as_string();
  else
    throw std::runtime_error("Each <action> must have an 'identifier' attribute.");

  if (auto attr = node.attribute("target"))
    target_ = attr.as_string();

  if (auto attr = node.attribute("variable"))
    variable_ = attr.as_string();

  // Read tabular block
  if (auto tab = node.child("tabular")) {
    interp_ = tab.attribute("interp").as_string("linear");
    for (auto pt : tab.children("point")) {
      Point p;
      p.time = pt.attribute("time").as_double();
      p.value = pt.attribute("value").as_double();
      points_.push_back(p);
    }
  }
}

//==============================================================================
// Value interpolation
//==============================================================================

double Action::value_at(double t) const
{
  if (points_.empty()) return 0.0;

  if (t <= points_.front().time) return points_.front().value;
  if (t >= points_.back().time) return points_.back().value;

  for (size_t i = 0; i < points_.size() - 1; ++i) {
    if (t >= points_[i].time && t <= points_[i + 1].time) {
      double t0 = points_[i].time, v0 = points_[i].value;
      double t1 = points_[i + 1].time, v1 = points_[i + 1].value;
      double f = (t - t0) / (t1 - t0);
      return v0 + f * (v1 - v0);
    }
  }
  return points_.back().value;
}

//==============================================================================
// Update function
//==============================================================================

void Action::update(double t) const
{
  double val = value_at(t);
  std::cout << "[Action] " << identifier_
            << " → target=" << target_
            << ", variable=" << variable_
            << ", value=" << val << std::endl;

  // TODO: Link to actual OpenSD object updates
  // Example:
  // if (variable_ == "pressure") model::nodes[target_index]->pressure() = val;
}

//==============================================================================
// XML readers
//==============================================================================

void read_actions_xml()
{
  std::string filename = settings::path_input + "actions.xml";
  if (!file_exists(filename)) return;

  std::cout << "Reading actions.xml..." << std::endl;

  pugi::xml_document doc;
  if (!doc.load_file(filename.c_str()))
    throw std::runtime_error("Error: cannot open actions.xml");

  pugi::xml_node root = doc.child("actions");
  read_actions_xml(root);
}

void read_actions_xml(pugi::xml_node root)
{
  for (pugi::xml_node act_node : root.children("action")) {
    model::actions.push_back(std::make_unique<Action>(act_node));
  }
  model::actions.shrink_to_fit();
}

}  // namespace opensd
