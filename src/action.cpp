#include "opensd/action.h"

#include <iostream>
#include <stdexcept>

#include "opensd/file_utils.h"
#include "opensd/hslab.h"
#include "opensd/settings.h"

namespace opensd {
namespace model {

std::vector<std::unique_ptr<Action>> actions;

}  // namespace model

Action::Action(pugi::xml_node node)
{
  identifier_ = node.attribute("identifier").as_string();

  if (auto tab = node.child("tabular")) {
    distribution_type_ = DistributionType::TABULAR;
    interp_ = tab.attribute("interp").as_string("linear");

    target_ = node.attribute("target").as_string();
    variable_ = node.attribute("variable").as_string();
    if (target_.empty()) {
      target_ = node.child("target").attribute("id").as_string();
      variable_ = node.child("target").attribute("variable").as_string();
    }

    for (auto pt : tab.children("point")) {
      Point p;
      p.time = pt.attribute("time").as_double();
      p.value = pt.attribute("value").as_double();
      points_.push_back(p);
    }
  } else if (auto fn = node.child("function")) {
    distribution_type_ = DistributionType::FUNCTION;
    interp_ = "function";
    std::string fname = fn.attribute("name").as_string();

    {
      py::gil_scoped_acquire gil;
      py::module sys = py::module::import("sys");
      sys.attr("path").attr("insert")(0, ".");
      py::module scripts = py::module::import("scripts");
      py_callable_ = scripts.attr(fname.c_str());
      if (!PyCallable_Check(py_callable_.ptr())) {
        throw std::runtime_error("Action function '" + fname + "' is not callable");
      }
    }
  } else {
    throw std::runtime_error("Action '" + identifier_ + "' has no distribution");
  }

  for (auto tgt : node.children("target")) {
    std::string target = tgt.attribute("id").as_string();
    std::string variable = tgt.attribute("variable").as_string();
    if (!target_.empty()) target_ += ",";
    if (!variable_.empty()) variable_ += ",";
    target_ += target;
    variable_ += variable;
    targets_.push_back(make_target_binding(
      target, variable, distribution_type_ == DistributionType::TABULAR));
  }

  if (targets_.empty() && !target_.empty()) {
    targets_.push_back(make_target_binding(
      target_, variable_, distribution_type_ == DistributionType::TABULAR));
  }
}

double Action::value_at(double t) const
{
  if (points_.empty()) return 0.0;
  if (t <= points_.front().time) return points_.front().value;
  if (t >= points_.back().time) return points_.back().value;

  for (size_t i = 0; i < points_.size() - 1; ++i) {
    if (t >= points_[i].time && t <= points_[i + 1].time) {
      double f = (t - points_[i].time) / (points_[i + 1].time - points_[i].time);
      return points_[i].value + f * (points_[i + 1].value - points_[i].value);
    }
  }

  return points_.back().value;
}

Action::TargetBinding Action::make_target_binding(
  const std::string& target, const std::string& variable, bool required) const
{
  TargetBinding binding;
  binding.target = target;
  binding.variable = variable;

  if (target.empty()) {
    if (required) throw std::runtime_error("Action target is empty");
    return binding;
  }

  for (auto& circuit : model::circuits) {
    for (auto& bc : circuit->bcs) {
      if (target == bc.identifier) {
        Node* node_ptr = circuit->get_node_by_identifier(bc.node_);
        if (!node_ptr) throw std::runtime_error("Node not found for BC: " + bc.node_);

        if (variable == "bval") {
          if (bc.var_ == "P") {
            binding.setter = [node_ptr](double val) { node_ptr->tpres_gues = val; };
          } else if (bc.var_ == "T") {
            binding.setter = [node_ptr](double val) { node_ptr->ttemp_gues = val; };
          } else if (bc.var_ == "msource") {
            binding.setter = [node_ptr](double val) { node_ptr->msource = val; };
          } else {
            throw std::runtime_error("Unknown BC variable in Action: " + bc.var_);
          }
        }
        return binding;
      }
    }

    for (auto& pipe : circuit->pipes) {
      if (target == pipe->identifier) {
        if (variable == "heat_input") {
          binding.setter = [pipe](double val) { pipe->heat_input = val; };
        } else if (variable == "Kforward") {
          binding.setter = [pipe](double val) { pipe->Kforward = val; };
        } else {
          throw std::runtime_error("Unknown pipe action variable: " + variable);
        }
        return binding;
      }
    }
  }

  const std::string layer_marker = ".layer";
  auto pos = target.find(layer_marker);
  if (pos != std::string::npos) {
    std::string hslab_id = target.substr(0, pos);
    int layer_no = std::stoi(target.substr(pos + layer_marker.size()));

    for (auto& hslab : model::hslabs) {
      if (hslab->identifier == hslab_id) {
        if (layer_no < 0 || layer_no >= static_cast<int>(hslab->layers.size())) {
          throw std::runtime_error("Layer target out of range: " + target);
        }
        auto layer = hslab->layers[layer_no];
        if (variable != "heat_input") {
          throw std::runtime_error("Unknown layer action variable: " + variable);
        }
        binding.setter = [layer](double val) {
          layer->heat_input = val;
          for (auto& snode : layer->snodes) {
            snode->heat_input = snode->heat_frac * val;
          }
        };
        return binding;
      }
    }
  }

  if (required) throw std::runtime_error("Action target not found: " + target);

  std::cerr << "Warning: unresolved function action target '" << target
            << "' in action '" << identifier_ << "'" << std::endl;
  return binding;
}

void Action::apply_function_result(const py::object& result) const
{
  if (result.is_none()) return;

  py::gil_scoped_acquire gil;

  if (py::isinstance<py::dict>(result)) {
    py::dict values = result.cast<py::dict>();
    for (const auto& binding : targets_) {
      if (!binding.setter) continue;
      py::str key1(binding.target + "." + binding.variable);
      py::str key2(binding.target);
      if (values.contains(key1)) {
        binding.setter(values[key1].cast<double>());
      } else if (values.contains(key2)) {
        binding.setter(values[key2].cast<double>());
      }
    }
    return;
  }

  if (py::isinstance<py::list>(result) || py::isinstance<py::tuple>(result)) {
    py::sequence seq = result.cast<py::sequence>();
    if (seq.size() != targets_.size()) {
      throw std::runtime_error("Function action '" + identifier_
        + "' returned " + std::to_string(seq.size())
        + " values for " + std::to_string(targets_.size()) + " targets");
    }
    for (size_t i = 0; i < targets_.size(); ++i) {
      if (targets_[i].setter) targets_[i].setter(seq[i].cast<double>());
    }
    return;
  }

  double value = result.cast<double>();
  for (const auto& binding : targets_) {
    if (binding.setter) binding.setter(value);
  }
}

void Action::update(double t, double dt) const
{
  if (distribution_type_ == DistributionType::TABULAR) {
    double val = value_at(t);
    for (const auto& binding : targets_) {
      if (binding.setter) binding.setter(val);
    }
    return;
  }

  py::gil_scoped_acquire gil;
  py::object result = py_callable_(t, dt);
  apply_function_result(result);
}

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
