// fluidframe.h
#ifndef OPENSD_FLUIDFRAME_H
#define OPENSD_FLUIDFRAME_H

#include <memory>

namespace opensd {

class FluidFrame {
public:
  virtual ~FluidFrame() = default;

  // Polymorphic deep copy
  virtual std::shared_ptr<FluidFrame> clone() const = 0;

  // Core operations
  virtual void update(int input_pair, double val1, double val2) = 0;

  // Common accessors used by solvers
  virtual double hmass() const = 0;
  virtual double smass() const = 0;
  virtual double T() const = 0;
  virtual double rhomass() const = 0;
  virtual double cpmass() const = 0;
  virtual double cvmass() const = 0;
  virtual double viscosity() const = 0;
  virtual double conductivity() const = 0;
  virtual double speed_sound() const = 0;
  virtual double Qth() const = 0;

  // Phase and derivatives
  virtual int phase() const = 0;
  virtual double first_partial_deriv(int var1, int var2, int var3) const = 0;
  virtual double first_two_phase_deriv(int var1, int var2, int var3) const = 0;
};

} // namespace opensd
#endif // OPENSD_FLUIDFRAME_H
