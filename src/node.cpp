//! \file node.cpp
#include "opensd/node.h"

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <memory>

#include "opensd/error.h"
#include "opensd/xml_interface.h"
#include "opensd/circuit.h"
#include "opensd/face.h"
#include "opensd/fluid.h"
#include "opensd/coolprop_adapter.h"
#include "opensd/orifice.h"

namespace opensd {

namespace {

double face_mass_flow_gues(const std::shared_ptr<Face>& face) {
  if (face->choked) {
    if (auto orifice = std::dynamic_pointer_cast<Orifice>(face)) {
      return orifice->G * orifice->cfarea * orifice->opening;
    }
    if (auto pface = std::dynamic_pointer_cast<PFace>(face)) {
      const double sign = pface->vflow_gues < 0.0 ? -1.0 : 1.0;
      return sign * pface->Gcr * pface->cfarea;
    }
  }
  return face->ther_gues->rhomass() * face->vflow_gues;
}

} // namespace

//==============================================================================
// Global variables
//==============================================================================

//==============================================================================
// Node implementation
//==============================================================================

Node::Node(pugi::xml_node flnode_node)
{
  if (check_for_node(flnode_node, "identifier")) {
    identifier = get_node_value(flnode_node, "identifier");
    tpres_old  = stod(get_node_value(flnode_node, "tpres_old"));
    ttemp_old  = stod(get_node_value(flnode_node, "ttemp_old"));
    tenth_old  = stod(get_node_value(flnode_node, "tenth_old"));
    volume     = stod(get_node_value(flnode_node, "volume"));
    elevation  = stod(get_node_value(flnode_node, "elevation"));
    msource    = stod(get_node_value(flnode_node, "msource"));
    heat_input = stod(get_node_value(flnode_node, "heat_input"));
    std::string node_type = flnode_node.attribute("type").as_string("");
    is_reservoir = (node_type == "reservoir" || node_type == "tptank");
    is_tptank = (node_type == "tptank");
    height = flnode_node.attribute("height").as_double(0.0);
    cross_area = flnode_node.attribute("cross_area").as_double(0.0);
    tpvolume = flnode_node.attribute("tpvolume").as_double(volume);
    level = flnode_node.attribute("level").as_double(0.0);
    level_old = level;
    if (is_reservoir && tpvolume == 0.0) {
      tpvolume = volume;
    }

    pugi::xml_attribute fixed_var_attr = flnode_node.attribute("fixed_var");
    if (fixed_var_attr) {
      std::string fixed_var_str = fixed_var_attr.value();
      std::istringstream ss(fixed_var_str);
      std::string item;
      while (std::getline(ss, item, ',')) {
        fixed_var.insert(item);
      }
    }

  } else {
    fatal_error("Must specify identifier of flow node in geometry XML file.");
  }
  esource = 0.0;
  hresidue = 0.0;
  mresidue = 0.;
  mflow_in = 1.E-4;
  mflow_out = 0.0;
}


/* Node::Node(std::string identifier, double volume, double heat_input, double elevation, double tpres_old, double ttemp_old, double tenth_old) //Circuit* circuit, 
  : identifier(identifier), volume(volume), heat_input(heat_input), elevation(elevation), tpres_old(tpres_old), ttemp_old(ttemp_old), tenth_old(tenth_old) {
  // node_ind = circuit.size();
  // if (tpres_old != 0.0)
    // this->tpres_old = tpres_old;
  // if (ttemp_old != 0.0)
    // this->ttemp_old = ttemp_old;
  // if (tenth_old != 0.0)
    // this->tenth_old = tenth_old;

  esource = 0.0;
  mflow_in = 1.0E-4;
  msource = 0.0;
  hresidue = 0.0;
  mresidue = 0.0;
} */



double Node::eqn_cont(double time, double delt, bool trans_sim, double alpha_mom) {
  double isum_gues = 0.;
  double isum_old = 0.;
  for (const auto& iface : ifaces) {
    isum_gues += face_mass_flow_gues(iface);
    isum_old += iface->ther_old->rhomass() * iface->vflow_old;
  }
  double osum_gues = 0.;
  double osum_old = 0.;
  for (const auto& oface : ofaces) {
    osum_gues += face_mass_flow_gues(oface);
    osum_old += oface->ther_old->rhomass() * oface->vflow_old;
  }

  mflow_in = isum_gues;
  mflow_out = osum_gues;


  double B, D;
  if (circuit->fltype != FluidType::INCOMPRESSIBLE && ther_old->phase() == 6) {
    B = B1 + volume * ther_old->first_two_phase_deriv(CoolProp::iDmass, CoolProp::iP, CoolProp::iHmass) / ther_old->rhomass();
    D = volume * ther_old->first_two_phase_deriv(CoolProp::iDmass, CoolProp::iHmass, CoolProp::iP);
  } else {
    B = B1 + volume * ther_old->first_partial_deriv(CoolProp::iDmass, CoolProp::iP, CoolProp::iHmass) / ther_old->rhomass();
    D = volume * ther_old->first_partial_deriv(CoolProp::iDmass, CoolProp::iHmass, CoolProp::iP);
  }

  double trans3 = trans_sim * ther_old->rhomass() * B * (spres_gues - spres_old) / delt;
  double trans4 = trans_sim * D * (senth_gues - senth_old) / delt;

  double y = (trans3 + trans4 + alpha_mom * (osum_gues - isum_gues) + (1. - alpha_mom) * (osum_old - isum_old) - msource);

  return y;
}

double Node::eqn_ener(double time, double delt, bool trans_sim, double alpha_ener) {
  // initialize heat input terms
  _heat_input_esource = 0.0;
  _heat_input_msource = msource * tenth_gues;

  // handle fixed_var logic (assumes fixed_var is std::string)
  if (fixed_var.count("T") || fixed_var.count("H")) {
    _heat_input_esource = esource;
  } 
  else if (fixed_var.count("msource") || fixed_var.count("P")) {
    if (msource > 1.e-6) {
      _heat_input_msource = msource * tenth_old;
    }
  }

  double isum_gues = 0.0;
  double isum_old  = 0.0;
  double isum_gues2 = 0.0;
  double isum_old2  = 0.0;
  for (const auto& iface : ifaces) {
    double mflow_gues = face_mass_flow_gues(iface);
    double mflow_old = iface->ther_old->rhomass() * iface->vflow_old;

    double up_contrib_gues = iface->upstream->tenth_gues * std::max(mflow_gues, 0.0);
    double down_contrib_gues = iface->downstream->tenth_gues * std::max(-mflow_gues, 0.0);
    isum_gues += (up_contrib_gues - down_contrib_gues);

    double up_contrib_old = iface->upstream->tenth_old * std::max(mflow_old, 0.0);
    double down_contrib_old = iface->downstream->tenth_old * std::max(-mflow_old, 0.0);
    isum_old += (up_contrib_old - down_contrib_old);

    isum_gues2 += mflow_gues;
    isum_old2  += mflow_old;

  }


  double osum_gues = 0.0;
  double osum_old  = 0.0;
  double osum_gues2 = 0.0;
  double osum_old2  = 0.0;

  for (const auto& oface : ofaces) {
    double mflow_gues = face_mass_flow_gues(oface);
    double mflow_old = oface->ther_old->rhomass() * oface->vflow_old;

    double up_contrib_gues = oface->upstream->tenth_gues * std::max(mflow_gues, 0.0);
    double down_contrib_gues = oface->downstream->tenth_gues * std::max(-mflow_gues, 0.0);
    osum_gues += (up_contrib_gues - down_contrib_gues);

    double up_contrib_old = oface->upstream->tenth_old * std::max(mflow_old, 0.0);
    double down_contrib_old = oface->downstream->tenth_old * std::max(-mflow_old, 0.0);
    osum_old += (up_contrib_old - down_contrib_old);

    osum_gues2 += mflow_gues;
    osum_old2  += mflow_old;
  }

  // weighted face convective heat input (alpha_ener weighting)
  _heat_input_faceconv  = alpha_ener * (isum_gues - osum_gues) + (1.0 - alpha_ener) * (isum_old - osum_old);
  _heat_input_faceconv2 = alpha_ener * (osum_gues2 - isum_gues2) + (1.0 - alpha_ener) * (osum_old2 - isum_old2) - msource;


  // -------- face heat generation / slab heat contributions ----------
  isum_gues = 0.0;
  isum_old  = 0.0;

  for (const auto& iface : ifaces) {
    // (iface.heat_input + sum(iface.heat_hslab)) * max(sign(iface.vflow_gues),0)
    double sum_hslab_gues = std::accumulate(iface->heat_hslab.begin(), iface->heat_hslab.end(), 0.0);
    double sum_hslab_old  = std::accumulate(iface->heat_hslab_old.begin(), iface->heat_hslab_old.end(), 0.0);

    double pos_gues = (iface->vflow_gues > 0.0 ? 1.0 : 0.0);
    double pos_old  = (iface->vflow_old  > 0.0 ? 1.0 : 0.0);

    isum_gues += (iface->heat_input + sum_hslab_gues) * pos_gues;
    isum_old  += (iface->heat_input_old + sum_hslab_old) * pos_old;
  }
  osum_gues = 0.0;
  osum_old  = 0.0;
  for (const auto& oface : ofaces) {

    double sum_hslab_gues = std::accumulate(oface->heat_hslab.begin(), oface->heat_hslab.end(), 0.0);
    double sum_hslab_old  = std::accumulate(oface->heat_hslab_old.begin(), oface->heat_hslab_old.end(), 0.0);

    // uses max(np.sign(-oface.vflow_gues),0) <-- this is 1 when oface.vflow_gues < 0
    double negpos_gues = (oface->vflow_gues < 0.0 ? 1.0 : 0.0);
    double negpos_old  = (oface->vflow_old  < 0.0 ? 1.0 : 0.0);

    osum_gues += (oface->heat_input + sum_hslab_gues) * negpos_gues;
    osum_old  += (oface->heat_input_old + sum_hslab_old) * negpos_old;
  }
  _heat_input_facegen = alpha_ener * (isum_gues + osum_gues) + (1.0 - alpha_ener) * (isum_old + osum_old);

  // -------- transient and storage terms ----------
  double C = volume * ther_old->rhomass();
  double E = volume;

  double trans1 = trans_sim * C * (tenth_gues - tenth_old) / delt;
  double trans2 = trans_sim * E * (spres_gues - spres_old) / delt;

  double y = trans1
           - trans2
           - _heat_input_faceconv
           - _heat_input_faceconv2 * tenth_old * (trans_sim ? 1.0 : 0.0)
           - _heat_input_msource
           - heat_input
           - std::accumulate(heat_hslab.begin(), heat_hslab.end(), 0.0)
           - _heat_input_facegen
           - _heat_input_esource;

// if (identifier == "node3") {
//   std::cout << "flag1 " << identifier << std::endl
//             << "  trans1 = " << trans1 << std::endl
//             << "  trans2 = " << trans2 << std::endl
//             << "  _heat_input_faceconv = " << _heat_input_faceconv << std::endl
//             << "  _heat_input_faceconv2_term = "
//             << (_heat_input_faceconv2 * tenth_old * (trans_sim ? 1.0 : 0.0)) << std::endl
//             << "  _heat_input_msource = " << _heat_input_msource << std::endl
//             << "  heat_input = " << heat_input << std::endl
//             << "  heat_hslab_sum = " << std::accumulate(heat_hslab.begin(), heat_hslab.end(), 0.0) << std::endl
//             << "  _heat_input_facegen = " << _heat_input_facegen << std::endl
//             << "  _heat_input_esource = " << _heat_input_esource << std::endl
//             << "  y = " << y << std::endl;
// }

  return y;

}

void Node::update_gues() {
  tpres_gues = tpres_old;
  ttemp_gues = ttemp_old;
  spres_gues = spres_old;
  stemp_gues = stemp_old;
  tenth_gues = tenth_old;
  senth_gues = senth_old;
  ther_gues->update(CoolProp::HmassP_INPUTS,senth_gues,spres_gues);
  if (is_tptank) {
    update_sat(spres_gues);
  }

  double c1 = 5./4. - 0.26; //self.mech_gues.poissons_ratio()
  double youngs_modulus = 1.E11; 
  double isum = 0.0;
  for (const auto& iface : ifaces) {
	  auto pface = std::dynamic_pointer_cast<PFace>(iface);
	  if (pface) {
      // if (pface->has_wall()) {  // Assuming has_wall() is a method that checks for wall existence
          isum += pface->diameter * (pface->delx * pface->cfarea) * c1 //pending may not applicable only for shell side flow (add warning)
                  / (2.0 * 0.019 * youngs_modulus); //pface->wall->thk
      // }
	}
  }
  
  double osum = 0.0;
  for (const auto& oface : ofaces) {
  	  auto pface = std::dynamic_pointer_cast<PFace>(oface);
	  if (pface) {

      // if (pface->has_wall()) {
          osum += pface->diameter * (pface->delx * pface->cfarea) * c1 //pface->wall->c1
                  / (2.0 * 0.019 * youngs_modulus); // pface->wall->thk //pface->wall->mech_gues->youngs_modulus()
      // }
	  }
  }
  
  B1 = isum + osum;



}

void Node::assign_staticvar() {
  spres_old = tpres_old;
  stemp_old = ttemp_old;
  senth_old = tenth_old;
  velocity = 0.;
}

void Node::update_statictemp() {
  stemp_gues = ttemp_gues - 0.5*velocity*velocity/ther_gues->cpmass();
}

void Node::update_totalenth() {
  tenth_gues = senth_gues + 0.5*velocity*velocity;
}

void Node::update_totaltemp() {
  ttemp_gues = stemp_gues + 0.5*velocity*velocity/ther_gues->cpmass();
}

void Node::update_staticenth() {
  senth_gues = tenth_gues - 0.5*velocity*velocity;
}

void Node::update_staticpres() {
  spres_gues = tpres_gues - 0.5*ther_gues->rhomass()*velocity*velocity; //slug pending
}

void Node::assign_prop() {
	if (circuit->fltype != FluidType::INCOMPRESSIBLE) {
  ther_gues = std::make_shared<opensd::CoolPropAdapter>("BICUBIC&HEOS", circuit->flname);
  ther_old  = std::make_shared<opensd::CoolPropAdapter>("BICUBIC&HEOS", circuit->flname);
	}
	else {
      if (circuit->fllib=="CoolProp") {
  ther_gues = std::make_shared<opensd::CoolPropAdapter>("INCOMP", circuit->flname);
  ther_old = std::make_shared<opensd::CoolPropAdapter>("INCOMP", circuit->flname);
  // ther_old  = node->ther_gues->clone();
      } else { //circuit->fllib=="User"

  bool found = false;
  for (auto& fptr : model::fluids) {
    if (fptr->name() == circuit->flname) {
      ther_gues = fptr->clone(); // deep copy
      ther_old  = fptr->clone();
      found = true;
      break;
    }
  }
  if (!found) fatal_error(fmt::format("Could not find fluid '{}'", circuit->flname));


      }


    }
  ther_old->update(CoolProp::HmassP_INPUTS,senth_old,spres_old);
  if (is_tptank) {
    update_sat(spres_old);
    const int ph = ther_old->phase();
    if (ph == 0) {
      volfracliq = 1.0;
      level = height;
      watermass = ther_old->rhomass() * tpvolume;
    } else if (ph == 5) {
      volfracliq = 0.0;
      level = 0.0;
      watermass = 0.0;
    } else if (ph == 6) {
      volfracliq = 1.0 - ther_old->Qth() * ther_old->rhomass() / rhog;
      if (cross_area > 0.0) {
        level = volfracliq * tpvolume / cross_area;
      }
      watermass = ther_old->rhomass() * tpvolume;
    }
    level_old = level;
  }
}

void Node::update_staticvar(std::optional<double> velocity_in) {
  // if (pressure < 0) {
    double pressure = tpres_gues;
  // }

  if (velocity_in.has_value()) {
    // directly use provided velocity
    velocity = velocity_in.value();
  } else if (is_reservoir) {
    velocity = 0.0;
  } else {
  if (std::find(fixed_var.begin(), fixed_var.end(), "P") != fixed_var.end()) {
    velocity = 0.0;
  } else {
    double sum_velocity = 0.0;
    int count = 0;
    for (const auto& face : ifaces) {
      sum_velocity += face->velocity;
      count++;
    }
    for (const auto& face : ofaces) {
      sum_velocity += face->velocity;
      count++;
    }
    if (count > 0) {
      velocity = sum_velocity / count;
    } else {
      velocity = 0.0;
    }
  }
  }
  spres_gues = pressure - 0.5 * ther_gues->rhomass() * velocity * velocity;
  stemp_gues = ttemp_gues - 0.5 * velocity * velocity / ther_gues->cpmass();
  senth_gues = tenth_gues - 0.5 * velocity * velocity;

  if (spres_gues < 0.0) {
    std::cerr << "Warning: negative spres in flow_components in update_staticvar. Zero velocity assumed: "
              << identifier << " " << tpres_gues << " " << velocity << " "
              << ther_gues->rhomass() << " " << 0.5 * ther_gues->rhomass() * velocity * velocity << std::endl;
    spres_gues = pressure;
    stemp_gues = ttemp_gues;
    senth_gues = tenth_gues;
  }
  if (is_tptank) {
    update_sat(spres_gues);
  }
}

void Node::update_old() {
  if (is_tptank) {
    update_sat(spres_gues);
    update_level();
  }

  tpres_old = tpres_gues;
  ttemp_old = ttemp_gues;
  spres_old = spres_gues;
  stemp_old = stemp_gues;
  tenth_old = tenth_gues;
  senth_old = senth_gues;

  ther_old->update(CoolProp::HmassP_INPUTS,senth_old,spres_old);
  if (is_tptank) {
    level_old = level;
  }

  // if (circuit->flag_tp || dynamic_cast<TPTank*>(this)) {
  //   ther_old.update_sat();
  // }

}

void Node::update_sat(double pressure) {
  if (!is_tptank || circuit == nullptr || circuit->fltype == FluidType::INCOMPRESSIBLE) {
    return;
  }
  if (pressure < 0.0) {
    pressure = spres_gues;
  }

  try {
    auto flstate = std::unique_ptr<CoolProp::AbstractState>(
      CoolProp::AbstractState::factory("BICUBIC&HEOS", circuit->flname));
    flstate->update(CoolProp::PQ_INPUTS, pressure, 0.0);
    Tsat = flstate->T();
    hf = flstate->hmass();
    rhof = flstate->rhomass();
    muf = flstate->viscosity();
    cpf = flstate->cpmass();
    kf = flstate->conductivity();
    flstate->update(CoolProp::PQ_INPUTS, pressure, 1.0);
    hg = flstate->hmass();
    rhog = flstate->rhomass();
    mug = flstate->viscosity();
    cpg = flstate->cpmass();
    kg = flstate->conductivity();
  } catch (const CoolProp::CoolPropBaseError&) {
    auto flstate = std::unique_ptr<CoolProp::AbstractState>(
      CoolProp::AbstractState::factory("HEOS", circuit->flname));
    flstate->update(CoolProp::PQ_INPUTS, pressure, 0.0);
    Tsat = flstate->T();
    hf = flstate->hmass();
    rhof = flstate->rhomass();
    muf = flstate->viscosity();
    cpf = flstate->cpmass();
    kf = flstate->conductivity();
    flstate->update(CoolProp::PQ_INPUTS, pressure, 1.0);
    hg = flstate->hmass();
    rhog = flstate->rhomass();
    mug = flstate->viscosity();
    cpg = flstate->cpmass();
    kg = flstate->conductivity();
  }
}

void Node::update_level() {
  if (!is_tptank) {
    return;
  }

  const int ph = ther_gues->phase();
  if (ph == 0) {
    volfracliq = 1.0;
    level = height;
    watermass = ther_gues->rhomass() * tpvolume;
  } else if (ph == 5) {
    volfracliq = 0.0;
    level = 0.0;
    watermass = 0.0;
  } else if (ph == 6) {
    if (rhog <= 0.0) {
      update_sat(spres_gues);
    }
    volfracliq = 1.0 - ther_gues->Qth() * ther_gues->rhomass() / rhog;
    if (cross_area > 0.0) {
      level = volfracliq * tpvolume / cross_area;
    }
    watermass = ther_gues->rhomass() * tpvolume;
  }
}


void Node::save_to_hdf5(hid_t group_id) const {
  H5LTset_attribute_string(group_id, ".", "identifier", identifier.c_str());

  const double stored_volume = is_tptank ? tpvolume : volume;
  H5LTset_attribute_double(group_id, ".", "volume", &stored_volume, 1);
  H5LTset_attribute_double(group_id, ".", "heat_input", &heat_input, 1);
  H5LTset_attribute_double(group_id, ".", "elevation", &elevation, 1);
  H5LTset_attribute_double(group_id, ".", "tpres_gues", &tpres_gues, 1);
  H5LTset_attribute_double(group_id, ".", "spres_gues", &spres_gues, 1);
  H5LTset_attribute_double(group_id, ".", "ttemp_gues", &ttemp_gues, 1);
  H5LTset_attribute_double(group_id, ".", "stemp_gues", &stemp_gues, 1);
  H5LTset_attribute_double(group_id, ".", "tenth_gues", &tenth_gues, 1);
  H5LTset_attribute_double(group_id, ".", "senth_gues", &senth_gues, 1);
  H5LTset_attribute_double(group_id, ".", "velocity", &velocity, 1);
  H5LTset_attribute_double(group_id, ".", "msource", &msource, 1);
}

void Node::load_from_hdf5(hid_t group_id) {
  identifier = opensd::read_string_attribute(group_id, "identifier");
  volume = opensd::read_double_attribute(group_id, "volume");
  if (is_tptank) {
    volume = tpvolume;
  }
  heat_input = opensd::read_double_attribute(group_id, "heat_input");
  elevation = opensd::read_double_attribute(group_id, "elevation");
  tpres_old = opensd::read_double_attribute(group_id, "tpres_gues");
  spres_old = opensd::read_double_attribute(group_id, "spres_gues");
  ttemp_old = H5Aexists(group_id, "ttemp_gues") > 0
    ? opensd::read_double_attribute(group_id, "ttemp_gues")
    : ttemp_old;
  stemp_old = H5Aexists(group_id, "stemp_gues") > 0
    ? opensd::read_double_attribute(group_id, "stemp_gues")
    : stemp_old;
  tenth_old = opensd::read_double_attribute(group_id, "tenth_gues");
  senth_old = opensd::read_double_attribute(group_id, "senth_gues");
  velocity = opensd::read_double_attribute(group_id, "velocity");
  msource = opensd::read_double_attribute(group_id, "msource");
}

} // namespace opensd
