#ifndef OPENSD_MESSAGE_PASSING_H
#define OPENSD_MESSAGE_PASSING_H

#include <cstdint>

#ifdef OPENSD_MPI
#include <mpi.h>
#endif

#include "opensd/vector.h"

namespace opensd {
namespace mpi {

extern int rank;
extern int n_procs;
extern bool master;

#ifdef OPENSD_MPI
extern MPI_Comm intracomm;
#endif

} // namespace mpi
} // namespace opensd

#endif // OPENSD_MESSAGE_PASSING_H
