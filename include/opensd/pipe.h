//! \file pipe.h

#ifndef OPENSD_PIPE_H
#define OPENSD_PIPE_H

#include "pugixml.hpp"

#include "opensd/node.h"

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================
class Circuit;
class PFace;

//==============================================================================
//! \class Pipe
//==============================================================================

class Pipe {
public:
  std::string identifier; //!< User-defined identifier
  std::string dnode_str;
  std::string unode_str;
  double diameter; //!< Pipe diameter in [m]
  double length; //!< Pipe length in [m]
  explicit Pipe(pugi::xml_node pipe_node);
  Pipe() = default;
  // virtual ~Pipe() = default;
  int ncell;
  std::shared_ptr<Node> unode;
  std::shared_ptr<Node> dnode;
  std::shared_ptr<Circuit> circuit;
  vector<std::shared_ptr<PFace>> faces;
  double ufrac;
  double dfrac;
  int npar;
  double qcrit;
  double Kforward_old;
  double Kforward = 0.;
  double heat_input;
  double mflow;
  double cfarea1;
  double roughness;

  
  Pipe(std::string identifier, std::shared_ptr<Circuit> circuit, double diameter, double length, std::shared_ptr<Node> unode, double ufrac, std::shared_ptr<Node> dnode, double dfrac, double ficopt, double roughness, int ncell, double heat_input, double cfarea, int npar, double qcrit, double Kforward, int flowreg);
  void add_wall(double thk, std::string solname, std::string sollib, int restraint);
  // void update_mflow();
  
  
protected:

};

//==============================================================================
//! \class Wall
//==============================================================================

class Wall {
public:
  Wall(double thk, const std::string& solname, const std::string& sollib, const std::string& restraint);
  
private:
  double thk;
  double c1;
  // std::unique_ptr<Solid> mech_gues;  // Unique pointer to a Solid object
};

//==============================================================================
// Non-member functions
//==============================================================================


} // namespace opensd

#endif // OPENSD_PIPE_H
