//! \file simulation.h
//! \brief Variables/functions related to a running simulation

#ifndef OPENSD_SIMULATION_H
#define OPENSD_SIMULATION_H

#include <tuple>

namespace opensd {

//==============================================================================
// Global variable declarations
//==============================================================================

namespace simulation {

extern "C" double current_time; //!< current time
extern "C" double delt; //!< time step
extern "C" bool initialized;  //!< has simulation been initialized?

} // namespace simulation

//==============================================================================
// Functions
//==============================================================================

void calculate_work();

} // namespace opensd

#endif // OPENSD_SIMULATION_H
