//! \file initialize.cpp
#include "opensd/initialize.h"

#include <iostream>

#include "opensd/capi.h"
// #include "opensd/constants.h"
#include "opensd/action.h"
#include "opensd/error.h"
#include "opensd/file_utils.h"
#include "opensd/fluid.h"
#include "opensd/solid.h"
#include "opensd/settings.h"
#include "opensd/geometry.h"
#include "opensd/message_passing.h"
#include "opensd/openmp_interface.h"
#include "opensd/output.h"
#include "opensd/post.h"
#include "opensd/string_utils.h"
#include <petscsys.h>

int opensd_init(int argc, char* argv[], const void* intracomm) {
  using namespace opensd;

#ifdef OPENSD_MPI
  // Check if intracomm was passed
  MPI_Comm comm;
  if (intracomm) {
    comm = *static_cast<const MPI_Comm*>(intracomm);
  } else {
    comm = PETSC_COMM_WORLD;
  }

  // Initialize MPI for C++
  initialize_mpi(comm);
#endif

  // Parse command-line arguments
  int err = parse_command_line(argc, argv);
  if (err)
    return err;

  // Read XML input files
  // if (!read_model_xml())
  read_separate_xml_files();
  discretize_pipes();
  discretize_layers();
  
  bool loaded_restart = false;
  if (settings::run_mode == RunMode::TRANSIENT) {
  try {
  hid_t file_id = H5Fopen("circuits.h5", H5F_ACC_RDONLY, H5P_DEFAULT);

  size_t index = 0;
  for (auto& circuit : model::circuits) {
    std::string group_name = "/circuits/circuit_" + std::to_string(index);
    if (H5Lexists(file_id, group_name.c_str(), H5P_DEFAULT) <= 0) {
      std::cerr << "Warning: Expected circuit group not found: " << group_name << "\n";
      break;
    }

    hid_t group_id = H5Gopen(file_id, group_name.c_str(), H5P_DEFAULT);
    if (!circuit) {
      std::cerr << "Error: model::circuits[" << index << "] is null. Skipping.\n";
    } else {
      circuit->load_from_hdf5(group_id);  // update existing object
    }
    H5Gclose(group_id);
    ++index;
  }

  H5Fclose(file_id);

  std::cout << "Loaded data into " << index << " existing circuit(s) from HDF5.\n";
  loaded_restart = true;

} catch (const std::exception& e) {
  std::cerr << "HDF5 error during load: " << e.what() << std::endl;
}
  }

initialize_circuits(); //assign properties
initialize_hslabs();

if (loaded_restart) {
  try {
    hid_t file_id = H5Fopen("circuits.h5", H5F_ACC_RDONLY, H5P_DEFAULT);
    if (H5Lexists(file_id, "/hslabs", H5P_DEFAULT) > 0) {
      hid_t hslabs_group = H5Gopen(file_id, "/hslabs", H5P_DEFAULT);
      for (size_t i = 0; i < model::hslabs.size(); ++i) {
        std::string group_name = "hslab_" + std::to_string(i);
        if (H5Lexists(hslabs_group, group_name.c_str(), H5P_DEFAULT) <= 0) continue;
        hid_t group_id = H5Gopen(hslabs_group, group_name.c_str(), H5P_DEFAULT);
        model::hslabs[i]->load_from_hdf5(group_id);
        H5Gclose(group_id);
      }
      H5Gclose(hslabs_group);
      std::cout << "Loaded HSlab state from HDF5.\n";
    }
    H5Fclose(file_id);
  } catch (const std::exception& e) {
    std::cerr << "HDF5 error during HSlab load: " << e.what() << std::endl;
  }
}

// Print to verify
for (size_t i = 0; i < model::circuits.size(); ++i) {
  const auto& circuit = model::circuits[i];
  std::cout << "Circuit [" << i << "] ID: " << circuit->identifier << "\n";
  std::cout << "  mean_flow: " << circuit->mean_flow << "\n";
  std::cout << "  eps_h: " << circuit->eps_h << "\n";

  // Nodes
  // std::cout << "  Nodes (" << circuit->nodes.size() << "):\n";
  // for (size_t j = 0; j < circuit->nodes.size(); ++j) {
  //   const auto& node = circuit->nodes[j];
  //   if (node) {
  //     std::cout << "    Node [" << j << "] ID: " << node->identifier << "\n";
  //     std::cout << "      volume: " << node->volume << "\n";
  //     std::cout << "      tpres_old: " << node->tpres_old << "\n";
  //     std::cout << "      tpres_gues: " << node->tpres_gues << "\n";
  //   } else {
  //     std::cout << "    Node [" << j << "] is null\n";
  //   }
  // }

  /*
  // Pipes
  std::cout << "  Pipes (" << circuit->pipes.size() << "):\n";
  for (size_t j = 0; j < circuit->pipes.size(); ++j) {
    const auto& pipe = circuit->pipes[j];
    if (pipe) {
      std::cout << "    Pipe [" << j << "] ID: " << pipe->identifier << "\n";
      std::cout << "      length: " << pipe->length << "\n";
      std::cout << "      diameter: " << pipe->diameter << "\n";
    } else {
      std::cout << "    Pipe [" << j << "] is null\n";
    }
  }

  // BCs
  std::cout << "  BCs (" << circuit->bcs.size() << "):\n";
  for (size_t j = 0; j < circuit->bcs.size(); ++j) {
    const auto& bc = circuit->bcs[j];
    std::cout << "    BC [" << j << "] ID: " << bc.identifier << "\n";
    std::cout << "      node: " << bc.node_ << "\n";
    std::cout << "      var: " << bc.var_ << "\n";
    std::cout << "      val: " << bc.val_ << "\n";
  }
  */
    // Faces
  std::cout << "  Faces (" << circuit->faces.size() << "):\n";
  for (size_t j = 0; j < circuit->faces.size(); ++j) {
    const auto& face = circuit->faces[j];
    if (face) {
      std::cout << "    Face [" << j << "] faceno: " << face->faceno << "\n";

      // if (face->unode) {
      //   std::cout << "      unode ID: " << face->unode->identifier << "\n";
      //   std::cout << "      ufrac: " << face->ufrac << ", uheight: " << face->uheight << "\n";
      // } else {
      //   std::cout << "      unode is null\n";
      // }
      //
      // if (face->dnode) {
      //   std::cout << "      dnode ID: " << face->dnode->identifier << "\n";
      //   std::cout << "      dfrac: " << face->dfrac << ", dheight: " << face->dheight << "\n";
      // } else {
      //   std::cout << "      dnode is null\n";
      // }

      std::cout << "      vflow_old: " << face->vflow_old << ", vflow_gues " << face->vflow_gues << "\n";
      std::cout << "      choked: " << std::boolalpha << face->choked << "\n";
      std::cout << "      heat_input: " << face->heat_input << "\n";
      // std::cout << "      heat_hslab size: " << face->heat_hslab.size() << "\n";

    } else {
      std::cout << "    Face [" << j << "] is null\n";
    }
  }

  
}

  for (size_t i = 0; i < model::hslabs.size(); ++i) {
    const auto& hslab = model::hslabs[i];
    std::cout << "HSlab [" << i << "] ID: " << hslab->identifier << "\n";
    std::cout << "  mean_ht: " << hslab->mean_ht << "\n";
    // std::cout << "  eps_h: " << circuit->eps_h << "\n";
  }
	
  return 0;
}

namespace opensd {

#ifdef OPENSD_MPI
void initialize_mpi(MPI_Comm intracomm)
{
  mpi::intracomm = intracomm;

  // Determine number of processes and rank for each
  MPI_Comm_size(intracomm, &mpi::n_procs);
  MPI_Comm_rank(intracomm, &mpi::rank);
  mpi::master = (mpi::rank == 0);
  std::cerr << ">> MPI rank: " << opensd::mpi::rank << std::endl;
}
#endif // OPENSD_MPI

int parse_command_line(int argc, char* argv[])
{
  int last_flag = 0;
  for (int i = 1; i < argc; ++i) {
    std::string arg {argv[i]};
    if (arg[0] == '-') {
      if (arg == "-s" || arg == "--threads") {
        // Read number of threads
        i += 1;

#ifdef _OPENMP
        // Read and set number of OpenMP threads
        int n_threads = std::stoi(argv[i]);
        if (n_threads < 1) {
          std::string msg {"Number of threads must be positive."};
          strcpy(opensd_err_msg, msg.c_str());
          return OPENSD_E_INVALID_ARGUMENT;
        }
        omp_set_num_threads(n_threads);

#else
        if (mpi::master) {
          warning("Ignoring number of threads specified on command line.");
        }
#endif

} else {
        fmt::print(stderr, "Unknown option: {}\n", argv[i]);
        print_usage();
        return OPENSD_E_UNASSIGNED;
      }
      last_flag = i;
    }
  }

  #pragma omp parallel
  {
    #pragma omp single
    std::cout << "Threads used = " << omp_get_num_threads() << std::endl;
  }

  // Determine directory where XML input files are
  if (argc > 1 && last_flag < argc - 1) {
    settings::path_input = std::string(argv[last_flag + 1]);

    // check that the path is either a valid directory or file
    if (!dir_exists(settings::path_input) &&
        !file_exists(settings::path_input)) {
      fatal_error(fmt::format(
        "The path specified to the OpenSD executable '{}' does not exist.",
        settings::path_input));
    }

    // Add slash at end of directory if it isn't there
    if (!ends_with(settings::path_input, "/") &&
        dir_exists(settings::path_input)) {
      settings::path_input += "/";
    }
  }

  return 0;
}

void read_separate_xml_files()
{
  read_settings_xml();
  read_fluids_xml();
  for (auto& fptr : opensd::model::fluids) {
    std::cout << "Fluid ID: " << fptr->id()
              << ", Name: " << fptr->name()
              << ", Cp: " << fptr->cpmass()
              << ", Density: " << fptr->rhomass() << " kg/m3" << std::endl;
  }
  read_solids_xml();
  for (auto& fptr : opensd::model::solids) {
    std::cout << "Solid ID: " << fptr->id()
              << ", Name: " << fptr->name()
              << ", Cp: " << fptr->cpmass()
              << ", Density: " << fptr->rhomass() << " kg/m3" << std::endl;
  }
  read_geometry_xml();
  // read_conditions_xml();

  read_actions_xml();  // <— Add here
  read_post_xml();

  for (const auto& a : opensd::model::actions) {
    std::cout << "Action: " << a->identifier()
              << ", Target: " << a->target()
              << ", Variable: " << a->variable()
              << ", Interp: " << a->interp() << std::endl;
  }
}

} // namespace opensd
