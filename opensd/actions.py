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

    @staticmethod
    def _target_id(target):
        if target is None:
            return ""
        if isinstance(target, str):
            return target
        if hasattr(target, "identifier"):
            return str(target.identifier)
        if hasattr(target, "hslab") and hasattr(target.hslab, "identifier") and hasattr(target, "layerno"):
            return f"{target.hslab.identifier}.layer{target.layerno}"
        if callable(target) and hasattr(target, "__name__"):
            return target.__name__
        return str(target)

    @staticmethod
    def _as_list(value):
        if isinstance(value, (list, tuple)):
            return list(value)
        return [value]

    @classmethod
    def _expand_targets(cls, targets, variables):
        target_groups = cls._as_list(targets)
        variable_groups = cls._as_list(variables)

        if len(variable_groups) == 1 and len(target_groups) > 1:
            variable_groups = variable_groups * len(target_groups)
        if len(target_groups) != len(variable_groups):
            raise ValueError("Action target and variable lists must have the same length")

        expanded = []
        for target, variable in zip(target_groups, variable_groups):
            if isinstance(target, (list, tuple)):
                for item in target:
                    expanded.append((cls._target_id(item), str(variable)))
            else:
                expanded.append((cls._target_id(target), str(variable)))
        return expanded

    def to_xml_element(self, element):
        """
        Converts this Action object to an XML subelement.
        """
        if not self.enabled:
            return None

        subelement = ET.SubElement(element, "action")
        subelement.set("identifier", self.identifier)

        for target, variable in self._expand_targets(self.target, self.variable):
            target_elem = ET.SubElement(subelement, "target")
            target_elem.set("id", target)
            target_elem.set("variable", variable)

        if isinstance(self.distribution, str):
            func_elem = ET.SubElement(subelement, "function")
            func_elem.set("name", self.distribution)
        else:
            if len(subelement.findall("target")) == 1:
                target_elem = subelement.find("target")
                subelement.set("target", target_elem.get("id"))
                subelement.set("variable", target_elem.get("variable"))
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
