//! \file node.h

#ifndef OPENSD_NODE_H
#define OPENSD_NODE_H

#include <memory>
#include <optional>
#include "pugixml.hpp"

// #include "opensd/circuit.h"
#include "opensd/vector.h"
#include "CoolProp.h"
#include "fluidframe.h"
#include "AbstractState.h"
#include "hdf5_interface.h"
#include <petscksp.h>

namespace opensd {
using std::shared_ptr;

//==============================================================================
// Global variables
//==============================================================================
class Circuit;
class Face;

//==============================================================================
//! \class Node
//==============================================================================

class Node {
public:
  std::string identifier; //!< User-defined identifier
  double mresidue = 0.0;
  double msource = 0.0;
  vector<std::shared_ptr<Face>> ifaces;
  vector<std::shared_ptr<Face>> ofaces;
  double mflow_in = 1.0E-4;
  double mflow_out = 0.0;
  Circuit* circuit = nullptr;
  int node_ind = -1;
  bool is_reservoir = false;
  bool is_tptank = false;
  
  double tpres_old = 0.0;
  double ttemp_old = 0.0;
  double tenth_old = 0.0;
  double tpres_gues = 0.0;
  double ttemp_gues = 0.0;
  double tenth_gues = 0.0;

  double spres_old = 0.0;
  double stemp_old = 0.0;
  double senth_old = 0.0;
  double spres_gues = 0.0;
  double stemp_gues = 0.0;
  double senth_gues = 0.0;
  
  double heat_input = 0.0;
  double elevation = 0.0;
  
  std::set<std::string> fixed_var;
  double esource = 0.0;
  double hresidue = 0.0;
  double volume = 0.0;
  double level = 0.0;
  double level_old = 0.0;
  double height = 0.0;
  double cross_area = 0.0;
  double tpvolume = 0.0;
  double volfracliq = 0.0;
  double watermass = 0.0;
  double Tsat = 0.0;
  double hf = 0.0;
  double hg = 0.0;
  double rhof = 0.0;
  double rhog = 0.0;
  double muf = 0.0;
  double mug = 0.0;
  double cpf = 0.0;
  double cpg = 0.0;
  double kf = 0.0;
  double kg = 0.0;
  shared_ptr<FluidFrame> ther_gues;
  shared_ptr<FluidFrame> ther_old;
  double velocity = 0.0;
  double B1 = 0.0;
  
  double _heat_input_esource = 0.0;
  double _heat_input_msource = 0.0;
  double _heat_input_faceconv = 0.0;
  double _heat_input_faceconv2 = 0.0;
  double _heat_input_facegen = 0.0;
  std::vector<double> heat_hslab;
  std::vector<PetscScalar> Acols;
  std::vector<PetscScalar> Avals;
  PetscScalar brow;

  explicit Node(pugi::xml_node flnode_node);
  Node(std::string identifier, double volume, double heat_input, double elevation, double tpres_old = 0.0, double ttemp_old = 0.0, double tenth_old = 0.0); //Circuit* circuit, 
  Node() = default;
  // virtual ~Node() = default;
  // ~Node() {
  // delete ther_old;
  // delete ther_gues;
  // }

  double eqn_cont(double time, double delt, bool trans_sim, double alpha_mom);
  double eqn_ener(double time, double delt, bool trans_sim, double alpha_ener);
  void update_gues();
  void assign_staticvar();
  void update_statictemp();
  void update_totalenth();
  void update_staticvar(std::optional<double> velocity_in = std::nullopt);
  void assign_prop();
  void update_old();
  void update_staticpres();
  void update_totaltemp(); 
  void update_staticenth();
  void update_sat(double pressure = -1.0);
  void update_level();

  void save_to_hdf5(hid_t group_id) const;
  void load_from_hdf5(hid_t group_id);


};

//==============================================================================
// Non-member functions
//==============================================================================


} // namespace opensd

#endif // OPENSD_NODE_H
