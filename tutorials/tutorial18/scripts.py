import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

def tank_pressure(tank):
    import bindings

    return tank.tpres_gues


def tank_enthalpy(tank):
    import bindings

    return tank.tenth_gues


def lph3_heat_input(time, delt):
    base_heat = -39776208.0996519
    if time > 300.:
        return base_heat * 0.8
    return base_heat
