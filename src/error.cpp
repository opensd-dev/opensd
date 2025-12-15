#include "opensd/error.h"

#include "opensd/settings.h"

#include <cstdlib> // for exit
#include <iomanip> // for setw
#include <iostream>

//==============================================================================
// Global variables / constants
//==============================================================================

// Error codes
int OPENSD_E_UNASSIGNED {-1};
// int OPENSD_E_WARNING {1};
int OPENSD_E_INVALID_ARGUMENT {-5};

// Error message
char opensd_err_msg[256];

//==============================================================================
// Functions
//==============================================================================

namespace opensd {

void output(const std::string& message, std::ostream& out, int indent)
{
  // Set line wrapping and indentation
  int line_wrap = 80;

  // Determine length of message
  int length = message.size();

  int i_start = 0;
  int line_len = line_wrap - indent + 1;
  while (i_start < length) {
    if (length - i_start < line_len) {
      // Remainder of message will fit on line
      out << message.substr(i_start) << std::endl;
      break;

    } else {
      // Determine last space in current line
      std::string s = message.substr(i_start, line_len);
      auto pos = s.find_last_of(' ');

      // Write up to last space, or whole line if no space is present
      out << s.substr(0, pos) << '\n' << std::setw(indent) << " ";

      // Advance starting position
      i_start += (pos == std::string::npos) ? line_len : pos + 1;
    }
  }
}

void write_message(const std::string& message, int level)
{

  if (level <= settings::verbosity) {
    std::cout << " ";
    output(message, std::cout, 1);
  }
}

void fatal_error(const std::string& message, int err)
{

  // Write error message
  std::cerr << " ERROR: ";
  output(message, std::cerr, 8);

  // Abort the program
  std::exit(err);
}

} // namespace opensd
