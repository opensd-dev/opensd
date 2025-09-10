#ifndef OPENSD_SETTINGS_H
#define OPENSD_SETTINGS_H

//! \file settings.h
//! \brief Settings for OpenSD

#include <string>

#include "pugixml.hpp"

#include "opensd/constants.h"
#include "opensd/vector.h"

namespace opensd {

//==============================================================================
// Global variable declarations
//==============================================================================

namespace settings {

extern std::string path_input;  //!< directory where main .xml files resides
extern RunMode run_mode;       //!< Run mode ('steady', 'design', 'sensitivity', 'optimize', 'transient restart', etc.)
// extern SolverType solver_type; //!< Solver Type (Monte Carlo or Random Ray)
extern "C" int verbosity;          //!< How verbose to make output
extern double alpha_mom; //!< relaxation factor
extern double alpha_ener; //!< relaxation factor
extern vector<double> tim_slot;
extern int no_main_iter;
extern int no_flow_iter;
extern double conv_crit_flow;
extern bool temp_solve;
extern bool flag_write;
} // namespace settings

//==============================================================================
// Functions
//==============================================================================

//! Read settings from XML file
void read_settings_xml();

//! Read settings from XML node
//! \param[in] root XML node for <settings>
void read_settings_xml(pugi::xml_node root);

} // namespace opensd

#endif // OPENSD_SETTINGS_H
