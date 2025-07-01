//! \file face.h

#ifndef OPENSD_FACE_H
#define OPENSD_FACE_H

#include "pugixml.hpp"

#include "opensd/node.h"
#include "opensd/pipe.h"
#include "opensd/facether.h"
#include "opensd/connection.h"
#include "opensd/vector.h"

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================
class FaceTher;
class Connection;

//==============================================================================
//! \class Face
//==============================================================================

class Face {
public:
  int faceno;
  std::shared_ptr<Node> unode;
  double ufrac;
  double uheight;
  std::shared_ptr<Node> dnode;
  double dfrac;
  double dheight;
  double vflow_old;
  double vflow_gues;
  double mflow;
  double velocity;
  double heat_input_old;
  double heat_input;
  std::vector<double> heat_hslab;
  std::vector<double> heat_hslab_old;
  bool choked;
  double presidue;
  double Gcr;
  double pcr;
  double tpres_old;
  double spres_old;
  double ttemp_old;
  double stemp_old;
  double tpres_gues;
  double spres_gues;
  double ttemp_gues;
  double stemp_gues;
  FaceTher* ther_gues;
  FaceTher* ther_old;
  
  Connection* upstream;
  Connection* downstream;

  double aplus;
  double aminus;
  double bplus;
  double bminus;
  
  
  Face(int faceno, std::shared_ptr<Node> unode, double ufrac, std::shared_ptr<Node> dnode, double dfrac);
  Face() = default;
  virtual ~Face() = default;
  void assign_statevar();
  void update_statevar();
  virtual void update_gues();
  void assign_prop();
  virtual void update_old();

  virtual void update_velocity() {}
  virtual void update_fricfact() {}

  virtual double eqn_mom(double x, double time, double delt, bool trans_sim, double alpha_mom) {return 0;}
  virtual void update_abcoef(double time, double delt, double trans_sim, double alpha_mom) {}
  
  virtual void save_to_hdf5(hid_t group_id) const {}
  virtual void load_from_hdf5(hid_t group_id) {}


};

//==============================================================================
//! \class PFace
//==============================================================================

class PFace : public Face {
public:
  std::shared_ptr<Pipe> pipe;
  double diameter;
  double cfarea;
  double delx;
  double delz;
  double roughness;
  double Re;
  double fricopt;
  double fricfact_old;
  double fricfact_gues;
  double opening;
  std::shared_ptr<Circuit> circuit;
  std::shared_ptr<Wall> wall;
  
  PFace(int faceno, std::shared_ptr<Pipe> pipe, std::shared_ptr<Node> unode, double ufrac, std::shared_ptr<Node> dnode, double dfrac, 
    double diameter, double cfarea, double delx, double delz, double fricopt, double roughness);
  PFace() = default;
  virtual ~PFace() = default;

  // virtual void assign_statevar();
  double eqn_mom(double x, double time, double delt, bool trans_sim, double alpha_mom) override;
  void update_abcoef(double time, double delt, double trans_sim, double alpha_mom) override;
  

  void update_old() override;
  void update_gues() override;
  void update_velocity() override;
  
  void update_fricfact() override;

  void save_to_hdf5(hid_t group_id) const override;
  void load_from_hdf5(hid_t group_id) override;
  
};


//==============================================================================
// Non-member functions
//==============================================================================


} // namespace opensd

#endif // OPENSD_Face_H
