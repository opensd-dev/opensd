#include "opensd/message_passing.h"

namespace opensd {
namespace mpi {

int rank {0};
int n_procs {1};
bool master {true};

#ifdef OPENSD_MPI
MPI_Comm intracomm {MPI_COMM_NULL};
#endif

extern "C" bool opensd_master()
{
  return mpi::master;
}

} // namespace mpi

} // namespace opensd
