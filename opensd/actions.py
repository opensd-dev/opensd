# actions.py
import lxml.etree as ET

class Action(object):
    """
    Represents a time-dependent change to a variable (e.g., BC value, pump speed).
    """

    def __init__(self, identifier, target, variable, distribution, enabled=True):
        """
        Parameters
        ----------
        identifier : str
            Unique identifier for this action
        target : str
            Target object (e.g., bc1, pump1)
        variable : str
            Variable to modify (e.g., 'bval', 'speed')
        distribution : Tabular or similar
            Defines how the variable changes with time
        enabled : bool
            Whether this action is active
        """
        self.identifier = identifier
        self.target = target
        self.variable = variable
        self.distribution = distribution
        self.enabled = enabled

    def to_xml_element(self, element):
        """
        Converts this Action object to an XML subelement.
        """
        if not self.enabled:
            return None

        subelement = ET.SubElement(element, "action")
        subelement.set("identifier", self.identifier)
        subelement.set("target", self.target)
        subelement.set("variable", self.variable)
        self.distribution.to_xml_element(subelement)
        return subelement


class Actions(object):
    """
    Represents the entire actions.xml file, containing all transient definitions.
    """

    def __init__(self, actions):
        """
        Parameters
        ----------
        actions : list[Action]
            List of all defined Action objects
        """
        self.actions = actions

    def export_to_xml(self, filename="actions.xml"):
        """
        Writes the actions to an XML file.
        """
        root = ET.Element("actions")
        for action in self.actions:
            action.to_xml_element(root)

        tree = ET.ElementTree(root)
        tree.write(
            filename,
            pretty_print=True,
            xml_declaration=True,
            encoding="utf-8"
        )
