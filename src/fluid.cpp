#include "opensd/fluid.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include "opensd/file_utils.h"
#include "opensd/settings.h"

namespace opensd {
namespace model {

// std::unordered_map<int32_t, int32_t> fluid_map;
std::vector<std::unique_ptr<Fluid>> fluids;

} // namespace model

//==============================================================================
// Fluid implementation
//==============================================================================

Fluid::Fluid(pugi::xml_node node)
{
  // Assign ID
  if (auto id_attr = node.attribute("id")) {
    id_ = id_attr.as_int();
  } else {
    throw std::runtime_error("Each <fluid> must have an 'id' attribute.");
  }

  // Assign name
  if (auto name_attr = node.attribute("name")) {
    name_ = name_attr.as_string();
  } else {
    name_ = "Fluid" + std::to_string(id_);
  }

  // // Populate fluid_map
  // model::fluid_map[id_] = model::fluids.size();

  // Read fluid properties (if node exists)
  if (auto n = node.child("cpmass")) cpmass_ = n.attribute("value").as_double();
  if (auto n = node.child("cvmass")) cvmass_ = n.attribute("value").as_double();
  if (auto n = node.child("rhomass")) rhomass_ = n.attribute("value").as_double();
  if (auto n = node.child("molarmass")) molarmass_ = n.attribute("value").as_double();
  if (auto n = node.child("viscosity")) viscosity_ = n.attribute("value").as_double();
  if (auto n = node.child("conductivity")) conductivity_ = n.attribute("value").as_double();
  if (auto n = node.child("adiabatic_compressibility"))
    adiabatic_compressibility_ = n.attribute("value").as_double();
  if (auto n = node.child("isothermal_compressibility"))
    isothermal_compressibility_ = n.attribute("value").as_double();
  if (auto n = node.child("boiling_point")) boiling_point_ = n.attribute("value").as_double();
  if (auto n = node.child("enthalpy_vaporization"))
    enthalpy_vaporization_ = n.attribute("value").as_double();
}

// Fluid::~Fluid()
// {
//   model::fluid_map.erase(id_);
// }

void Fluid::update(int input_pair, double val1, double val2)
{
  switch (input_pair) {
    case 9:  // PT_INPUTS
      T_ = val2;
      hmass_ = cpmass_ * T_;
      break;

    case 20:  // HmassP_INPUTS
      T_ = val1 / cpmass_;
      hmass_ = cpmass_ * T_;
      break;

    case 2:  // PQ_INPUTS
      if (val2 >= 0.0 && val2 <= 1.0) {
        T_ = boiling_point_;
        hmass_ = cpmass_ * T_ + val2 * enthalpy_vaporization_;
      } else {
        throw std::runtime_error("Q out of range in Fluid::update()");
      }
      break;

    default:
      throw std::runtime_error("input_pair not recognized in Fluid::update()");
  }
  // std::cout << T_ << " " << input_pair << " " << val1 << " " << val2 << " " << std::endl;

  speed_sound_ = std::sqrt(1.0 / (adiabatic_compressibility_ * rhomass_));
}

double Fluid::first_partial_deriv(int var1, int var2, int var3) const {
  if (var1 == 36 && var2 == 20 && var3 == 37)
    return isothermal_compressibility_ * rhomass_;
  else if (var1 == 36 && var2 == 37 && var3 == 20)
    return 0.0;
  else {
    std::cerr << "first_partial_deriv option not available. stopping\n";
    std::exit(EXIT_FAILURE);
  }
}

double Fluid::first_two_phase_deriv(int var1, int var2, int var3) const {
  return first_two_phase_deriv_;
}



//==============================================================================
// HDF5 interface (stubs — you can later implement)
//==============================================================================

// void Fluid::to_hdf5(hid_t /*group*/) const
// {
//   // TODO: Implement HDF5 write support
// }
//
// void Fluid::from_hdf5(hid_t /*group*/)
// {
//   // TODO: Implement HDF5 read support
// }

//==============================================================================
// XML readers
//==============================================================================

void read_fluids_xml()
{
  // Check if fluids.xml exists. If not, just return since it is optional
  std::string filename = settings::path_input + "fluids.xml";
  if (!file_exists(filename))
    return;

  std::cout << "Reading fluids.xml..." << std::endl;

  pugi::xml_document doc;
  if (!doc.load_file("fluids.xml")) {
    throw std::runtime_error("Error: cannot open fluids.xml");
  }

  pugi::xml_node root = doc.child("fluids");
  read_fluids_xml(root);
}

void read_fluids_xml(pugi::xml_node root)
{
  for (pugi::xml_node fluid_node : root.children("fluid")) {
    model::fluids.push_back(std::make_unique<Fluid>(fluid_node));
  }
  model::fluids.shrink_to_fit();
}

//==============================================================================
// Memory management
//==============================================================================

// void free_memory_fluid()
// {
//   model::fluids.clear();
//   model::fluid_map.clear();
// }

} // namespace opensd
