//! \file pump.h

#ifndef OPENSD_PUMP_H
#define OPENSD_PUMP_H

#include "pugixml.hpp"

#include "opensd/face.h"
#include "opensd/node.h"
#include "opensd/interp1d.h"

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
  // double delp_fr;
  // double delp_gr;

  int Nop;
  // double ufrac;
  // double dfrac;
  // int flowreg;

  // double delz;

  // std::shared_ptr<Circuit> circuit;

  Pump(const std::string& identifier,
      double ufrac,
      double dfrac,
      double delz,
      int Nop); //int flowreg

  Pump() = default;
  virtual ~Pump() = default;

  // virtual void assign_statevar();
  // virtual double QHfuncs(double Q);
  // virtual double dHdQfuncs(double Q);


  // void save_to_hdf5(hid_t group_id) const override;
  // void load_from_hdf5(hid_t group_id) override;

};

class VSPump : public Pump {
public:
  std::string dnode_str;
  std::string unode_str;

  std::vector<double> speeds;

  std::vector<Interp1D> QH_funcs;
  std::vector<Interp1D> dHdQ_funcs;

  // double Head;
  std::string curve_file;
  double curve_speed;

  explicit VSPump(pugi::xml_node pipe_node);

  double QHfuncs(double Q) const;
  double dHdQfuncs(double Q) const;

  void update_abcoef(double time, double delt,
                    double trans_sim, double alpha_mom);

  double eqn_mom(double x, double time, double delt,
                bool trans_sim, double alpha_mom);

private:
  double interp_speed(const std::vector<Interp1D>& funcs,
                      double Q) const;
};

//==============================================================================
// Non-member functions
//==============================================================================


} // namespace opensd

#endif // OPENSD_PUMP_H
