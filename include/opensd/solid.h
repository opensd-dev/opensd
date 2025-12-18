// solid.h
#ifndef OPENSD_SOLID_H
#define OPENSD_SOLID_H

// #include <string>
// #include <memory>
#include <vector>
#include "opensd/solidframe.h"
#include "opensd/xml_interface.h" // assumed to provide pugi::xml_node
#include "opensd/memory.h"        // if you have helpers; safe to include
// #include <pugixml.hpp>            // include explicitly if xml_interface doesn't

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================

class Solid;

namespace model {

// extern std::unordered_map<int32_t, int32_t> material_map;
extern vector<unique_ptr<Solid>> solids;

} // namespace model

//==============================================================================
//! A fluid with properties
//==============================================================================

class Solid : public SolidFrame {
public:
  //----------------------------------------------------------------------------
  // Types

  //----------------------------------------------------------------------------
  // Constructors, destructors, factory functions
  // Solid() {};
  Solid(pugi::xml_node solid_node);
  ~Solid() override = default;


  // explicit Solid(pugi::xml_node fluid_node);
  Solid(const Solid& other) = default;                       // copyable
  std::shared_ptr<SolidFrame> clone() const override {
    return std::make_shared<Solid>(*this);
  }

  //----------------------------------------------------------------------------
  // Methods

  void update(double temperature) override;

  //----------------------------------------------------------------------------
  // Accessors

  int32_t id() const { return id_; }
  const std::string& name() const { return name_; }

  double rhomass() const override { return rhomass_; }
  double cpmass() const override { return cpmass_; }
  double conductivity() const override { return conductivity_; }

private:
  //----------------------------------------------------------------------------
  // Private methods
  
  //----------------------------------------------------------------------------
  // Private data members
  int32_t id_ {-1};
  std::string name_;

  double rhomass_ {0.0};
  double cpmass_ {0.0};
  double viscosity_ {0.0};
  double conductivity_ {0.0};

};

//==============================================================================
// Non-member functions
//==============================================================================

//! Read fluid data from fluids.xml
void read_solids_xml();

//! Read fluid data XML node
//! \param[in] root node of fluids XML element
void read_solids_xml(pugi::xml_node root);

// void free_memory_material();

} // namespace opensd
#endif // OPENSD_SOLID_H

