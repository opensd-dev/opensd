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
  H5Fclose(file_id);
}

void write_string(hid_t loc_id, const std::string& name, const std::string& value) {
  herr_t status = H5LTmake_dataset_string(loc_id, name.c_str(), value.c_str());
  if (status < 0) throw std::runtime_error("Failed to write string: " + name);
}

std::string read_string(hid_t loc_id, const std::string& name) {
  char buffer[1024];
  H5LTread_dataset_string(loc_id, name.c_str(), buffer);
  return std::string(buffer);
}

void write_scalar(hid_t loc_id, const std::string& name, double value) {
  hsize_t dim = 1;
  herr_t status = H5LTmake_dataset_double(loc_id, name.c_str(), 1, &dim, &value);
  if (status < 0) throw std::runtime_error("Failed to write scalar: " + name);
}

double read_scalar(hid_t loc_id, const std::string& name) {
  double value;
  herr_t status = H5LTread_dataset_double(loc_id, name.c_str(), &value);
  if (status < 0) throw std::runtime_error("Failed to read scalar: " + name);
  return value;
}

void write_vector(hid_t loc_id, const std::string& name, const std::vector<int>& vec) {
  if (vec.empty()) {
    std::cerr << "Warning: not writing vector '" << name << "' because it's empty.\n";
    return;
  }

  hsize_t dim = vec.size();
  herr_t status = H5LTmake_dataset_int(loc_id, name.c_str(), 1, &dim, vec.data());
  if (status < 0) throw std::runtime_error("Failed to write vector: " + name);
}

std::vector<int> read_vector_int(hid_t loc_id, const std::string& name) {
  hsize_t dim;
  H5LTget_dataset_info(loc_id, name.c_str(), &dim, nullptr, nullptr);
  std::vector<int> vec(dim);
  H5LTread_dataset_int(loc_id, name.c_str(), vec.data());
  return vec;
}

} // namespace opensd
