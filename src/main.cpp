#ifdef OPENSD_MPI
// #include <mpi.h>
#endif
#include <iostream>
// #include "CoolProp.h"
#include "opensd/capi.h"
#include "opensd/constants.h"
#include <petscsys.h>

int main(int argc, char* argv[])
{
  
  using namespace opensd;
  int err;

#ifdef OPENSD_MPI

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

  // std::cout << CoolProp::PropsSI("T","P",101325,"Q",0,"Water") << std::endl;
  std::cout << PI << std::endl;

#ifdef OPENSD_MPI
  std::cerr << ">> Finalizing MPI" << std::endl;
  PetscFinalize(); // Cleans up PETSc + MPI
#endif

  return 0;
  
}
