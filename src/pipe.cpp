//! \file pipe.cpp
#include "opensd/pipe.h"

#include <iostream>

#include "opensd/error.h"
#include "opensd/xml_interface.h"
#include "opensd/vector.h"

// #include <string>
// #include <cstdlib>  // for std::exit
// #include <dlfcn.h>  // for dynamic loading of libraries (Linux)
// #include <filesystem> // C++17 for current directory

namespace opensd {

//==============================================================================
// Global variables
//==============================================================================

//==============================================================================
// Wall implementation
//==============================================================================

Wall::Wall(double thk, const std::string& solname, const std::string& sollib, const std::string& restraint) {

  c1 = 5.0 / 4.0 - 0.26;

  this->thk = thk;
}

//==============================================================================
// Pipe implementation
//==============================================================================

Pipe::Pipe(pugi::xml_node pipe_node)
{
  if (check_for_node(pipe_node, "identifier")) {
    this->identifier = get_node_value(pipe_node, "identifier");
  } else {
    fatal_error("Must specify identifier of pipe in geometry XML file.");
  }

  this->diameter  = stod(get_node_value(pipe_node, "diameter"));
  this->length    = stod(get_node_value(pipe_node, "length"));
  this->dnode_str = get_node_value(pipe_node, "dnode");
  this->unode_str = get_node_value(pipe_node, "unode");
  this->ncell     = stod(get_node_value(pipe_node, "ncell"));
  this->cfarea    = stod(get_node_value(pipe_node, "cfarea"));
  this->unode = nullptr;
  this->dnode = nullptr;
  double ufrac;
  double dfrac;
  double delx;
  double delz;
  this->roughness = stod(get_node_value(pipe_node, "roughness"));
  double fricopt;

}

void Pipe::save_to_hdf5(hid_t group_id) const {
  write_string_attribute(group_id, "identifier", identifier);
  write_double_attribute(group_id, "diameter", diameter);
  write_double_attribute(group_id, "length", length);
  write_double_attribute(group_id, "ufrac", ufrac);
  write_double_attribute(group_id, "dfrac", dfrac);
}
void Pipe::load_from_hdf5(hid_t group_id) {
  // identifier = read_string(group_id, "identifier");
  length = read_double_attribute(group_id, "length");
  diameter = read_double_attribute(group_id, "diameter");
}

/*Pipe::Pipe(std::string identifier, Circuit* circuit, double diameter, double length, Node* unode, double ufrac, Node* dnode, double dfrac, double ficopt, double roughness, int ncell, double heat_input, double cfarea, int npar, double qcrit, double Kforward, int flowreg)
    : identifier(identifier), circuit(circuit), diameter(diameter), length(length), unode(unode), ufrac(ufrac), dnode(dnode), dfrac(dfrac), ncell(ncell), heat_input(heat_input), npar(npar), qcrit(qcrit), Kforward_old(Kforward), Kforward(Kforward), cfarea1(cfarea1) {

     double delx = length / ncell;
    double delz = (dnode.elevation - unode.elevation) / ncell;

    if (delz - delx > 1.E-6) {
        std::cout << "length is less than the elevation difference. stopping " << identifier << std::endl;
        exit(EXIT_FAILURE);
    }

    if (cfarea == NULL) {
        cfarea = M_PI * std::pow(diameter, 2) / 4.0;
    }

    cfarea *= npar;

    nodes.reserve(ncell - 1);
    for (int i = 0; i < ncell - 1; ++i) {
        Node node(identifier + "_node" + std::to_string(i), delx * cfarea, unode.elevation + (dnode.elevation - unode.elevation) * (i + 1) / ncell);
        node.i = i;
        nodes.push_back(node);
    }

    faces.reserve(ncell);
    for (int i = 0; i < ncell; ++i) {
        if (i == 0 && ncell == 1) {
            faces.push_back(PFace(i, *this, unode, ufrac, dnode, dfrac, diameter, cfarea, delx, delz, ficopt, roughness));
        } else if (i == 0) {
            faces.push_back(PFace(i, *this, unode, ufrac, nodes[0], NULL, diameter, cfarea, delx, delz, ficopt, roughness));
        } else if (i == ncell - 1) {
            faces.push_back(PFace(i, *this, nodes[ncell - 2], NULL, dnode, dfrac, diameter, cfarea, delx, delz, ficopt, roughness));
        } else {
            faces.push_back(PFace(i, *this, nodes[i - 1], NULL, nodes[i], NULL, diameter, cfarea, delx, delz, ficopt, roughness));
        }
    }

    for (int i = 0; i < ncell - 1; ++i) {
        nodes[i].ifaces.push_back(faces[i]);
        nodes[i].ofaces.push_back(faces[i + 1]);
    }

    unode.ofaces.push_back(faces[0]);

    dnode.ifaces.push_back(faces[ncell - 1]);

    mflow = 0.0;
 }*/

void Pipe::add_wall(double thk, std::string solname, std::string sollib, int restraint) {
  // for (auto& face : faces) {
    // face.wall = Wall(thk,solname,sollib,restraint);
  // }
}

// void Pipe::update_mflow() {
    // std::vector<double> mface;
    // for (auto& face : faces) {
        // mface.push_back(face.mflow);
    // }
    // if (!mface.empty()) {
        // mflow = std::accumulate(mface.begin(), mface.end(), 0.0) / mface.size();
    // }
// }



} // namespace opensd
