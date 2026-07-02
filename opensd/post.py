from pathlib import Path
import lxml.etree as ET

from opensd.checkvalue import PathLike
from ._xml import clean_indentation, reorder_attributes


class Calculate(object):
    def __init__(self, function, *components, identifier=None):
        self.function = function
        self.components = list(components)
        self.identifier = identifier or getattr(function, "__name__", str(function))

    def to_xml_element(self, element):
        subelement = ET.Element("calculate") if element is None else ET.SubElement(element, "calculate")
        subelement.set("identifier", self.identifier)
        subelement.set("function", getattr(self.function, "__name__", str(self.function)))
        for component in self.components:
            comp_elem = ET.SubElement(subelement, "component")
            comp_elem.set("id", str(component))
        return subelement


class Post(list):
    def __init__(self, calculations=()):
        super().__init__(calculations)

    def export_to_xml(self, path: PathLike = "post.xml"):
        p = Path(path)
        if p.is_dir():
            p /= "post.xml"

        with open(str(p), "w", encoding="utf-8", errors="xmlcharrefreplace") as fh:
            self._write_xml(fh)

    def _write_xml(self, file, header=True, level=0, spaces_per_level=2,
                   trailing_indent=True):
        indentation = level * spaces_per_level * " "
        if header:
            file.write("<?xml version='1.0' encoding='utf-8'?>\n")
        file.write(indentation + "<post>\n")

        for item in self:
            element = item.to_xml_element(None)
            clean_indentation(element, level=level + 1)
            element.tail = element.tail.strip(" ")
            file.write((level + 1) * spaces_per_level * " ")
            reorder_attributes(element)
            file.write(ET.tostring(element, encoding="unicode"))

        file.write(indentation + "</post>\n")
        if trailing_indent:
            file.write(indentation)
