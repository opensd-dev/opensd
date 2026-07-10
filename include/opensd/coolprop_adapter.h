// coolprop_adapter.h
#ifndef OPENSD_COOLPROP_ADAPTER_H
#define OPENSD_COOLPROP_ADAPTER_H

#include "opensd/fluidframe.h"
#include "CoolProp.h"
#include "AbstractState.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

namespace opensd {

class CoolPropAdapter : public FluidFrame {
public:
  explicit CoolPropAdapter(const std::string& backend, const std::string& fluid_name)
    : backend_(backend), fluid_name_(fluid_name)
  {
    // factory returns raw pointer
    auto *raw = CoolProp::AbstractState::factory(backend_, fluid_name_);
    state_.reset(raw);
  }

  // copy / clone support: reconstruct underlying state using stored names
  CoolPropAdapter(const CoolPropAdapter& other)
    : backend_(other.backend_), fluid_name_(other.fluid_name_)
  {
    state_.reset(CoolProp::AbstractState::factory(backend_, fluid_name_));
  }

  std::shared_ptr<FluidFrame> clone() const override {
    return std::make_shared<CoolPropAdapter>(*this);
  }

  void update(int input_pair, double val1, double val2) override {
    if (input_pair == CoolProp::HmassP_INPUTS && backend_ != "INCOMP") {
      try {
        state_->update(static_cast<CoolProp::input_pairs>(input_pair), val1, val2);
        return;
      } catch (const CoolProp::CoolPropBaseError&) {
      }
    }

    if (input_pair == CoolProp::HmassP_INPUTS) {
      const double pressure = val2;
      const double tmin = std::isfinite(state_->Tmin()) ? state_->Tmin() : 400.0;
      const double tmax = std::isfinite(state_->Tmax()) ? state_->Tmax() : 1100.0;

      state_->update(CoolProp::PT_INPUTS, pressure, tmin);
      const double hmin = state_->hmass();
      state_->update(CoolProp::PT_INPUTS, pressure, tmax);
      const double hmax = state_->hmass();

      const double eps = std::max(1.0e-3, 1.0e-10 * std::abs(hmax - hmin));
      const double hmass = std::clamp(val1, hmin + eps, hmax - eps);
      double lo = tmin;
      double hi = tmax;
      for (int i = 0; i < 80; ++i) {
        const double mid = 0.5 * (lo + hi);
        state_->update(CoolProp::PT_INPUTS, pressure, mid);
        if (state_->hmass() < hmass) {
          lo = mid;
        } else {
          hi = mid;
        }
      }
      state_->update(CoolProp::PT_INPUTS, pressure, 0.5 * (lo + hi));
      return;
    }

    state_->update(static_cast<CoolProp::input_pairs>(input_pair), val1, val2);
  }

  double hmass() const override { return state_->hmass(); }
  double T() const override { return state_->T(); }
  double rhomass() const override { return state_->rhomass(); }
  double cpmass() const override { return state_->cpmass(); }
  double cvmass() const override { return state_->cvmass(); }
  double viscosity() const override { return state_->viscosity(); }
  double conductivity() const override { return state_->conductivity(); }
  double speed_sound() const override { return state_->speed_sound(); }

  int phase() const override { return state_->phase(); }

  double first_partial_deriv(int var1, int var2, int var3) const override {
    return state_->first_partial_deriv(
      static_cast<CoolProp::parameters>(var1),
      static_cast<CoolProp::parameters>(var2),
      static_cast<CoolProp::parameters>(var3));
  }

  double first_two_phase_deriv(int var1, int var2, int var3) const override {
    return state_->first_two_phase_deriv(
      static_cast<CoolProp::parameters>(var1),
      static_cast<CoolProp::parameters>(var2),
      static_cast<CoolProp::parameters>(var3));
  }

private:
  std::unique_ptr<CoolProp::AbstractState> state_;
  std::string backend_;
  std::string fluid_name_;
};

} // namespace opensd
#endif // OPENSD_COOLPROP_ADAPTER_H
