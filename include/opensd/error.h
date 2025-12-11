#ifndef OPENSD_ERROR_H
#define OPENSD_ERROR_H

#include <cstring>
#include <sstream>
#include <string>

#include <fmt/format.h>

#include "opensd/capi.h"
#include "opensd/settings.h"


namespace opensd {

[[noreturn]] void fatal_error(const std::string& message, int err = -1);

[[noreturn]] inline void fatal_error(std::stringstream message)
{
  fatal_error(message.str());
}

[[noreturn]] inline void fatal_error(const char* message)
{
  fatal_error(std::string {message, std::strlen(message)});
}

void write_message(const std::string& message, int level = 0);

} // namespace opensd
#endif // OPENSD_ERROR_H
