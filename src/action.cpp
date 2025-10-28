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
    identifier_ = node.attribute("identifier").as_string();
    target_     = node.attribute("target").as_string();
    variable_   = node.attribute("variable").as_string();

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
    // Pre-resolve target object
    link_target();
}

//==============================================================================
// Compute value at given time using linear interpolation
//==============================================================================

double Action::value_at(double t) const
{
    if (points_.empty()) return 0.0;
    if (t <= points_.front().time) return points_.front().value;
    if (t >= points_.back().time) return points_.back().value;

    for (size_t i = 0; i < points_.size() - 1; ++i) {
        if (t >= points_[i].time && t <= points_[i+1].time) {
            double f = (t - points_[i].time) / (points_[i+1].time - points_[i].time);
            return points_[i].value + f * (points_[i+1].value - points_[i].value);
        }
    }

    return points_.back().value;
}

//==============================================================================
// Pre-resolve BC / Node pointer
//==============================================================================

void Action::link_target()
{
    for (auto& circuit : model::circuits) {
        for (auto& bc : circuit->bcs) {
            if (target_ == bc.identifier) {
                Node* node_ptr = circuit->get_node_by_identifier(bc.node_);
                if (!node_ptr)
                    throw std::runtime_error("Node not found for BC: " + bc.node_);

                obj_ = node_ptr;

                if (variable_ == "bval") {
                    if (bc.var_ == "P") {
                        setter_ = [](Node* n, double val){ n->tpres_gues = val; };
                    } else if (bc.var_ == "T") {
                        setter_ = [](Node* n, double val){ n->ttemp_gues = val; };
                    } else if (bc.var_ == "msource") {
                        setter_ = [](Node* n, double val){ n->msource = val; };
                    } else {
                        throw std::runtime_error("Unknown BC variable in Action: " + bc.var_);
                    }
                }

                return; // target found, exit
            }
        }
    }

    throw std::runtime_error("Action target not found: " + target_);
}

//==============================================================================
// Update function
//==============================================================================

void Action::update(double t, double dt) const
{
    if (!obj_ || !setter_) return;

    double val = value_at(t);
    setter_(obj_, val);
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

    read_actions_xml(doc.child("actions"));
}

void read_actions_xml(pugi::xml_node root)
{
    for (auto act_node : root.children("action")) {
        model::actions.push_back(std::make_unique<Action>(act_node));
    }
    model::actions.shrink_to_fit();
}

} // namespace opensd
