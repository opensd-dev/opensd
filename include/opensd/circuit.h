//! \file circuit.h

#ifndef OPENSD_CIRCUIT_H
#define OPENSD_CIRCUIT_H

#include "pugixml.hpp"
#include "opensd/constants.h"
#include "opensd/node.h"
#include "opensd/pipe.h"
#include "opensd/bc.h"
#include "opensd/face.h"
#include "opensd/vector.h"
#include "hdf5_interface.h"
#include <petscksp.h>
#include <petscsnes.h>

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================

namespace model {
extern vector<std::shared_ptr<Circuit>> circuits;
extern vector<std::shared_ptr<Circuit>> circuits_owned;
} // namespace model

class Circuit;

//==============================================================================
//! \class Circuit
//==============================================================================

class Circuit {
public:
  std::string identifier; //!< User-defined identifier
  std::string flname;
  FluidType fltype;       //!< Fluid Type ('compressible', 'incompressible', 'two_phase')
  explicit Circuit(pugi::xml_node cir_node);
  Circuit() = default;
  double eps_m;
  double mean_flow;
  double eps_h;
  double eps_p;
  vector<std::shared_ptr<Node>> nodes;
  vector<std::shared_ptr<Node>> nodes_owned;
  vector<std::shared_ptr<Node>> ghost_nodes_owned1;
  std::map<int, std::vector<std::shared_ptr<Node>>> ghost_nodes_owned;
  std::map<int, std::vector<std::shared_ptr<Face>>> ghost_faces_owned;
  vector<int> indices_owned;
  vector<int> face_indices_owned;
  vector<PetscInt> ghost_indices_owned;
  vector<PetscInt> ghost_face_indices_owned;
  vector<PetscInt> ghost_face_global_indices;
  vector<PetscInt> old2new;
  vector<PetscInt> face_old2new;
  vector<std::shared_ptr<Pipe>> pipes;
  vector<BC> bcs;
  vector<std::shared_ptr<Face>> faces;
  vector<std::shared_ptr<Face>> faces_owned;
  vector<int> Pbound_ind;
  Mat A;
  Vec b;
  Vec pc;
  KSP ksp;
  Vec vflow_gues_local, aminus_local, aplus_local, bplus_local, bminus_local;
  Vec rhomass_local;
  Vec pc_local;
  Vec velocity_local;
  MPI_Comm comm;
  SNES snes;
  Vec x, r;
  int rank_in_comm, comm_size;

  void save_to_hdf5(hid_t group_id) const;
  void load_from_hdf5(hid_t group_id);

protected:

};

//==============================================================================
// Non-member functions
//==============================================================================

void read_circuits(pugi::xml_node node);
void initialize_circuits();

} // namespace opensd

#endif // OPENSD_CIRCUIT_H
