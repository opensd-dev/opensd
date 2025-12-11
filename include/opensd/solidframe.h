// solidframe.h
#ifndef OPENSD_SOLIDFRAME_H
#define OPENSD_SOLIDFRAME_H

#include <memory>

namespace opensd {

class SolidFrame {
public:
  virtual ~SolidFrame() = default;

  // Polymorphic deep copy
  virtual std::shared_ptr<SolidFrame> clone() const = 0;

  // Core operations
  // virtual void update(int input_pair, double val1, double val2) = 0;

  // Common accessors used by solvers
  virtual double rhomass() const = 0;
  virtual double cpmass() const = 0;
  virtual double conductivity() const = 0;

};

} // namespace opensd
#endif // OPENSD_SOLIDFRAME_H
