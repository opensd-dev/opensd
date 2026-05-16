// bindings.cpp
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>  // for std::vector
#include <memory>
#include <stdexcept>
#include "opensd/face.h"
#include "opensd/snode.h"
#include "opensd/facether.h"

namespace py = pybind11;

PYBIND11_MODULE(bindings, m) {

    // ---------------------------
    // FaceTher binding
    // ---------------------------
    py::class_<opensd::FaceTher>(m, "FaceTher")
        .def("rhomass", &opensd::FaceTher::rhomass)
        .def("cpmass", &opensd::FaceTher::cpmass)
        .def("conductivity", &opensd::FaceTher::conductivity);

    // ---------------------------
    // Face binding
    // ---------------------------
    py::class_<opensd::Face, std::shared_ptr<opensd::Face>>(m, "Face")
        .def_readwrite("faceno", &opensd::Face::faceno)
        .def_readwrite("velocity", &opensd::Face::velocity)
        .def_readwrite("stemp_gues", &opensd::Face::stemp_gues)
        .def_readwrite("ther_gues", &opensd::Face::ther_gues)
        .def_property_readonly("diameter", [](const opensd::Face& face) {
            auto pface = dynamic_cast<const opensd::PFace*>(&face);
            if (!pface) {
                throw std::runtime_error("diameter is only available for pipe faces");
            }
            return pface->diameter;
        });

    py::class_<opensd::PFace, opensd::Face, std::shared_ptr<opensd::PFace>>(m, "PFace")
        .def_readwrite("diameter", &opensd::PFace::diameter);
    //     .def_readwrite("ther_gues", &opensd::PFace::ther_gues);

    // ---------------------------
    // SNode binding
    // ---------------------------
    py::class_<opensd::SNode, std::shared_ptr<opensd::SNode>>(m, "SNode")
        // .def_readwrite("identifier", &opensd::SNode::identifier)
        .def_readwrite("temp_gues", &opensd::SNode::temp_gues);
}
