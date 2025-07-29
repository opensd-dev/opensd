#include "opensd/finalize.h"

#include "opensd/capi.h"
#include "opensd/simulation.h"
#include "opensd/timer.h"

namespace opensd {

void free_memory()
{
  // free_memory_geometry();
  // if (mpi::master) {
    // free_memory_cmfd();
  // }
  // if (settings::event_based) {
    // free_event_queues();
  // }
}

} // namespace opensd

using namespace opensd;

int opensd_finalize()
{
  if (simulation::initialized)
    opensd_simulation_finalize();

  // Clear results
  opensd_reset();

  // Reset timers
  reset_timers();

  // Reset global variables
  // settings::restart_run = false;
  // settings::run_mode = RunMode::UNSET;
  // data::energy_max = {INFTY, INFTY};
  // data::energy_min = {0.0, 0.0};

  // Deallocate arrays
  free_memory();

  // Free all MPI types
// #ifdef OPENMC_MPI
  // if (mpi::source_site != MPI_DATATYPE_NULL) {
    // MPI_Type_free(&mpi::source_site);
  // }
// #endif

  return 0;
}

int opensd_reset()
{
  return 0;
}

int opensd_reset_timers()
{
  reset_timers();
  return 0;
}
