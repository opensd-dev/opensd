#include "opensd/output.h"

#include <algorithm> // for transform, max
#include <cstdio>    // for stdout
#include <cstring>   // for strlen
#include <ctime>     // for time, localtime
#include <fstream>
#include <iomanip> // for setw, setprecision, put_time
#include <ios>     // for fixed, scientific, left
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <utility> // for pair

#include <fmt/core.h>
#include <fmt/ostream.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include "xtensor/xview.hpp"

#include "opensd/capi.h"
#include "opensd/constants.h"
#include "opensd/error.h"
#include "opensd/geometry.h"
// #include "opensd/math_functions.h"
#include "opensd/message_passing.h"
// #include "opensd/plot.h"
#include "opensd/settings.h"
#include "opensd/simulation.h"
#include "opensd/timer.h"

namespace opensd {

//==============================================================================

void title()
{
fmt::print(
"               ||     ||     ||     ||\n"
"            ====||=====||=====||=====||====\n"
"           || ███ || ███ || ███ || ███ ||\n"
"           ||____||____||____||____||\n"
"               ||     ||     ||     ||\n"
"         ================================\n"
"        ||         OpenSD Solver        ||\n"
"         ================================\n"
"\n"
);

  // Write version information
  fmt::print(
    "                 | The OpenSD System Dynamics Code\n"
    "       Copyright | 2024-2025 OpenSD contributors\n"
    "         License | GPL-3.0 license\n"
    "         Version | {}.{}.{}{}{}\n");
    // VERSION_MAJOR, VERSION_MINOR, VERSION_RELEASE, VERSION_DEV ? "-dev" : "",
    // VERSION_COMMIT_COUNT);
  // fmt::print("     Commit Hash | {}\n", VERSION_COMMIT_HASH);

  // Write the date and time
  fmt::print("       Date/Time | {}\n", time_stamp());

#ifdef OPENSD_MPI
  // Write number of processors
  fmt::print("   MPI Processes | {}\n", mpi::n_procs);
#endif

#ifdef _OPENMP
  // Write number of OpenMP threads
  fmt::print("  OpenMP Threads | {}\n", omp_get_max_threads());
#endif
  fmt::print("\n");
  std::fflush(stdout);
}

//==============================================================================

std::string header(const char* msg)
{
  // Determine how many times to repeat the '=' character.
  int n_prefix = (63 - strlen(msg)) / 2;
  int n_suffix = n_prefix;
  if ((strlen(msg) % 2) == 0)
    ++n_suffix;

  // Convert to uppercase.
  std::string upper(msg);
  std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

  // Add ===>  <=== markers.
  std::stringstream out;
  out << ' ';
  for (int i = 0; i < n_prefix; i++)
    out << '=';
  out << ">     " << upper << "     <";
  for (int i = 0; i < n_suffix; i++)
    out << '=';

  return out.str();
}

std::string header(const std::string& msg)
{
  return header(msg.c_str());
}

void header(const char* msg, int level)
{
  auto out = header(msg);

  // Print header based on verbosity level.
  if (settings::verbosity >= level) {
    fmt::print("\n{}\n\n", out);
    std::fflush(stdout);
  }
}

//==============================================================================

std::string time_stamp()
{
  std::stringstream ts;
  std::time_t t = std::time(nullptr); // get time now
  ts << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
  return ts.str();
}

//==============================================================================

void print_usage()
{
  if (mpi::master) {
    fmt::print(
      "Usage: opensd [options] [path]\n\n"
      "Options:\n"
      "  -p, --plot             Run in plotting mode\n"
      "  -r, --restart          Restart a previous run from a state point\n"
      "                         or a particle restart file\n"
      "  -s, --threads          Number of OpenMP threads\n"
      "  -v, --version          Show version information\n"
      "  -h, --help             Show this message\n");
  }
}

//==============================================================================

void print_version()
{
  if (mpi::master) {
    // fmt::print("OpenSD version {}.{}.{}{}{}\n", VERSION_MAJOR, VERSION_MINOR,
      // VERSION_RELEASE, VERSION_DEV ? "-dev" : "", VERSION_COMMIT_COUNT);
    // fmt::print("Commit hash: {}\n", VERSION_COMMIT_HASH);
    fmt::print("Copyright (c) 2024-2025 "
               "OpenSD contributors\nGPL-3.0 license\n");
  }
}

//==============================================================================

void print_build_info()
{
  const std::string n("no");
  const std::string y("yes");

  std::string mpi(n);
  // std::string profiling(n);
  // std::string coverage(n);

#ifdef OPENSD_MPI
  mpi = y;
#endif
// #ifdef PROFILINGBUILD
  // profiling = y;
// #endif
// #ifdef COVERAGEBUILD
  // coverage = y;
// #endif

  // Wraps macro variables in quotes
#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

  if (mpi::master) {
    fmt::print("Build type:            {}\n", STRINGIFY(BUILD_TYPE));
    fmt::print("Compiler ID:           {} {}\n", STRINGIFY(COMPILER_ID),
      STRINGIFY(COMPILER_VERSION));
    fmt::print("MPI enabled:           {}\n", mpi);
    // fmt::print("Parallel HDF5 enabled: {}\n", phdf5);
    // fmt::print("Coverage testing:      {}\n", coverage);
    // fmt::print("Profiling flags:       {}\n", profiling);
  }
}

//==============================================================================

void show_time(const char* label, double secs, int indent_level)
{
  int width = 33 - indent_level * 2;
  fmt::print("{0:{1}} {2:<{3}} = {4:>10.4f} seconds\n", "", 2 * indent_level,
    label, width, secs);
}

void show_rate(const char* label, double particles_per_sec)
{
  fmt::print(" {:<33} = {:.6} particles/second\n", label, particles_per_sec);
}

void print_runtime()
{
  using namespace simulation;

  double local[] = {
    time_finalize.elapsed(), time_total.elapsed(), time_massmom.elapsed(),
    time_pressure_correction.elapsed(), time_pc_assembly.elapsed(),
    time_pc_solve.elapsed(), time_pc_update.elapsed(), time_pc_update_a.elapsed(),
    time_pc_update_b.elapsed(), time_pc_update_cd.elapsed(),
    time_pc_update_ef.elapsed(), time_pc_update_g.elapsed(),
    time_pc_update_h.elapsed(), time_guess_flow.elapsed(),
    time_convergence.elapsed(), time_conv_mass.elapsed(),
    time_conv_mom.elapsed(), time_update_old.elapsed(),
    time_fluid_energy.elapsed(), time_solid_energy.elapsed(),
    time_hslab_sync.elapsed(), time_hslab_assembly.elapsed(),
    time_hslab_solve.elapsed(), time_hslab_update.elapsed()
  };
  double global[24];
  MPI_Reduce(local, global, 24, MPI_DOUBLE, MPI_MAX, 0, mpi::intracomm);
  if (!mpi::master) return;

  // display header block
  header("Timing Statistics", 6);
  // if (settings::verbosity < 6)
    // return;

  // display time elapsed for various sections
  // show_time("Total time for initialization", time_initialize.elapsed());
  // show_time("Total time in simulation", time_inactive.elapsed() + time_active.elapsed());
  // show_time("Time writing statepoints", time_statepoint.elapsed(), 1);
  show_time("Total time for finalization", global[0]);
  show_time("Total time elapsed", global[1]);
  show_time("Total time for mass momentum", global[2]);
  show_time("Total time for pressure correction", global[3]);
  show_time("Total time for pc assembly", global[4]);
  show_time("Total time for pc solve", global[5]);
  show_time("Total time for pc update", global[6]);
  show_time("Total time for pc update a", global[7]);
  show_time("Total time for pc update b", global[8]);
  show_time("Total time for pc update cd", global[9]);
  show_time("Total time for pc update ef", global[10]);
  show_time("Total time for pc update g", global[11]);
  show_time("Total time for pc update h", global[12]);
  show_time("Total time for guess flow", global[13]);
  show_time("Total time for convergence", global[14]);
  show_time("Total time for convergence mass", global[15]);
  show_time("Total time for convergence momentum", global[16]);
  show_time("Total time for update old", global[17]);
  show_time("Total time for fluid energy", global[18]);
  show_time("Total time for solid energy", global[19]);
  show_time("Total time for HSLAB sync", global[20]);
  show_time("Total time for HSLAB assembly", global[21]);
  show_time("Total time for HSLAB solve", global[22]);
  show_time("Total time for HSLAB update", global[23]);

}

//==============================================================================


void print_results()
{
  // display header block for results
  header("Results", 4);
  if (settings::verbosity < 4)
    return;

}

} // namespace opensd
