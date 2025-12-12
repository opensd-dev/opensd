from pathlib import Path
import lxml.etree as ET
from opensd.circuit import Circuit
from opensd.hslab import HSlab
from opensd.checkvalue import PathLike
from ._xml import clean_indentation,reorder_attributes

class InitialGuess:
    def __init__(self, geometry, conditions):
        # Compute initial fields from geometry + BCs
        for item in geometry:
            if type(item)==Circuit:
                circuit = item
                circuit.bcs = [bc for bc in conditions
                               if bc.circuit.identifier == circuit.identifier]
                circuit.get_reference_prop()
            elif type(item) == HSlab:
                hslab = item
                hslab.get_reference_prop()
        self.geometry = geometry

    def export_to_xml(self, path: PathLike = 'initial_guess.xml'):
        """Export initial_guess to an XML file.

        Parameters
        ----------
        path : str
            Path to file to write. Defaults to 'settings.xml'.

        """
        root_element = self.to_xml_element()

        # Check if path is a directory
        p = Path(path)
        if p.is_dir():
            p /= 'initial_guess.xml'

        # Write the XML Tree to the settings.xml file
        tree = ET.ElementTree(root_element)
        tree.write(str(p), xml_declaration=True, encoding='utf-8')

    def to_xml_element(self):
        """Create a 'conditions' element to be written to an XML file.

        """
        element = ET.Element("initial_guess")
        for circuit in sorted(self.geometry, key=lambda x: x.identifier):
            if type(circuit) != Circuit:  # skip hslabs
                continue

            circuit_elem = ET.SubElement(element, "circuit", {
                "identifier": str(circuit.identifier)
            })

            for node in circuit.nodes:
                ET.SubElement(circuit_elem, "node", {
                    "identifier": str(node.identifier),
                    "tpres_old": str(node.tpres_old),
                    "ttemp_old": str(node.ttemp_old),
                    "tenth_old": str(node.tenth_old)
                })

        for hslab in sorted(self.geometry, key=lambda x: x.identifier):
            if type(hslab) != HSlab:
                continue

            # circuit_elem = ET.SubElement(element, "circuit", {
                # "identifier": str(circuit.identifier)
            # })

            # for node in circuit.nodes:
                # ET.SubElement(circuit_elem, "node", {
                    # "identifier": str(node.identifier),
                    # "tpres_old": str(node.tpres_old),
                    # "ttemp_old": str(node.ttemp_old),
                    # "tenth_old": str(node.tenth_old)
                # })

        # Clean the indentation in the file to be user-readable
        clean_indentation(element)
        reorder_attributes(element)

        return element
