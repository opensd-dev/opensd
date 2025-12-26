//! \file pump.h

#ifndef OPENSD_PUMP_H
#define OPENSD_PUMP_H

#include "pugixml.hpp"

#include "opensd/face.h"
#include "opensd/node.h"

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================
// class Circuit;
class Face;

//==============================================================================
//! \class Pump
//==============================================================================

class Pump : public Face {
public:
  std::string identifier;
  std::string dnode_str;
  std::string unode_str;
  // double delp_fr;
  // double delp_gr;

  int Nop;
  int flowreg;

  double delz;

  // std::shared_ptr<Circuit> circuit;

  Pump(const std::string& identifier,
      std::shared_ptr<Node> unode, double ufrac,
      std::shared_ptr<Node> dnode, double dfrac,
      int Nop, int flowreg);

  void update_abcoef(double time, double delt,
                    bool trans_sim, double alpha_mom);

  double eqn_mom(double x, double time, double delt,
                bool trans_sim, double alpha_mom);

  Pump() = default;
  virtual ~Pump() = default;

  // virtual void assign_statevar();

  // void save_to_hdf5(hid_t group_id) const override;
  // void load_from_hdf5(hid_t group_id) override;

};

//==============================================================================
// Non-member functions
//==============================================================================


} // namespace opensd

#endif // OPENSD_PUMP_H
