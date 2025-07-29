#ifndef OPENSD_TIMER_H
#define OPENSD_TIMER_H

#include <chrono>

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================

class Timer;

namespace simulation {

extern Timer time_finalize;
extern Timer time_massmom;
extern Timer time_total;

} // namespace simulation

//==============================================================================
//! Class for measuring time elapsed
//==============================================================================

class Timer {
public:
  using clock = std::chrono::high_resolution_clock;

  Timer() {};

  //! Start running the timer
  void start();

  //! Get total elapsed time in seconds
  //! \return Elapsed time in [s]
  double elapsed();

  //! Stop running the timer
  void stop();

  //! Stop the timer and reset its elapsed time
  void reset();

private:
  bool running_ {false};                 //!< is timer running?
  std::chrono::time_point<clock> start_; //!< starting point for clock
  double elapsed_ {0.0};                 //!< elapsed time in [s]
};

//==============================================================================
// Non-member functions
//==============================================================================

void reset_timers();

} // namespace opensd

#endif // OPENSD_TIMER_H
