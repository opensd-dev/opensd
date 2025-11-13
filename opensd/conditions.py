from pathlib import Path
import lxml.etree as ET
from opensd.checkvalue import PathLike
from ._xml import clean_indentation,reorder_attributes

class Conditions(list):
    """A collection of boundary conditions.

    This class corresponds directly to the conditions.xml input file. It can be
    thought of as a normal Python list where each member is :class:`BC`.

    Parameters
    ----------
    items : Iterable of opensd.BC
        Items (circuits or heatslabs) to add to the collection

    """
    def __init__(self, items):
        super().__init__()
        for item in items:
            super().append(item)

    def export_to_xml(self, path: PathLike = 'conditions.xml'):
        """Export conditions to an XML file.

        Parameters
        ----------
        path : str
            Path to file to write. Defaults to 'settings.xml'.

        """
        root_element = self.to_xml_element()

        # Check if path is a directory
        p = Path(path)
        if p.is_dir():
            p /= 'conditions.xml'

        # Write the XML Tree to the settings.xml file
        tree = ET.ElementTree(root_element)
        tree.write(str(p), xml_declaration=True, encoding='utf-8')

    def to_xml_element(self):
        """Create a 'conditions' element to be written to an XML file.

        """
        element = ET.Element("conditions")
        for item in sorted(self, key=lambda x: x.identifier):
            subelement = item.to_xml_element()
            element.append(subelement)

        # Clean the indentation in the file to be user-readable
        clean_indentation(element)
        reorder_attributes(element)

        return element
