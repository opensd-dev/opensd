#ifndef OPENSD_MEMORY_H
#define OPENSD_MEMORY_H

/*
 * See notes in include/opensd/vector.h
 *
 * In an implementation of OpenSD that uses an accelerator, we may remove the
 * use of unique_ptr, etc. below and replace it with a custom
 * implementation behaving as expected on the device.
 */

#include <memory>

namespace opensd {
using std::make_unique;
using std::shared_ptr;
using std::unique_ptr;
} // namespace opensd

#endif // OPENSD_MEMORY_H
