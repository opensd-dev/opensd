//! \file constants.h
//! A collection of constants

#ifndef OPENSD_CONSTANTS_H
#define OPENSD_CONSTANTS_H


namespace opensd {

constexpr double PI {3.141592653589793238462643383279502884L};
constexpr double grav {9.81};

enum class RunMode {
  UNSET, // default value, OpenSD throws error if left to this
  STEADY,
  DESIGN,
  SENSITIVITY,
  OPTIMIZE,
  TRANSIENT
};

enum class FluidType {
  UNSET, // default value, OpenSD throws error if left to this
  COMPRESSIBLE,
  INCOMPRESSIBLE,
  TWO_PHASE
};

inline std::string fluid_type_to_string(FluidType type) {
  switch (type) {
    case FluidType::COMPRESSIBLE:   return "compressible";
    case FluidType::INCOMPRESSIBLE: return "incompressible";
    case FluidType::TWO_PHASE:      return "two_phase";
    case FluidType::UNSET:          return "unset";
  }
  return "unset"; // fallback
}

inline FluidType string_to_fluid_type(const std::string& str) {
  if (str == "compressible")   return FluidType::COMPRESSIBLE;
  if (str == "incompressible") return FluidType::INCOMPRESSIBLE;
  if (str == "two_phase")      return FluidType::TWO_PHASE;
  if (str == "unset")          return FluidType::UNSET;
  throw std::runtime_error("Unrecognized FluidType string: " + str);
}

} // namespace opensd

#endif // OPENSD_CONSTANTS_H
