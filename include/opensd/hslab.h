//! \file hslab.h

#ifndef OPENSD_HSLAB_H
#define OPENSD_HSLAB_H

#include "pugixml.hpp"
// #include "opensd/constants.h"
#include "opensd/layer.h"
#include "opensd/pipe.h"
// #include "opensd/bc.h"
#include "opensd/face.h"
#include "opensd/vector.h"
#include "opensd/memory.h"
// #include "hdf5_interface.h"
// #include <petscksp.h>
// #include <petscsnes.h>

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================

class HSlab;

namespace model {
extern vector<std::shared_ptr<HSlab>> hslabs;
extern vector<std::shared_ptr<HSlab>> hslabs_owned;
} // namespace model

//==============================================================================
//! \class HSlab
//==============================================================================

class HSlab {
public:
  std::string identifier; //!< User-defined identifier
  explicit HSlab(pugi::xml_node hslab_node);
  HSlab() = default;
  int ninc;
  int nlayers;
  double uarea;
  double darea;
  std::string uvar;
  std::string dvar;
  std::string ucompid;
  std::string dcompid;
  double uval;
  vector<std::shared_ptr<Face>> uval1;
  vector<std::shared_ptr<Face>> dval1;
  std::shared_ptr<Pipe> upipe;
  std::shared_ptr<Pipe> dpipe;
  double dval;
  // double eps_m;
  vector<double> eps_tlist;
  vector<double> htlist;
  // double mean_flow;
  double mean_ht;
  // double eps_h;
  // double eps_p;
  vector<std::shared_ptr<Layer>> layers;
  vector<std::shared_ptr<SNode>> uwnodes;
  vector<std::shared_ptr<SNode>> dwnodes;
  // vector<std::shared_ptr<Node>> nodes_owned;
  // vector<std::shared_ptr<Node>> ghost_nodes_owned1;
  // std::map<int, std::vector<std::shared_ptr<Node>>> ghost_nodes_owned;
  // std::map<int, std::vector<std::shared_ptr<Face>>> ghost_faces_owned;
  // vector<int> indices_owned;
  // vector<int> face_indices_owned;
  // vector<PetscInt> ghost_indices_owned;
  // vector<PetscInt> ghost_face_indices_owned;
  // vector<PetscInt> ghost_face_global_indices;
  // vector<PetscInt> old2new;
  // vector<PetscInt> face_old2new;
  // vector<std::shared_ptr<Pipe>> pipes;
  // vector<BC> bcs;
  // vector<std::shared_ptr<Face>> faces;
  // vector<std::shared_ptr<Face>> faces_owned;
  // vector<int> Pbound_ind;
  // Mat A;
  // Vec b;
  // Vec pc;
  // Mat Ah;
  // Vec bh;
  // Vec enth;
  // KSP ksp;
  // KSP ksph;
  // Vec vflow_gues_local, aminus_local, aplus_local, bplus_local, bminus_local;
  // Vec rhomass_local;
  // Vec pc_local;
  // Vec velocity_local;
  // MPI_Comm comm;
  // int rank_in_comm, comm_size;
  //
  // void save_to_hdf5(hid_t group_id) const;
  // void load_from_hdf5(hid_t group_id);
  // //! Find a node by its identifier string
  // Node* get_node_by_identifier(const std::string& id) const;


protected:

};

//==============================================================================
// Non-member functions
//==============================================================================

void read_hslabs(pugi::xml_node node);
void initialize_hslabs();


template<typename T>
std::shared_ptr<T> find_in_vector(
  const std::vector<std::shared_ptr<T>>& vec,
  const std::string& obj);

template<typename T>
std::shared_ptr<T> get_comp(const std::string& obj);



} // namespace opensd

#endif // OPENSD_HSLAB_H
