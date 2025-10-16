import xml.etree.ElementTree as ET
from collections.abc import Iterable
import opensd.checkvalue as cv
from pathlib import Path
from .mixin import IDManagerMixin
from ._xml import clean_indentation

class Fluid(IDManagerMixin):
    """A user-defined fluid.

    To create a fluid, one should create an instance of this class, add trace elements if present and set the properties like :meth:Fluid.set_density(). The fluid can then be assigned to a circuit using the
    :attr:`Circuit.assign_fluid` method.

    Parameters
    ----------
    fluid_id : int, optional
        Unique identifier for the fluid. If not specified, an identifier will
        automatically be assigned.
    name : str, optional
        Name of the fluid. If not specified, the name will be automatically assigned.

    Attributes
    ----------
    id : int
        Unique identifier for the fluid
    cpmass : float
        Specific heat at constant pressure of the fluid in J/kg-K.
    cvmass : float
        Specific heat at constant volume of the fluid.
    rhomass : float
        Density of the fluid.
    molarmass: float
        Molar mass of the fluid.

    """

    # _id_counter = 1
    next_id = 1
    used_ids = set()

    def __init__(self,fluid_id=None, name=''):
        self.id = fluid_id
        # self.id = Fluid._id_counter
        # Fluid._id_counter += 1

        # self.name = name if name else f"fluid_{self.id}"
        self.name = name

        # Fluid properties
        self.cpmass = None
        self.cvmass = None
        self.rhomass = None
        self.molarmass = None
        self.viscosity = None
        self.conductivity = None
        self.adiabatic_compressibility = None
        self.isothermal_compressibility = None
        self.boiling_point = None
        self.enthalpy_vaporization = None

    def to_xml_element(self):
        """Return XML element for this fluid."""
        element = ET.Element("fluid", attrib={"id": str(self.id)})
        if self.name:
            element.set("name", str(self.name))

        def add(tag, value, unit):
            if value is not None:
                subelement = ET.SubElement(element, tag)
                subelement.set("value", str(value))
                subelement.set("units", unit)

        add("cpmass", self.cpmass, "J/kg-K")
        add("cvmass", self.cvmass, "J/kg-K")
        add("rhomass", self.rhomass, "kg/m3")
        add("molarmass", self.molarmass, "kg/mol")
        add("viscosity", self.viscosity, "Pa-s")
        add("conductivity", self.conductivity, "W/m-K")
        add("adiabatic_compressibility", self.adiabatic_compressibility, "1/Pa")
        add("isothermal_compressibility", self.isothermal_compressibility, "1/Pa")
        add("boiling_point", self.boiling_point, "K")
        add("enthalpy_vaporization", self.enthalpy_vaporization, "J/kg")

        return element

class Fluids(cv.CheckedList):
    """Collection of Fluids used for an OpenSD simulation.

    This class corresponds directly to the fluids.xml input file. It can be thought of as a normal Python list where each member is a :class:Fluid.
    It behaves like a list as the following example demonstrates:

    >>> Na6 = opensd.Fluid()
    >>> Na7 = opensd.Fluid()
    >>> Na8 = opensd.Fluid()
    >>> fluids = opensd.Fluids([Na6])
    >>> fluids.append(Na7)
    >>> fluids += [Na8]

    Parameters
    ----------
    fluids : Iterable of opensd.Fluid
        Fluids to add to the collection

    """

    def __init__(self, fluids=None):
        super().__init__(Fluid, 'Fluids Collection')
        if fluids is not None:
            self += fluids

    def append(self, fluid):
        """Append fluid to collection

        Parameters
        ----------
        fluid : opensd.Fluid
            Fluid to append

        """
        super().append(fluid)

    def insert(self, index: int, fluid):
        """Insert fluid before index

        Parameters
        ----------
        index : int
            Index in list
        fluid : opensd.Fluid
            Fluid to insert

        """
        super().insert(index, fluid)

    def _write_xml(self, file, header=True, level=0, spaces_per_level=2,
                   trailing_indent=True):
        """Write XML content of fluids collection.

        Parameters
        ----------
        file : IOTextWrapper
            Open file handle to write content into.
        header : bool
            Whether or not to write the XML header
        level : int
            Indentation level of fluids element
        spaces_per_level : int
            Number of spaces per indentation
        trailing_indentation : bool
            Whether or not to write a trailing indentation for the fluids element

        """

        indentation = level * spaces_per_level * ' '
        if header:
            file.write("<?xml version='1.0' encoding='utf-8'?>\n")
        file.write(indentation + '<fluids>\n')

        # for fluid in self:
        for fluid in sorted(set(self), key=lambda x: x.id):
            elem = fluid.to_xml_element()
            clean_indentation(elem, level=level+1)
            elem.tail = elem.tail.strip(' ')
            file.write((level + 1) * spaces_per_level * ' ')
            file.write(ET.tostring(elem, encoding='unicode'))
            # file.write('\n')

        file.write(indentation + '</fluids>\n')
        if trailing_indent:
            file.write(indentation)

    def export_to_xml(self, path='fluids.xml'):
        """Export the fluids collection to an XML file.

        Parameters
        ----------
        path : str
            Path to file to write. Defaults to 'fluids.xml'.

        """
        # Check if path is a directory
        p = Path(path)
        if p.is_dir():
            p /= 'fluids.xml'

        # Write fluids to the file one-at-a-time.  This significantly reduces
        # memory demand over allocating a complete ElementTree and writing it in
        # one go.
        with open(str(p), 'w', encoding='utf-8',
                  errors='xmlcharrefreplace') as fh:
            self._write_xml(fh)
        print(f"✅ Exported {len(self)} fluids to '{path}'")
