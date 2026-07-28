import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))


def tank_pressure(tank):
    import bindings

    return tank.tpres_gues
