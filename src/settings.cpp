#include "opensd/settings.h"

#include <string>
#include <iostream>

#include "opensd/error.h"
#include "opensd/xml_interface.h"

namespace opensd {
    
//==============================================================================
// Global variables
//==============================================================================

namespace settings {

  std::string path_input;
  RunMode run_mode {RunMode::UNSET};
  int verbosity {0};
  double alpha_mom;
  double alpha_ener;
  vector<double> tim_slot;
  int no_main_iter;
  int no_flow_iter;
  bool temp_solve;
  double conv_crit_flow {1.E-10};
} // namespace settings

//==============================================================================
// Functions
//==============================================================================
    
void read_settings_xml()
{
  using namespace settings;
  using namespace pugi;
  
  std::string filename = path_input + "settings.xml";
/*   if (!file_exists(filename)) {
    if (run_mode != RunMode::PLOTTING) {
      fatal_error("Could not find any XML input files! In order to run OpenMC, "
                  "you first need a set of input files; at a minimum, this "
                  "includes settings.xml, geometry.xml, and materials.xml or a "
                  "single model XML file. Please consult the user's guide at "
                  "https://docs.openmc.org for further information.");
    } else {
      // The settings.xml file is optional if we just want to make a plot.
      return;
    }
  }
 */

  // Parse settings.xml file
  xml_document doc;
  auto result = doc.load_file(filename.c_str());
  if (!result) {
    fatal_error("Error processing settings.xml file.");
  }

  // Get root element
  xml_node root = doc.document_element();

  // Verbosity
  if (check_for_node(root, "verbosity")) {
    verbosity = std::stoi(get_node_value(root, "verbosity"));
  }


  write_message("Reading settings XML file...", 5);

  read_settings_xml(root);

}

void read_settings_xml(pugi::xml_node root)
{
  using namespace settings;
  using namespace pugi;

  // Check run mode if it hasn't been set from the command line
  // xml_node node_mode;
  if (run_mode == RunMode::UNSET) {
    if (check_for_node(root, "run_mode")) {
      std::string temp_str = get_node_value(root, "run_mode", true, true);
      if (temp_str == "steady") {
        run_mode = RunMode::STEADY;
      } else if (temp_str == "design") {
        run_mode = RunMode::DESIGN;
      } else if (temp_str == "sensitivity") {
        run_mode = RunMode::SENSITIVITY;
      } else if (temp_str == "optimize") {
        run_mode = RunMode::OPTIMIZE;
      } else if (temp_str == "transient") {
        run_mode = RunMode::TRANSIENT;
      } else {
        fatal_error("Unrecognized run mode: " + temp_str);
      }
    }
  }
  
  alpha_mom = stod(get_node_value(root, "alpha_mom"));
  alpha_ener = stod(get_node_value(root, "alpha_ener"));
  tim_slot = get_node_array<double>(root, "tim_slot");
  no_main_iter = stod(get_node_value(root, "no_main_iter"));
  no_flow_iter = stod(get_node_value(root, "no_flow_iter"));
  // conv_crit_flow = stod(get_node_value(root, "conv_crit_flow"));

  bool trans_sim = settings::run_mode == RunMode::TRANSIENT;
  if (trans_sim) {
    double start = tim_slot[0];
    double end = tim_slot[1];
    double step = tim_slot[0];
    tim_slot.clear();
  
    for (double t = start; t <= end + 1e-9; t += step) {
      tim_slot.push_back(t);
    }
  } else {
    tim_slot = {0.0};
  }

}

} // namespace opensd
