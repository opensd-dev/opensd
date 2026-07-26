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
Timer time_conv_mass;
Timer time_conv_mom;
Timer time_pressure_correction;
Timer time_guess_flow;
Timer time_update_old;
Timer time_pc_assembly;
Timer time_pc_solve;
Timer time_pc_update;
Timer time_pc_update_a;
Timer time_pc_update_b;
Timer time_pc_update_cd;
Timer time_pc_update_ef;
Timer time_pc_update_g;
Timer time_pc_update_h;
Timer time_fluid_energy;
Timer time_solid_energy;
Timer time_actions;
Timer time_post_calcs;
Timer time_output_write;
Timer time_hdf5_save;
long long n_time_steps = 0;
long long n_main_iterations = 0;
long long n_flow_iterations = 0;
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
  simulation::time_conv_mass.reset();
  simulation::time_conv_mom.reset();
  simulation::time_update_old.reset();
  simulation::time_pc_assembly.reset();
  simulation::time_pc_solve.reset();
  simulation::time_pc_update.reset();
  simulation::time_pc_update_a.reset();
  simulation::time_pc_update_b.reset();
  simulation::time_pc_update_cd.reset();
  simulation::time_pc_update_ef.reset();
  simulation::time_pc_update_g.reset();
  simulation::time_pc_update_h.reset();
  simulation::time_fluid_energy.reset();
  simulation::time_solid_energy.reset();
  simulation::time_actions.reset();
  simulation::time_post_calcs.reset();
  simulation::time_output_write.reset();
  simulation::time_hdf5_save.reset();
  simulation::n_time_steps = 0;
  simulation::n_main_iterations = 0;
  simulation::n_flow_iterations = 0;
}

} // namespace opensd
