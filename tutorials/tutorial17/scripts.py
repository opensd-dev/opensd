import math


TANK_DIAMETER = 3.6
TANK_HEIGHT = 29.88
TANK_GEOMETRY = ("vertcyl", TANK_DIAMETER, TANK_HEIGHT)
INITIAL_PRESSURE = 2.0e5
INITIAL_ENTHALPY = 515711.524041494
INITIAL_LEVEL = 5.74978952
TRANSIENT_OUTFLOW = 5.0
INFLOW = 10.0
LIQUID_DENSITY = 943.477


def _interp(x_values, y_values, x):
    if x <= x_values[0]:
        return y_values[0]
    for index in range(1, len(x_values)):
        if x <= x_values[index]:
            x0 = x_values[index - 1]
            x1 = x_values[index]
            y0 = y_values[index - 1]
            y1 = y_values[index]
            return y0 + (y1 - y0) * (x - x0) / (x1 - x0)
    return y_values[-1]


def _tank_area(geom):
    return math.pi * geom[1] ** 2 / 4.0


def _tank_volume(geom):
    return _tank_area(geom) * geom[2]


def pinet_tptank_level(geom, phase, quality, rhomass, rhog):
    if phase == 0:
        return geom[2]
    if phase == 5:
        return 0.0
    if phase != 6:
        raise ValueError("TPTank phase not known")

    volfracliq = 1.0 - quality * rhomass / rhog
    liquid_volume = volfracliq * _tank_volume(geom)
    if geom[0] == "vertcyl":
        return liquid_volume / _tank_area(geom)
    if geom[0] == "horicyl":
        h_values = [
            0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9,
            1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9,
            2.0, 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9,
            3.0, 3.1, 3.2, 3.3, 3.4, 3.5, 3.6,
        ]
        v_values = [
            0.0, 2.370380108, 6.647244898, 12.10558335, 18.47253262,
            25.58269726, 33.31899524, 41.59114995, 50.3255932,
            59.45997228, 68.93985091, 78.71657851, 88.74583345,
            98.98657869, 109.4002813, 119.9503061, 130.6014264,
            141.3194133, 152.0706774, 162.8219415, 173.5399283,
            184.1910486, 194.7410734, 205.154776, 215.3955213,
            225.4247762, 235.2015038, 244.6813824, 253.8157615,
            262.5502048, 270.8223595, 278.5586574, 285.6688221,
            292.0357714, 297.4941098, 301.7709746, 304.1413547,
        ]
        return _interp(v_values, h_values, liquid_volume)
    raise ValueError("geometry not found")


def _initial_level_from_pinet_state():
    try:
        import CoolProp.CoolProp as CP

        rhomass = CP.PropsSI("D", "P", INITIAL_PRESSURE, "H", INITIAL_ENTHALPY, "Water")
        quality = CP.PropsSI("Q", "P", INITIAL_PRESSURE, "H", INITIAL_ENTHALPY, "Water")
        rhog = CP.PropsSI("D", "P", INITIAL_PRESSURE, "Q", 1.0, "Water")
        if 0.0 <= quality <= 1.0:
            return pinet_tptank_level(TANK_GEOMETRY, 6, quality, rhomass, rhog)
    except Exception:
        pass
    return INITIAL_LEVEL


def tank_level(tank1):
    initial_level = _initial_level_from_pinet_state()
    if time <= 0.0:
        return initial_level
    net_mflow = INFLOW - TRANSIENT_OUTFLOW
    return initial_level + net_mflow * time / (LIQUID_DENSITY * _tank_area(TANK_GEOMETRY))
