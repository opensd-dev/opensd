//! \file initialize.cpp
#include "opensd/initialize.h"

#include <iostream>

// #include "opensd/capi.h"
// #include "opensd/constants.h"
#include "opensd/settings.h"
#include "opensd/geometry.h"

int opensd_init(int argc, char* argv[])
{
  using namespace opensd;

  // Read XML input files
  // if (!read_model_xml())
  read_separate_xml_files();
  discretize_pipes();
  initialize_circuits();
    
  return 0;
}

namespace opensd {
    
void read_separate_xml_files()
{
  read_settings_xml();
  read_geometry_xml();

}

} // namespace opensd
