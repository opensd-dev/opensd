#include "opensd/timer.h"

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================

namespace simulation {

Timer time_finalize;
Timer time_total;
Timer time_massmom;
Timer time_convergence;
Timer time_pressure_correction;
Timer time_guess_flow;
Timer time_update_old;

} // namespace simulation

//==============================================================================
// Timer implementation
//==============================================================================

void Timer::start()
{
  running_ = true;
  start_ = clock::now();
}

void Timer::stop()
{
  elapsed_ = elapsed();
  running_ = false;
}

void Timer::reset()
{
  running_ = false;
  elapsed_ = 0.0;
}

double Timer::elapsed()
{
  if (running_) {
    std::chrono::duration<double> diff = clock::now() - start_;
    return elapsed_ + diff.count();
  } else {
    return elapsed_;
  }
}

//==============================================================================
// Non-member functions
//==============================================================================

void reset_timers()
{
  simulation::time_finalize.reset();
  simulation::time_massmom.reset();
  simulation::time_total.reset();
  simulation::time_guess_flow.reset();
  simulation::time_pressure_correction.reset();
  simulation::time_convergence.reset();
  simulation::time_update_old.reset();
}

} // namespace opensd
