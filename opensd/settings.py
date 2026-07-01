from enum import Enum
import numpy as np
from numbers import Integral, Real
from pathlib import Path
import lxml.etree as ET
from collections import abc

import opensd.checkvalue as cv
from ._xml import clean_indentation, reorder_attributes
from opensd.checkvalue import PathLike

class RunMode(Enum):
    STEADY = 'steady'
    DESIGN = 'design'
    SENSITIVITY = 'sensitivity'
    OPTIMIZE = 'optimize'
    TRANSIENT = 'transient'

class Settings:
    """Settings used for an OpenSD simulation.

    Parameters
    ----------
    **kwargs : dict, optional
        Any keyword arguments are used to set attributes on the instance.

    Attributes
    ----------
    tim_slot : list of list of delt,etime
        Time slot.
    run_mode : {'steady', 'design', 'sensitivity', 'optimize', 'transient'}
        The type of calculation to perform (default is 'steady')
    verbosity : int
        Verbosity during simulation between 1 and 10. Verbosity levels are
        described in :ref:`verbosity`.
    """
    T_ambient = 300.

    def __init__(self, **kwargs):
        
        self._run_mode = RunMode.STEADY
        self._tim_slot = [[0.,0.]]
        self._verbosity = 0

        self._no_main_iter = 3000

        self._conv_crit_flow=1.E-10
        self._no_flow_iter = 300

        self._p_ambient = 1.E5

        self._temp_solve = True
        self._flag_write = True
        self._conv_crit_temp_SS = 1.E-10
        self._conv_crit_temp_trans = self._conv_crit_temp_SS

        self._conv_crit_ht = 1.E-10

        self._ZERO = 1.E-8

        self._show_warn = False

        self._alpha_mom = 0.6
        self._alpha_ener = np.array(1.0)
        self._alpha_heat = np.array(1.0)

        
        for key, value in kwargs.items():
            setattr(self, key, value)

    @property
    def no_main_iter(self) -> int:
        return self._no_main_iter

    @no_main_iter.setter
    def no_main_iter(self, no_main_iter: int):
        cv.check_type('no_main_iter', no_main_iter, Integral)
        cv.check_greater_than('no_main_iter', no_main_iter, 0)
        self._no_main_iter = no_main_iter

    def export_to_xml(self, path: PathLike = 'settings.xml'):
        """Export simulation settings to an XML file.

        Parameters
        ----------
        path : str
            Path to file to write. Defaults to 'settings.xml'.

        """
        root_element = self.to_xml_element()

        # Check if path is a directory
        p = Path(path)
        if p.is_dir():
            p /= 'settings.xml'

        # Write the XML Tree to the settings.xml file
        tree = ET.ElementTree(root_element)
        tree.write(str(p), xml_declaration=True, encoding='utf-8')


    def to_xml_element(self):
        """Create a 'settings' element to be written to an XML file.

        """
        # Reset xml element tree
        element = ET.Element("settings")
        self._create_run_mode_subelement(element)
        self._create_tim_slot_subelement(element)
        self._create_verbosity_subelement(element)
        self._create_alpha_mom_subelement(element)
        self._create_alpha_ener_subelement(element)
        self._create_alpha_heat_subelement(element)
        self._create_main_iter_subelement(element)
        self._create_flow_iter_subelement(element)
        self._create_temp_solve_subelement(element)
        self._create_flag_write_subelement(element)
        self._create_T_ambient_subelement(element)
        self._create_conv_crit_flow_subelement(element)
        self._create_conv_crit_temp_SS_subelement(element)
        self._create_conv_crit_temp_trans_subelement(element)
        self._create_conv_crit_ht_subelement(element)
        
        # Clean the indentation in the file to be user-readable
        clean_indentation(element)
        reorder_attributes(element)
        
        return element

    def _create_run_mode_subelement(self, root):
        elem = ET.SubElement(root, "run_mode")
        elem.text = self._run_mode.value

    def _create_tim_slot_subelement(self, root):
        elem = ET.SubElement(root, "tim_slot")
        flat_list = [str(val) for pair in self._tim_slot for val in pair]
        elem.text = ' '.join(flat_list)

    def _create_verbosity_subelement(self, root):
        if self._verbosity is not None:
            element = ET.SubElement(root, "verbosity")
            element.text = str(self._verbosity)

    def _create_alpha_mom_subelement(self, root):
        elem = ET.SubElement(root, "alpha_mom")
        elem.text = str(self._alpha_mom)

    def _create_T_ambient_subelement(self, root):
        elem = ET.SubElement(root, "T_ambient")
        elem.text = str(self.T_ambient)

    def _create_conv_crit_flow_subelement(self, root):
        elem = ET.SubElement(root, "conv_crit_flow")
        elem.text = str(self._conv_crit_flow)

    def _create_conv_crit_temp_SS_subelement(self, root):
        elem = ET.SubElement(root, "conv_crit_temp_SS")
        elem.text = str(self._conv_crit_temp_SS)

    def _create_conv_crit_temp_trans_subelement(self, root):
        elem = ET.SubElement(root, "conv_crit_temp_trans")
        elem.text = str(self._conv_crit_temp_trans)

    def _create_conv_crit_ht_subelement(self, root):
        elem = ET.SubElement(root, "conv_crit_ht")
        elem.text = str(self._conv_crit_ht)

    def _create_alpha_ener_subelement(self, root):
        elem = ET.SubElement(root, "alpha_ener")
        elem.text = str(self._alpha_ener)

    def _create_alpha_heat_subelement(self, root):
        elem = ET.SubElement(root, "alpha_heat")
        elem.text = str(self._alpha_heat)

    def _create_main_iter_subelement(self, root):
        elem = ET.SubElement(root, "no_main_iter")
        elem.text = str(self._no_main_iter)

    def _create_flow_iter_subelement(self, root):
        elem = ET.SubElement(root, "no_flow_iter")
        elem.text = str(self._no_flow_iter)

    def _create_temp_solve_subelement(self, root):
        elem = ET.SubElement(root, "temp_solve")
        elem.text = str(self._temp_solve)

    def _create_flag_write_subelement(self, root):
        elem = ET.SubElement(root, "flag_write")
        elem.text = str(self._flag_write)

    def _no_main_iter_from_xml_element(self, root):
        text = get_text(root, 'no_main_iter')
        if text is not None:
            self.no_main_iter = int(text)

    @property
    def verbosity(self) -> int:
        return self._verbosity

    @verbosity.setter
    def verbosity(self, verbosity: int):
        cv.check_type('verbosity', verbosity, Integral)
        cv.check_greater_than('verbosity', verbosity, -1, True)
        self._verbosity = verbosity

    @property
    def temp_solve(self) -> bool:
        return self._temp_solve

    @temp_solve.setter
    def temp_solve(self, temp_solve: bool):
        cv.check_type('temperature solver', temp_solve, bool)
        self._temp_solve = temp_solve

    @property
    def flag_write(self) -> bool:
        return self._flag_write

    @flag_write.setter
    def flag_write(self, flag_write: bool):
        cv.check_type('write outputs', flag_write, bool)
        self._flag_write = flag_write

    @property
    def run_mode(self) -> str:
        return self._run_mode.value

    @run_mode.setter
    def run_mode(self, run_mode: str):
        cv.check_value('run mode', run_mode, {x.value for x in RunMode})
        for mode in RunMode:
            if mode.value == run_mode:
                self._run_mode = mode

    @property
    def tim_slot(self):
        return self._tim_slot

    @tim_slot.setter
    def tim_slot(self, tim_slot):
        # Require a sequence (e.g. list or tuple)
        cv.check_type('tim_slot', tim_slot, abc.Sequence)
        if len(tim_slot) == 0:
            raise TypeError('tim_slot cannot be empty')

        for pair in tim_slot:
            cv.check_type('tim_slot item', pair, abc.Sequence)
            if len(pair) != 2:
                raise TypeError(f'Each "tim_slot" item must be a [dt, t_end] pair, got {pair}')
            dt, t_end = pair
            cv.check_type('dt', dt, (int, float))
            cv.check_type('t_end', t_end, (int, float))

        self._tim_slot = tim_slot

    @property
    def alpha_mom(self) -> float:
        return self._alpha_mom

    @alpha_mom.setter
    def alpha_mom(self, alpha_mom: float):
        cv.check_type('alpha_mom', alpha_mom, Real)
        cv.check_greater_than('alpha_mom', alpha_mom, 0)
        self._alpha_mom = alpha_mom

    @property
    def alpha_ener(self) -> float:
        return self._alpha_ener

    @alpha_ener.setter
    def alpha_ener(self, alpha_ener: float):
        cv.check_type('alpha_ener', alpha_ener, Real)
        cv.check_greater_than('alpha_ener', alpha_ener, 0)
        self._alpha_ener = alpha_ener

    @property
    def alpha_heat(self) -> float:
        return self._alpha_heat

    @alpha_heat.setter
    def alpha_heat(self, alpha_heat: float):
        cv.check_type('alpha_heat', alpha_heat, Real)
        cv.check_greater_than('alpha_heat', alpha_heat, 0)
        self._alpha_heat = alpha_heat

    @property
    def conv_crit_flow(self) -> float:
        return self._conv_crit_flow

    @conv_crit_flow.setter
    def conv_crit_flow(self, conv_crit_flow: float):
        cv.check_type('conv_crit_flow', conv_crit_flow, Real)
        cv.check_greater_than('conv_crit_flow', conv_crit_flow, 0)
        self._conv_crit_flow = conv_crit_flow

    @property
    def conv_crit_temp_SS(self) -> float:
        return self._conv_crit_temp_SS

    @conv_crit_temp_SS.setter
    def conv_crit_temp_SS(self, conv_crit_temp_SS: float):
        cv.check_type('conv_crit_temp_SS', conv_crit_temp_SS, Real)
        cv.check_greater_than('conv_crit_temp_SS', conv_crit_temp_SS, 0)
        self._conv_crit_temp_SS = conv_crit_temp_SS

    @property
    def conv_crit_temp_trans(self) -> float:
        return self._conv_crit_temp_trans

    @conv_crit_temp_trans.setter
    def conv_crit_temp_trans(self, conv_crit_temp_trans: float):
        cv.check_type('conv_crit_temp_trans', conv_crit_temp_trans, Real)
        cv.check_greater_than('conv_crit_temp_trans', conv_crit_temp_trans, 0)
        self._conv_crit_temp_trans = conv_crit_temp_trans

    @property
    def conv_crit_ht(self) -> float:
        return self._conv_crit_ht

    @conv_crit_ht.setter
    def conv_crit_ht(self, conv_crit_ht: float):
        cv.check_type('conv_crit_ht', conv_crit_ht, Real)
        cv.check_greater_than('conv_crit_ht', conv_crit_ht, 0)
        self._conv_crit_ht = conv_crit_ht
