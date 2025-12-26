//! \file ger.h

#ifndef OPENSD_GER_H
#define OPENSD_GER_H

#include "pugixml.hpp"

#include "opensd/face.h"
#include "opensd/node.h"

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================
class Face;

//==============================================================================
//! \class GER
//==============================================================================

class GER : public Face {
public:
  std::string identifier;
  std::string dnode_str;
  std::string unode_str;
  double delz;
  double Ck;
  double m;
  double n;
  double delp_fr;
  double delp_gr;

  // std::shared_ptr<Circuit> circuit;

  explicit GER(pugi::xml_node pipe_node);
  GER() = default;
  virtual ~GER() = default;

  // virtual void assign_statevar();

  double eqn_mom(double x, double time, double delt, bool trans_sim, double alpha_mom) override;
  void update_abcoef(double time, double delt, double trans_sim, double alpha_mom) override;
  //
  //
  // void update_old() override;
  // void update_gues() override;
  // void update_velocity() override;
  //
  // void update_heat_input() override;
  //
  // void save_to_hdf5(hid_t group_id) const override;
  // void load_from_hdf5(hid_t group_id) override;

};


//==============================================================================
// Non-member functions
//==============================================================================


} // namespace opensd

#endif // OPENSD_GER_H
