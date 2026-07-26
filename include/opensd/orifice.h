//! \file orifice.h

#ifndef OPENSD_ORIFICE_H
#define OPENSD_ORIFICE_H

#include <memory>
#include <string>

#include "pugixml.hpp"

#include "opensd/face.h"

namespace opensd {

class Circuit;

class Orifice : public Face {
public:
  std::string identifier;
  std::string dnode_str;
  std::string unode_str;
  double diameter;
  double Cd;
  double opening;
  double cfarea;
  double delp_fr;
  double delp_gr;
  double G;
  double cr_ttemp = 0.0;
  double cr_hmass = 0.0;
  double cr_cpmass = 0.0;
  double cr_viscosity = 0.0;
  double cr_conductivity = 0.0;
  std::shared_ptr<Circuit> circuit;
  std::unique_ptr<CoolProp::AbstractState> flstate;

  explicit Orifice(pugi::xml_node orifice_node);
  Orifice() = default;
  virtual ~Orifice() = default;

  double eqn_mom(double x, double time, double delt, bool trans_sim, double alpha_mom) override;
  void update_abcoef(double time, double delt, double trans_sim, double alpha_mom) override;
  void update_gues() override;
  void update_velocity() override;
  void update_Gcr() override;

  void save_to_hdf5(hid_t group_id) const override;
  void load_from_hdf5(hid_t group_id) override;
};

} // namespace opensd

#endif // OPENSD_ORIFICE_H
