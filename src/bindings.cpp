// bindings.cpp
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>  // for std::vector
#include <memory>
#include <stdexcept>
#include "opensd/face.h"
#include "opensd/snode.h"
#include "opensd/facether.h"
#include "opensd/circuit.h"
#include "opensd/hslab.h"
#include "opensd/layer.h"

namespace py = pybind11;

PYBIND11_MODULE(bindings, m) {

    // ---------------------------
    // FaceTher binding
    // ---------------------------
    py::class_<opensd::FaceTher>(m, "FaceTher")
        .def("rhomass", &opensd::FaceTher::rhomass)
        .def("cpmass", &opensd::FaceTher::cpmass)
        .def("conductivity", &opensd::FaceTher::conductivity)
        .def("viscosity", &opensd::FaceTher::viscosity);

    // ---------------------------
    // Face binding
    // ---------------------------
    py::class_<opensd::Face, std::shared_ptr<opensd::Face>>(m, "Face")
        .def_readwrite("faceno", &opensd::Face::faceno)
        .def_readwrite("velocity", &opensd::Face::velocity)
        .def_readwrite("mflow", &opensd::Face::mflow)
        .def_readwrite("unode", &opensd::Face::unode)
        .def_readwrite("dnode", &opensd::Face::dnode)
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

    py::class_<opensd::Node, std::shared_ptr<opensd::Node>>(m, "Node")
        .def_readwrite("identifier", &opensd::Node::identifier)
        .def_readwrite("stemp_gues", &opensd::Node::stemp_gues)
        .def_readwrite("ttemp_gues", &opensd::Node::ttemp_gues)
        .def_readwrite("tpres_gues", &opensd::Node::tpres_gues)
        .def_readwrite("spres_gues", &opensd::Node::spres_gues)
        .def_readwrite("tenth_gues", &opensd::Node::tenth_gues)
        .def_readwrite("senth_gues", &opensd::Node::senth_gues)
        .def_readwrite("msource", &opensd::Node::msource)
        .def_readwrite("esource", &opensd::Node::esource)
        .def_readwrite("level", &opensd::Node::level);

    // ---------------------------
    // SNode binding
    // ---------------------------
    py::class_<opensd::SNode, std::shared_ptr<opensd::SNode>>(m, "SNode")
        .def_readwrite("identifier", &opensd::SNode::identifier)
        .def_readwrite("temp_gues", &opensd::SNode::temp_gues)
        .def_readwrite("eface", &opensd::SNode::eface)
        .def_readwrite("wface", &opensd::SNode::wface);

    py::class_<opensd::SFace, std::shared_ptr<opensd::SFace>>(m, "SFace")
        .def_readwrite("temp_gues", &opensd::SFace::temp_gues);

    py::class_<opensd::Layer, std::shared_ptr<opensd::Layer>>(m, "Layer")
        .def_readwrite("layerno", &opensd::Layer::layerno)
        .def_readwrite("heat_input", &opensd::Layer::heat_input)
        .def_property_readonly("nodes", [](opensd::Layer& layer) {
            return layer.snodes;
        }, py::return_value_policy::reference_internal)
        .def_readwrite("snodes", &opensd::Layer::snodes);

    py::class_<opensd::HSlab, std::shared_ptr<opensd::HSlab>>(m, "HSlab")
        .def_readwrite("identifier", &opensd::HSlab::identifier)
        .def_readwrite("layers", &opensd::HSlab::layers);

    py::class_<opensd::Pipe, std::shared_ptr<opensd::Pipe>>(m, "Pipe")
        .def_readwrite("identifier", &opensd::Pipe::identifier)
        .def_readwrite("heat_input", &opensd::Pipe::heat_input)
        .def_readwrite("mflow", &opensd::Pipe::mflow)
        .def_readwrite("faces", &opensd::Pipe::faces)
        .def_readwrite("dnode", &opensd::Pipe::dnode)
        .def_readwrite("unode", &opensd::Pipe::unode);

    m.def("get_comp", [](const std::string& id) -> py::object {
        for (auto& circuit : opensd::model::circuits) {
            for (auto& pipe : circuit->pipes) {
                if (pipe->identifier == id) return py::cast(pipe);
            }
            for (auto& node : circuit->nodes) {
                if (node->identifier == id) return py::cast(node);
            }
        }

        for (auto& hslab : opensd::model::hslabs) {
            if (hslab->identifier == id) return py::cast(hslab);
        }

        throw std::runtime_error("Object not found in project: " + id);
    });
}
