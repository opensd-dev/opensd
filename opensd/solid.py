import xml.etree.ElementTree as ET
from collections.abc import Iterable
import opensd.checkvalue as cv
from pathlib import Path
from .mixin import IDManagerMixin
from ._xml import clean_indentation

class Solid(IDManagerMixin):
    """A user-defined solid.

    To create a solid, one should create an instance of this class, and set the properties like :meth:Solid.set_density(). The fluid can then be assigned to a heatslab layer using the
    :attr:`hslab1.add_layer(...,solname="SS6",sollib="User")` method.

    Parameters
    ----------
    solid_id : int, optional
        Unique identifier for the solid. If not specified, an identifier will
        automatically be assigned.
    name : str, optional
        Name of the solid. If not specified, the name will be automatically assigned.

    Attributes
    ----------
    id : int
        Unique identifier for the solid
    cpmass : float
        Specific heat of the solid in J/kg-K.
    rhomass : float
        Density of the solid.

    """

    # _id_counter = 1
    next_id = 1
    used_ids = set()

    def __init__(self,solid_id=None, name=''):
        self.id = solid_id
        # self.id = Solid._id_counter
        # Solid._id_counter += 1

        # self.name = name if name else f"fluid_{self.id}"
        self.name = name

        # Solid properties
        self.cpmass = None
        self.rhomass = None
        self.conductivity = None

    def to_xml_element(self):
        """Return XML element for this solid."""
        element = ET.Element("solid", attrib={"id": str(self.id)})
        if self.name:
            element.set("name", str(self.name))

        def add(tag, value, unit):
            if value is not None:
                subelement = ET.SubElement(element, tag)
                subelement.set("value", str(value))
                subelement.set("units", unit)

        add("cpmass", self.cpmass, "J/kg-K")
        add("rhomass", self.rhomass, "kg/m3")
        add("conductivity", self.conductivity, "W/m-K")

        return element

class Solids(cv.CheckedList):
    """Collection of Solids used for an OpenSD simulation.

    This class corresponds directly to the solids.xml input file. It can be thought of as a normal Python list where each member is a :class:Solid.
    It behaves like a list as the following example demonstrates:

    >>> SS6 = opensd.Solid()
    >>> CS7 = opensd.Solid()
    >>> MS8 = opensd.Solid()
    >>> solids = opensd.Solids([SS6])
    >>> solids.append(CS7)
    >>> solids += [MS8]

    Parameters
    ----------
    solids : Iterable of opensd.Solid
        Solids to add to the collection

    """

    def __init__(self, solids=None):
        super().__init__(Solid, 'Solids Collection')
        if solids is not None:
            self += solids

    def append(self, solid):
        """Append solid to collection

        Parameters
        ----------
        solid : opensd.Solid
            Solid to append

        """
        super().append(solid)

    def insert(self, index: int, solid):
        """Insert soild before index

        Parameters
        ----------
        index : int
            Index in list
        solid : opensd.Solid
            Solid to insert

        """
        super().insert(index, solid)

    def _write_xml(self, file, header=True, level=0, spaces_per_level=2,
                   trailing_indent=True):
        """Write XML content of solids collection.

        Parameters
        ----------
        file : IOTextWrapper
            Open file handle to write content into.
        header : bool
            Whether or not to write the XML header
        level : int
            Indentation level of solids element
        spaces_per_level : int
            Number of spaces per indentation
        trailing_indentation : bool
            Whether or not to write a trailing indentation for the solids element

        """

        indentation = level * spaces_per_level * ' '
        if header:
            file.write("<?xml version='1.0' encoding='utf-8'?>\n")
        file.write(indentation + '<solids>\n')

        # for fluid in self:
        for fluid in sorted(set(self), key=lambda x: x.id):
            elem = fluid.to_xml_element()
            clean_indentation(elem, level=level+1)
            elem.tail = elem.tail.strip(' ')
            file.write((level + 1) * spaces_per_level * ' ')
            file.write(ET.tostring(elem, encoding='unicode'))
            # file.write('\n')

        file.write(indentation + '</solids>\n')
        if trailing_indent:
            file.write(indentation)

    def export_to_xml(self, path='solids.xml'):
        """Export the solids collection to an XML file.

        Parameters
        ----------
        path : str
            Path to file to write. Defaults to 'solids.xml'.

        """
        # Check if path is a directory
        p = Path(path)
        if p.is_dir():
            p /= 'solids.xml'

        # Write solids to the file one-at-a-time.  This significantly reduces
        # memory demand over allocating a complete ElementTree and writing it in
        # one go.
        with open(str(p), 'w', encoding='utf-8',
                  errors='xmlcharrefreplace') as fh:
            self._write_xml(fh)
        print(f"✅ Exported {len(self)} solids to '{path}'")
