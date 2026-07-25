#ifndef FACETHER_H
#define FACETHER_H

// #include "Upstream.h"  // Include necessary headers for Upstream, Downstream, Face, FlState, and TPTank
// #include "Downstream.h"
#include "face.h"
#include "node.h"
// #include "FlState.h"
// #include "AbstractStateSat.h"
#include "connection.h"

namespace opensd {

class Face;
    
class FaceTher {
private:
  double _Qth = -1000.;
  double _rhomass, _cpmass, _viscosity, _conductivity, _hmass, _drho_dp_consth;
public:
  // Node unode;
  // Node dnode;
  // Upstream uther;
  // Downstream dther;
  Face* face;
  bool flag_tp;
  double flowreg;
  // FlState flstate;
  
  // FaceTher(Upstream uther, Downstream dther, Face face, bool flag_tp, double flowreg, Upstream unode, Downstream dnode);
  FaceTher(Face* face);
  FaceTher() = default;
  // virtual ~FaceTher() = default;

  // void update(FlState* flstate=nullptr);
  void update();
  // void update_sat(double p=nullptr);
  void set_rhomass(double val) { _rhomass = val; }
  void set_state(double rhomass, double cpmass, double viscosity,
                 double conductivity, double hmass, double drho_dp_consth = 0.0);
  double rhomass();
  double cpmass();
  double viscosity();
  double conductivity();
  double drho_dp_consth();
  double Qth();
  double hmass();
};

} //namespace opensd

#endif
