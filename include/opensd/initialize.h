#ifndef OPENSD_INITIALIZE_H
#define OPENSD_INITIALIZE_H

#include <string>

#include "opensd/circuit.h"
#include "opensd/hslab.h"

#ifdef OPENSD_MPI
#include <mpi.h>
#endif

namespace opensd {

int parse_command_line(int argc, char* argv[]);
#ifdef OPENSD_MPI
void initialize_mpi(MPI_Comm intracomm);
#endif

//! Read circuit, heat slab, and settings from a single XML file
// bool read_model_xml();
//! Read inputs from separate XML files
void read_separate_xml_files();
//! Write some output that occurs right after initialization
// void initial_output();

void discretize_pipes();
void discretize_layers();

} // namespace opensd

#endif // OPENSD_INITIALIZE_H
