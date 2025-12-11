#ifndef SFACETHER_H
#define SFACETHER_H

// #include "Upstream.h"  // Include necessary headers for Upstream, Downstream, Face, FlState, and TPTank
// #include "Downstream.h"
#include "sface.h"
#include "snode.h"
// #include "FlState.h"
// #include "AbstractStateSat.h"
// #include "connection.h"

namespace opensd {

class SFace;
    
class SFaceTher {
private:
  double _rhomass, _cpmass, _conductivity;
public:
  // Node unode;
  // Node dnode;
  // Upstream uther;
  // Downstream dther;
  SFace* sface;
  // bool flag_tp;
  // double flowreg;
  // FlState flstate;
  
  // FaceTher(Upstream uther, Downstream dther, Face face, bool flag_tp, double flowreg, Upstream unode, Downstream dnode);
  SFaceTher(SFace* sface);
  SFaceTher() = default;
  // virtual ~FaceTher() = default;

  // void update(FlState* flstate=nullptr);
  void update();
  // void update_sat(double p=nullptr);
  // void set_rhomass(double val) { _rhomass = val; }
  // double rhomass();
  // double cpmass();
  // double viscosity();
  double conductivity();
  // double drho_dp_consth();
  // double Qth();
  // double hmass();
};

} //namespace opensd

#endif
