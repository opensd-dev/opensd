//! \file sfacether.cpp

#include "opensd/sfacether.h"
#include "opensd/layer.h"

namespace opensd {
    
// FaceTher::FaceTher(Upstream uther, Downstream dther, Face face, bool flag_tp, double flowreg, Upstream unode, Downstream dnode) :
  // unode(unode), dnode(dnode), uther(uther), dther(dther), face(face), flag_tp(flag_tp), flowreg(flowreg), flstate(unode.circuit.flstate) {}
SFaceTher::SFaceTher(SFace* sface) :
  sface(sface) {}

void SFaceTher::update() {
	double delx1 = sface->unode->layer->delx;
	double delx2 = sface->dnode->layer->delx;
	double k1 = sface->unode->ther_gues->conductivity();
	double k2 = sface->dnode->ther_gues->conductivity();
	
  _conductivity = (delx1+delx2)/(delx1/k1+delx2/k2);
}

void SFaceTher::update_old() {
  double delx1 = sface->unode->layer->delx;
  double delx2 = sface->dnode->layer->delx;
  double k1 = sface->unode->ther_old->conductivity();
  double k2 = sface->dnode->ther_old->conductivity();

  _conductivity = (delx1 + delx2) / (delx1 / k1 + delx2 / k2);
}

double SFaceTher::conductivity() {
  return _conductivity;
}


} //namespace opensd
