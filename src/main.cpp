#ifdef OPENSD_MPI
// #include <mpi.h>
#endif
#include <iostream>
// #include "CoolProp.h"
#include "opensd/capi.h"
#include "opensd/constants.h"
#include "opensd/error.h"
#include <petscsys.h>
#include <pybind11/embed.h>
#include <omp.h>
#include <gsl/gsl_errno.h>

namespace py = pybind11;
int main(int argc, char* argv[])
{
  // #pragma omp parallel
  // {
  //   #pragma omp single
  //   std::cout << "Threads used = " << omp_get_num_threads() << std::endl;
  // }
  using namespace opensd;
  int err;

#ifdef OPENSD_MPI
  gsl_set_error_handler_off();
  py::scoped_interpreter guard{};
  PetscInitialize(&argc, &argv, NULL, NULL); // Also initializes MPI
  MPI_Comm world = PETSC_COMM_WORLD;
  err = opensd_init(argc, argv, &world);
#else
  err = opensd_init(argc, argv, nullptr);
#endif
  if (err == -1) {
    // This happens for the -h and -v flags
    return 0;
  } else if (err) {
    // fatal_error(opensd_err_msg);
  }
  
  opensd_run();

  err = opensd_finalize();
  if (err)
    fatal_error(opensd_err_msg);

#ifdef OPENSD_MPI
  std::cerr << ">> Finalizing MPI" << std::endl;
  PetscFinalize(); // Cleans up PETSc + MPI
#endif

  return 0;
  
}
