#ifndef OPENSD_HDF5_INTERFACE_H
#define OPENSD_HDF5_INTERFACE_H

#include <string>
#include <vector>
#include "hdf5.h"
#include "hdf5_hl.h"

namespace opensd {

hid_t create_or_open_file(const std::string& filename);
void close_file(hid_t file_id);

void write_string(hid_t loc_id, const std::string& name, const std::string& value);
std::string read_string(hid_t loc_id, const std::string& name);

void write_scalar(hid_t loc_id, const std::string& name, double value);
double read_scalar(hid_t loc_id, const std::string& name);

void write_vector(hid_t loc_id, const std::string& name, const std::vector<int>& vec);
std::vector<int> read_vector_int(hid_t loc_id, const std::string& name);
void write_vector(hid_t loc_id, const std::string& name, const std::vector<double>& vec);
std::vector<double> read_vector_double(hid_t loc_id, const std::string& name);

void write_string_attribute(hid_t group_id, const std::string& name, const std::string& value);
void write_double_attribute(hid_t group_id, const std::string& name, double value);

void read_string_attribute(hid_t group_id, const std::string& name, std::string& value);
std::string read_string_attribute(hid_t group_id, const std::string& name);
void read_double_attribute(hid_t group_id, const std::string& name, double& value);
double read_double_attribute(hid_t loc_id, const std::string& name);

} // namespace opensd

#endif
