// fluid.h
#ifndef OPENSD_FLUID_H
#define OPENSD_FLUID_H

#include <string>
#include <memory>
#include <vector>
#include "opensd/fluidframe.h"
#include "opensd/xml_interface.h" // assumed to provide pugi::xml_node
#include "opensd/memory.h"        // if you have helpers; safe to include
#include <pugixml.hpp>            // include explicitly if xml_interface doesn't

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================

class Fluid;

namespace model {

// extern std::unordered_map<int32_t, int32_t> material_map;
extern vector<unique_ptr<Fluid>> fluids;

} // namespace model

//==============================================================================
//! A fluid with properties
//==============================================================================

class Fluid : public FluidFrame {
public:
  //----------------------------------------------------------------------------
  // // Types
  // struct ThermalTable {
  //   int index_table;   //!< Index of table in data::thermal_scatt
  //   int index_nuclide; //!< Index in nuclide_
  //   double fraction;   //!< How often to use table
  // };

  //----------------------------------------------------------------------------
  // Constructors, destructors, factory functions
  // Fluid() {};
  Fluid(pugi::xml_node fluid_node);
  ~Fluid() override = default;


  // explicit Fluid(pugi::xml_node fluid_node);
  Fluid(const Fluid& other) = default;                       // copyable
  std::shared_ptr<FluidFrame> clone() const override {
    return std::make_shared<Fluid>(*this);
  }

  //----------------------------------------------------------------------------
  // Methods

  void update(int input_pair, double val1, double val2) override;

  // void calculate_xs(Particle& p) const;
  //
  // //! Assign thermal scattering tables to specific nuclides within the material
  // //! so the code knows when to apply bound thermal scattering data
  // void init_thermal();
  //
  // //! Set up mapping between global nuclides vector and indices in nuclide_
  // void init_nuclide_index();
  //
  // //! Finalize the material, assigning tables, normalize density, etc.
  // void finalize();
  //
  // //! Write material data to HDF5
  // void to_hdf5(hid_t group) const;
  //
  // //! Export physical properties to HDF5
  // //! \param[in] group  HDF5 group to write to
  // void export_properties_hdf5(hid_t group) const;
  //
  // //! Import physical properties from HDF5
  // //! \param[in] group  HDF5 group to read from
  // void import_properties_hdf5(hid_t group);
  //
  // //! Add nuclide to the material
  // //
  // //! \param[in] nuclide Name of the nuclide
  // //! \param[in] density Density of the nuclide in [atom/b-cm]
  // void add_nuclide(const std::string& nuclide, double density);
  //
  // //! Set atom densities for the material
  // //
  // //! \param[in] name Name of each nuclide
  // //! \param[in] density Density of each nuclide in [atom/b-cm]
  // void set_densities(
  //   const vector<std::string>& name, const vector<double>& density);
  //
  // //! Clone the material by deep-copying all members, except for the ID,
  // //  which will get auto-assigned to the next available ID. After creating
  // //  the new material, it is added to openmc::model::materials.
  // //! \return reference to the cloned material
  // Material& clone();
  //
  //----------------------------------------------------------------------------
  // Accessors

  int32_t id() const { return id_; }
  const std::string& name() const { return name_; }

  double rhomass() const override { return rhomass_; }
  double cpmass() const override { return cpmass_; }
  double cvmass() const override { return cvmass_; }
  double molarmass() const { return molarmass_; }
  double viscosity() const override { return viscosity_; }
  double conductivity() const override { return conductivity_; }
  double adiabatic_compressibility() const { return adiabatic_compressibility_; }
  double isothermal_compressibility() const { return isothermal_compressibility_; }
  double boiling_point() const { return boiling_point_; }
  double enthalpy_vaporization() const { return enthalpy_vaporization_; }
  double T() const { return T_; }
  double hmass() const { return hmass_; }
  double speed_sound() const { return speed_sound_; }
  double first_partial_deriv(int var1, int var2, int var3) const override;
  double first_two_phase_deriv(int var1, int var2, int var3) const override;
  int phase() const override { return 0; } // or whatever logic you use


  //----------------------------------------------------------------------------
  // // Data
  // int32_t id_ {C_NONE};                 //!< Unique ID
  // std::string name_;                    //!< Name of material
  // vector<int> nuclide_;                 //!< Indices in nuclides vector
  // vector<int> element_;                 //!< Indices in elements vector
  // NCrystalMat ncrystal_mat_;            //!< NCrystal material object
  // xt::xtensor<double, 1> atom_density_; //!< Nuclide atom density in [atom/b-cm]
  // double density_;                      //!< Total atom density in [atom/b-cm]
  // double density_gpcc_;                 //!< Total atom density in [g/cm^3]
  // double charge_density_;               //!< Total charge density in [e/b-cm]
  // double volume_ {-1.0};                //!< Volume in [cm^3]
  // vector<bool> p0_; //!< Indicate which nuclides are to be treated with
  //                   //!< iso-in-lab scattering
  //
  // // To improve performance of tallying, we store an array (direct address
  // // table) that indicates for each nuclide in data::nuclides the index of the
  // // corresponding nuclide in the nuclide_ vector. If it is not present in the
  // // material, the entry is set to -1.
  // vector<int> mat_nuclide_index_;
  //
  // // Thermal scattering tables
  // vector<ThermalTable> thermal_tables_;
  //
  // unique_ptr<Bremsstrahlung> ttb_;

private:
  //----------------------------------------------------------------------------
  // // Private methods
  //
  // //! Calculate the collision stopping power
  // void collision_stopping_power(double* s_col, bool positron);
  //
  // //! Initialize bremsstrahlung data
  // void init_bremsstrahlung();
  //
  // //! Normalize density
  // void normalize_density();
  //
  // void calculate_neutron_xs(Particle& p) const;
  // void calculate_photon_xs(Particle& p) const;

  //----------------------------------------------------------------------------
  // Private data members
  int32_t id_ {-1};
  std::string name_;

  double cpmass_ {0.0};
  double cvmass_ {0.0};
  double rhomass_ {0.0};
  double molarmass_ {0.0};
  double viscosity_ {0.0};
  double conductivity_ {0.0};
  double adiabatic_compressibility_ {0.0};
  double isothermal_compressibility_ {0.0};
  double boiling_point_ {0.0};
  double enthalpy_vaporization_ {0.0};

  double T_;
  double hmass_;
  double speed_sound_;

  double first_two_phase_deriv_ = 0.0;       // default can be changed

};

//==============================================================================
// Non-member functions
//==============================================================================

//! Read fluid data from fluids.xml
void read_fluids_xml();

//! Read fluid data XML node
//! \param[in] root node of fluids XML element
void read_fluids_xml(pugi::xml_node root);

// void free_memory_material();

} // namespace opensd
#endif // OPENSD_FLUID_H

