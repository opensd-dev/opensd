import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))


def cep_trip_msource(time, delt):
    if time > 20.0:
        return -412.0 * 0.8
    return -412.0


def lph2shell_msource(shell):
    import bindings

    return shell.msource


def lph2shell_esource(shell):
    import bindings

    return shell.esource


def node10_pressure(node):
    import bindings

    return node.tpres_gues


def lph2shell_pressure(shell):
    import bindings

    return shell.tpres_gues


def lph2shell_enthalpy(shell):
    import bindings

    return shell.tenth_gues


def node12_msource(node):
    import bindings

    return node.msource


def lph3shell_msource(shell):
    import bindings

    return shell.msource
