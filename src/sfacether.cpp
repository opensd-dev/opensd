//! \file sfacether.cpp

#include "opensd/sfacether.h"

namespace opensd {
    
// FaceTher::FaceTher(Upstream uther, Downstream dther, Face face, bool flag_tp, double flowreg, Upstream unode, Downstream dnode) :
  // unode(unode), dnode(dnode), uther(uther), dther(dther), face(face), flag_tp(flag_tp), flowreg(flowreg), flstate(unode.circuit.flstate) {}
SFaceTher::SFaceTher(SFace* sface) :
  sface(sface) {}


} //namespace opensd
