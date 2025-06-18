#include "opensd/hdf5_interface.h"
#include <stdexcept>
#include <iostream>

namespace opensd {

hid_t create_or_open_file(const std::string& filename) {
  hid_t file_id = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  if (file_id < 0) throw std::runtime_error("Failed to create HDF5 file.");
  return file_id;
}

void close_file(hid_t file_id) {
  herr_t status = H5Fclose(file_id);
  if (status < 0) std::cerr << "Warning: Failed to close HDF5 file.\n";
}

void write_string(hid_t loc_id, const std::string& name, const std::string& value) {
  herr_t status = H5LTmake_dataset_string(loc_id, name.c_str(), value.c_str());
  if (status < 0) throw std::runtime_error("Failed to write string: " + name);
}

std::string read_string(hid_t loc_id, const std::string& name) {
  char buffer[1024];  // Fixed-size buffer; adjust if needed
  herr_t status = H5LTread_dataset_string(loc_id, name.c_str(), buffer);
  if (status < 0) throw std::runtime_error("Failed to read string: " + name);
  return std::string(buffer);
}

void write_scalar(hid_t loc_id, const std::string& name, double value) {
  herr_t status = H5LTset_attribute_double(loc_id, ".", name.c_str(), &value, 1);
  if (status < 0) throw std::runtime_error("Failed to write scalar attribute: " + name);
}

double read_scalar(hid_t loc_id, const std::string& name) {
  double value;
  herr_t status = H5LTget_attribute_double(loc_id, ".", name.c_str(), &value);
  if (status < 0) throw std::runtime_error("Failed to read scalar attribute: " + name);
  return value;
}

void write_vector(hid_t loc_id, const std::string& name, const std::vector<int>& vec) {
  if (vec.empty()) {
    std::cerr << "Warning: vector '" << name << "' is empty; skipping write.\n";
    return;
  }
  hsize_t dim = vec.size();
  herr_t status = H5LTmake_dataset_int(loc_id, name.c_str(), 1, &dim, vec.data());
  if (status < 0) throw std::runtime_error("Failed to write vector: " + name);
}

std::vector<int> read_vector_int(hid_t loc_id, const std::string& name) {
  hsize_t dim;
  herr_t status = H5LTget_dataset_info(loc_id, name.c_str(), &dim, nullptr, nullptr);
  if (status < 0) throw std::runtime_error("Failed to get dataset info for: " + name);

  std::vector<int> vec(dim);
  status = H5LTread_dataset_int(loc_id, name.c_str(), vec.data());
  if (status < 0) throw std::runtime_error("Failed to read vector: " + name);

  return vec;
}

void write_vector(hid_t loc_id, const std::string& name, const std::vector<double>& vec) {
  if (vec.empty()) {
    std::cerr << "Warning: vector '" << name << "' is empty; skipping write.\n";
    return;
  }
  hsize_t dim = vec.size();
  herr_t status = H5LTmake_dataset_double(loc_id, name.c_str(), 1, &dim, vec.data());
  if (status < 0) throw std::runtime_error("Failed to write vector: " + name);
}

std::vector<double> read_vector_double(hid_t loc_id, const std::string& name) {
  hsize_t dim;
  herr_t status = H5LTget_dataset_info(loc_id, name.c_str(), &dim, nullptr, nullptr);
  if (status < 0) throw std::runtime_error("Failed to get dataset info for: " + name);

  std::vector<double> vec(dim);
  status = H5LTread_dataset_double(loc_id, name.c_str(), vec.data());
  if (status < 0) throw std::runtime_error("Failed to read vector: " + name);

  return vec;
}

void write_string_attribute(hid_t group_id, const std::string& name, const std::string& value) {
  H5LTset_attribute_string(group_id, ".", name.c_str(), value.c_str());
}

void write_double_attribute(hid_t group_id, const std::string& name, double value) {
  H5LTset_attribute_double(group_id, ".", name.c_str(), &value, 1);
}

void read_string_attribute(hid_t group_id, const std::string& name, std::string& value) {
  char buffer[1024];
  H5LTget_attribute_string(group_id, ".", name.c_str(), buffer);
  value = std::string(buffer);
}

std::string read_string_attribute(hid_t group_id, const std::string& name) {
  char buffer[256]; // adjust if needed
  herr_t status = H5LTget_attribute_string(group_id, ".", name.c_str(), buffer);
  if (status < 0) throw std::runtime_error("Failed to read attribute: " + name);
  return std::string(buffer);
}

void read_double_attribute(hid_t group_id, const std::string& name, double& value) {
  H5LTget_attribute_double(group_id, ".", name.c_str(), &value);
}

double read_double_attribute(hid_t loc_id, const std::string& name) {
  double value;
  herr_t status = H5LTget_attribute_double(loc_id, ".", name.c_str(), &value);
  if (status < 0) throw std::runtime_error("Failed to read attribute: " + name);
  return value;
}

} // namespace opensd

