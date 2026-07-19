import os
import runpy
import shlex
import shutil
import sys
from pathlib import Path

import pytest
import numpy as np

import opensd
from opensd import executor
from opensd.circuit import Circuit
from opensd.hslab import HSlab, LumpedMass
from opensd import hslab


ROOT = Path(__file__).resolve().parents[3]
TUTORIALS_ROOT = ROOT / "tutorials"


PINET_VALUE_REFERENCES = {
    "tutorial1": {
        "checks": [
            {
                "columns": [f"tpres_gues:node{i}" for i in range(1, 13)],
                "scale": 1.0e-5,
                "expected": [
                    1.439937,
                    1.333257,
                    1.046483,
                    0.9771963,
                    1.01839,
                    0.971006,
                    1.004324,
                    1.041618,
                    1.078198,
                    1.09116,
                    1.00000,
                    1.003655,
                ],
                "decimal": 4,
            }
        ],
    },
    "tutorial2": {
        "checks": [
            {
                "columns": [f"tpres_gues:node{i}" for i in range(1, 30)],
                "scale": 1.0e-5,
                "expected": [
                    6,
                    5.2151,
                    4.1131,
                    3,
                    3.8546,
                    3.2057,
                    3,
                    3.0423,
                    3,
                    3,
                    4.1131,
                    3,
                    5.2151,
                    6,
                    3.9848,
                    3,
                    3.5975,
                    3.1286,
                    3,
                    3,
                    3.5478,
                    3,
                    3.5975,
                    3.1286,
                    3,
                    3.9848,
                    3,
                    3,
                    3,
                ],
                "decimal": 3,
            }
        ],
    },
    "tutorial3": {
        "time_checks": [
            {
                "column": "tpres_gues:node2",
                "scale": 1.0e-5,
                "times": [
                    0,
                    2.8,
                    5.6,
                    8.4,
                    11.2,
                    14,
                    16.8,
                    19.5,
                    22.3,
                    25.1,
                    27.9,
                    30.7,
                    33.5,
                    36.3,
                    39.1,
                    41.9,
                    44.7,
                    47.5,
                    50.3,
                    53.1,
                    55.8,
                    58.6,
                    61.4,
                    64.2,
                    67,
                    69.8,
                    72.6,
                    75.4,
                    78.2,
                    81,
                    83.8,
                    86.6,
                    89.3,
                    92.1,
                    94.9,
                ],
                "expected": [
                    7.885,
                    10.13,
                    12.55,
                    15.12,
                    17.78,
                    16.94,
                    15.38,
                    13.45,
                    9.13,
                    8.11,
                    7.44,
                    7.025,
                    10.52,
                    11.53,
                    12.11,
                    12.45,
                    9.35,
                    8.31,
                    7.52,
                    7.12,
                    10.61,
                    11.54,
                    12.21,
                    12.81,
                    9.45,
                    8.485,
                    7.59,
                    7.19,
                    10.56,
                    11.40,
                    12.33,
                    12.90,
                    9.45,
                    8.51,
                    7.68,
                ],
                "decimal": 0,
            }
        ],
    },
    "tutorial4": {
        "time_checks": [
            {
                "column": "tpres_gues:pipe2_node4",
                "scale": 1.0 / 7.0e5,
                "offset": -1.0,
                "times": [
                    0, 0.0138522427, 0.0166226913, 0.0175461741, 0.0193931398, 0.0212401055,
                    0.0221635884, 0.0230870712, 0.0230870712, 0.0240105541, 0.0240105541, 0.0258575198,
                    0.0267810026, 0.0277044855, 0.0295514512, 0.030474934, 0.0318601583, 0.033707124,
                    0.036939314, 0.0387862797, 0.040171504, 0.0415567282, 0.0434036939, 0.0452506596,
                    0.0470976253, 0.0475593668, 0.0494063325, 0.0517150396, 0.0526385224, 0.055408971,
                    0.0577176781, 0.0595646438, 0.0618733509, 0.0637203166, 0.0664907652, 0.0692612137,
                    0.073878628, 0.0757255937, 0.0784960422, 0.0812664908, 0.0840369393, 0.085883905,
                    0.0868073879, 0.0886543536, 0.0905013193, 0.092348285, 0.0951187335, 0.0988126649,
                    0.101583113, 0.103430079, 0.107124011, 0.108970976, 0.112203166, 0.116358839,
                    0.118205805, 0.122361478, 0.124208443, 0.126978892, 0.127440633, 0.12974934,
                    0.13298153, 0.135751979, 0.137598945, 0.13944591, 0.140369393, 0.143601583,
                    0.148680739, 0.151451187, 0.154683377, 0.157453826, 0.161147757, 0.167612137,
                    0.170844327, 0.172691293, 0.174538259, 0.175461741, 0.178693931, 0.182387863,
                    0.186543536, 0.188390501, 0.192084433, 0.194854881, 0.200395778, 0.205936675,
                    0.207783641, 0.209168865, 0.21055409, 0.213324538, 0.215171504, 0.21701847,
                    0.218865435, 0.221174142, 0.223944591, 0.226253298, 0.230408971, 0.233641161,
                    0.236411609, 0.240105541, 0.244722955, 0.247955145, 0.250263852, 0.252110818,
                    0.253957784, 0.256728232, 0.257189974, 0.259498681, 0.262730871, 0.264577836,
                    0.266424802, 0.268733509, 0.271965699, 0.276121372, 0.281200528, 0.285356201,
                    0.289511873, 0.292744063, 0.295976253, 0.300131926, 0.302440633, 0.30474934,
                    0.307058047, 0.310290237, 0.31444591, 0.319986807, 0.323218997, 0.327836412,
                    0.329683377, 0.332453826, 0.335224274, 0.338918206, 0.341688654, 0.34353562,
                    0.345844327, 0.348153034, 0.350923483, 0.354617414, 0.35969657, 0.363852243,
                    0.365699208, 0.367546174, 0.369854881, 0.371701847, 0.37585752, 0.379551451,
                    0.383245383, 0.386477573, 0.388324538, 0.393865435, 0.396174142, 0.400791557,
                    0.404023747, 0.406332454, 0.408641161, 0.411873351, 0.415567282, 0.419261214,
                    0.422955145, 0.428034301, 0.436807388, 0.441424802, 0.445118734, 0.447427441,
                    0.451583113, 0.456200528, 0.458970976, 0.460817942, 0.464511873, 0.470976253,
                    0.476978892, 0.482519789, 0.48621372, 0.48944591, 0.4926781, 0.496833773,
                    0.499604222, 0.503298153, 0.507915567, 0.512532982, 0.516226913, 0.52176781,
                    0.524538259, 0.527308707, 0.531926121, 0.536081794, 0.539313984, 0.542084433,
                    0.543931398, 0.547163588, 0.553166227, 0.557783641, 0.56055409, 0.562862797,
                    0.565171504, 0.568403694, 0.57348285, 0.57671504, 0.584564644, 0.589182058,
                    0.594722955, 0.597493404, 0.599340369, 0.601187335, 0.604419525, 0.608113456,
                    0.612269129, 0.615963061, 0.618733509, 0.624274406, 0.627968338, 0.630277045,
                    0.632124011, 0.633970976, 0.640435356, 0.64505277, 0.648746702, 0.651055409,
                    0.652902375, 0.657519789, 0.659828496, 0.665369393, 0.67091029, 0.675065963,
                    0.681068602, 0.687532982, 0.69030343, 0.691226913, 0.694459103, 0.699076517,
                ],
                "expected": [
                    -0.0111221945, -0.0111221945, -0.00783042394, -0.00289276808, 0.0124688279,
                    0.0404488778, 0.0651371571, 0.085436409, 0.0881795511, 0.0909226933, 0.0936658354,
                    0.0876309227, 0.0794014963, 0.0772069825, 0.0794014963, 0.08159601, 0.0769326683,
                    0.0667830424, 0.0525187032, 0.0503241895, 0.0519700748, 0.0536159601, 0.0486783042,
                    0.0409975062, 0.0349625935, 0.033042394, 0.0316708229, 0.0344139651, 0.0388029925,
                    0.047032419, 0.0574563591, 0.0750124688, 0.0865336658, 0.0906483791, 0.0821446384,
                    0.0761097257, 0.058553616, 0.0503241895, 0.047032419, 0.0448379052, 0.0388029925,
                    0.0289276808, 0.0223441397, 0.0080798005, 0.000947630923, -0.00179551122,
                    0.00643391521, 0.0217955112, 0.0338653367, 0.0393516209, 0.0289276808,
                    0.0179551122, -0.00124688279, -0.0275810474, -0.0352618454, -0.042394015,
                    -0.0506234414, -0.0676309227, -0.0742144638, -0.076957606, -0.073117207,
                    -0.0621446384, -0.053915212, -0.0426683292, -0.0358104738, -0.0295012469,
                    -0.0412967581, -0.0572069825, -0.0676309227, -0.076957606, -0.0788778055,
                    -0.073117207, -0.0626932668, -0.055286783, -0.046234414, -0.0341645885,
                    -0.0209975062, -0.0122194514, -0.0177057357, -0.0242892768, -0.0330673317,
                    -0.0352618454, -0.0297755611, -0.0264837905, -0.0209975062, -0.0155112219,
                    -0.00563591022, 0.0108229426, 0.0223441397, 0.0344139651, 0.0415461347,
                    0.0442892768, 0.0415461347, 0.0371571072, 0.0333167082, 0.0357855362, 0.0374314214,
                    0.0360598504, 0.0324937656, 0.0366084788, 0.0448379052, 0.0552618454, 0.0618453865,
                    0.0761097257, 0.08159601, 0.085436409, 0.0799501247, 0.0722693267, 0.0601995012,
                    0.047032419, 0.0355112219, 0.0272817955, 0.0171321696, 0.0146633416, 0.0220698254,
                    0.0311221945, 0.0341396509, 0.0289276808, 0.0185037406, 0.00917705736,
                    -0.00289276808, -0.0127680798, -0.0253865337, -0.0341645885, -0.0363591022,
                    -0.0292269327, -0.02319202, -0.0171571072, -0.0155112219, -0.02319202,
                    -0.0297755611, -0.0401995012, -0.0478802993, -0.0555610973, -0.06159601,
                    -0.0665336658, -0.0714713217, -0.0673566085, -0.0610473815, -0.0522693267,
                    -0.0412967581, -0.0303241895, -0.0237406484, -0.0286783042, -0.0363591022,
                    -0.0404738155, -0.0418453865, -0.0363591022, -0.030872818, -0.0204488778,
                    -0.0089276808, 0.0042394015, 0.0141147132, 0.0228927681, 0.029201995, 0.0234413965,
                    0.0174064838, 0.0121945137, 0.0196009975, 0.0311221945, 0.0423690773, 0.0514214464,
                    0.0574563591, 0.0519700748, 0.0459351621, 0.0409975062, 0.0360598504, 0.0305735661,
                    0.0278304239, 0.033042394, 0.0418204489, 0.0442892768, 0.040723192, 0.0283790524,
                    0.0141147132, -0.00234413965, -0.0133167082, -0.0209975062, -0.0226433915,
                    -0.0166084788, -0.0122194514, -0.0100249377, -0.0138653367, -0.0275810474,
                    -0.0391022444, -0.0467830424, -0.0506234414, -0.0530922693, -0.0489775561,
                    -0.0412967581, -0.0341645885, -0.0303241895, -0.0267581047, -0.0248379052,
                    -0.030872818, -0.0363591022, -0.0418453865, -0.0391022444, -0.0292269327,
                    -0.0182543641, -0.0111221945, -0.00563591022, 0.00314214464, 0.00725685786,
                    0.00369077307, 0.000399002494, -0.00124688279, 0.00286783042, 0.00862842893,
                    0.0130174564, 0.0185037406, 0.0245386534, 0.0415461347, 0.0481296758, 0.0459351621,
                    0.0429177057, 0.0377057357, 0.0294763092, 0.0256359102, 0.0231670823, 0.0270074813,
                    0.0327680798, 0.0379800499, 0.0311221945, 0.0242643392, 0.0206982544, 0.0124688279,
                    0.00314214464,
                ],
                "atol": 0.025,
            }
        ],
    },
    "tutorial5": {
        "time_checks": [
            {
                "column": "msource:node1",
                "time_shift": 10.0,
                "times": [
                    0.0,
                    27.2020725388601,
                    43.0699481865285,
                    81.6062176165803,
                    142.810880829016,
                    197.215025906736,
                    242.551813471503,
                    301.489637305699,
                    343.426165803109,
                    444.300518134715,
                    544.041450777202,
                    644.915803108808,
                    743.523316062176,
                    845.531088082902,
                    944.138601036269,
                    1144.75388601036,
                    1244.49481865285,
                    1344.23575129534,
                    1445.11010362694,
                    1545.98445595855,
                    1643.4585492228,
                    1749.99999999999,
                ],
                "expected": [
                    1.76167076167076,
                    1.66584766584766,
                    1.61425061425061,
                    1.5036855036855,
                    1.32678132678133,
                    1.19410319410319,
                    1.09090909090909,
                    0.972972972972973,
                    0.894348894348894,
                    0.732186732186732,
                    0.5995085995086,
                    0.491400491400492,
                    0.400491400491401,
                    0.329238329238329,
                    0.27027027027027,
                    0.181818181818182,
                    0.147420147420148,
                    0.120393120393121,
                    0.0982800982800987,
                    0.0786240786240787,
                    0.0638820638820644,
                    0.051597051597052,
                ],
                "atol": 0.01,
            }
        ],
    },
    "tutorial6": {
        "time_checks": [
            {
                "column": "ttemp_gues:node2",
                "offset": -273.15,
                "times": [0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50],
                "expected": [
                    393.3472,
                    393.3383,
                    392.9872,
                    391.5439,
                    389.3058,
                    387.3356,
                    385.9367,
                    384.9961,
                    384.3729,
                    383.9614,
                    383.6887,
                ],
                "decimal": 0,
            }
        ],
    },
    "tutorial7": {
        "time_checks": [
            {
                "column": "vflow:pipe1",
                "times": [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20],
                "expected": [
                    0.0018603,
                    0.0017773,
                    0.0016833,
                    0.0015824,
                    0.001473,
                    0.0013531,
                    0.0013426,
                    0.0013426,
                    0.0013426,
                    0.0013426,
                    0.0013426,
                    0.0013426,
                    0.0013426,
                    0.0013426,
                    0.0013426,
                    0.0013426,
                    0.0013426,
                    0.0013426,
                    0.0013426,
                    0.0013426,
                    0.0013426,
                ],
                "decimal": 5,
            }
        ],
    },
    "tutorial8": {
        "checks": [
            {
                "columns": [f"spres_gues:pipe1_face{i}" for i in range(20)],
                "scale": 1.0e-3,
                "expected": [
                    6888.016504,
                    6886.136167,
                    6884.268556,
                    6882.421156,
                    6880.594158,
                    6878.787761,
                    6877.002146,
                    6875.201083,
                    6873.135554,
                    6870.609996,
                    6867.839134,
                    6865.020446,
                    6862.133006,
                    6859.145805,
                    6856.043237,
                    6852.816902,
                    6849.462167,
                    6845.976515,
                    6842.358969,
                    6838.968367,
                ],
                "decimal": 0,
            },
            {
                "columns": [f"vflow:pipe1_face{i}" for i in range(20)],
                "expected": [
                    0.000125915,
                    0.000127811,
                    0.000129842,
                    0.000132023,
                    0.000134371,
                    0.000136906,
                    0.000139655,
                    0.000142649,
                    0.000154655,
                    0.000234802,
                    0.00032516,
                    0.000414691,
                    0.000503866,
                    0.000592881,
                    0.000681835,
                    0.000770785,
                    0.000859763,
                    0.000948795,
                    0.001037897,
                    0.001127013,
                ],
                "decimal": 5,
            },
        ],
    },
    "tutorial9": {
        "checks": [
            {
                "columns": ["ttemp_gues:node4"],
                "offset": -273.15,
                "expected": [514.43],
                "decimal": 1,
            }
        ],
    },
    "tutorial10": {
        "checks": [
            {
                "columns": [f"mflow:pipe{i}" for i in range(1, 4)],
                "expected": [0.8834, 0.8834, 0.8834],
                "decimal": 4,
            },
            {
                "columns": ["ttemp_gues:node1", "ttemp_gues:node4"],
                "transform": "absolute_difference",
                "expected": [5.409],
                "decimal": 3,
            },
        ],
    },
    "tutorial12": {
        "checks": [
            {
                "columns": [f"mflow:pipe{i}" for i in range(1, 6)],
                "expected": [0.2901, 0.2901, 0.2901, 0.2901, 0.2901],
                "decimal": 4,
            },
            {
                "columns": ["Qth:node4"],
                "expected": [0.02180],
                "decimal": 5,
            },
        ],
    },
    "tutorial13": {
        "profile_checks": [
            {
                "columns": ["ttemp_gues:node1"]
                + [f"ttemp_gues:pipe1_node{i}" for i in range(9)]
                + ["ttemp_gues:node2"],
                "positions": [i * 0.9144 / 10.0 for i in range(11)],
                "sample_positions": [
                    0.079332229,
                    0.160098077,
                    0.244005707,
                    0.319292655,
                    0.389878412,
                    0.462033562,
                    0.531049926,
                    0.5984949,
                    0.681620828,
                    0.763170374,
                    0.838439352,
                ],
                "offset": -273.15,
                "expected": [
                    331.9095582,
                    339.9206318,
                    349.1795528,
                    358.4400111,
                    367.8796519,
                    377.6757007,
                    386.7589334,
                    395.1290706,
                    404.7448189,
                    412.7557527,
                    418.8060232,
                ],
                "decimal": 0,
            }
        ],
    },
    "tutorial18": {
        "time_checks": [
            {
                "column": "tank_pressure",
                "scale": 1.0e-5,
                "times": [0.0, 300.0, 400.0, 500.0, 600.0, 700.0, 800.0, 900.0, 1000.0],
                "expected": [
                    1.520482,
                    1.522878949,
                    1.77061509,
                    2.03328268,
                    2.30931901,
                    2.59667375,
                    2.893964496,
                    3.199335589,
                    3.511451651,
                ],
                "decimal": 2,
            },
            {
                "column": "tank_enthalpy",
                "scale": 1.0e-3,
                "times": [0.0, 300.0, 400.0, 500.0, 600.0, 700.0, 800.0, 900.0, 1000.0],
                "expected": [
                    484.058,
                    484.2800002,
                    505.9528118,
                    526.6135666,
                    546.318825,
                    565.1195728,
                    583.0623281,
                    600.1986703,
                    616.5662238,
                ],
                "decimal": 0,
            },
        ],
    },
    "tutorial20": {
        "time_checks": [
            {
                "column": "tank_pressure",
                "scale": 1.0e-5,
                "times": [
                    0.0,
                    0.01,
                    0.02,
                    0.03,
                    0.04,
                    0.05,
                    0.1,
                    0.5,
                    1.0,
                    1.5,
                    2.0,
                    2.1,
                    2.2,
                    2.3,
                    2.4,
                    2.5,
                    2.6,
                    2.7,
                    2.8,
                    2.9,
                    3.0,
                    4.0,
                    5.0,
                    6.0,
                    7.0,
                    8.0,
                    9.0,
                    9.98,
                ],
                "expected": [
                    123,
                    84.37104134,
                    60.91706122,
                    48.15655766,
                    46.61774324,
                    46.61125364,
                    46.57877175,
                    46.31684825,
                    45.98409963,
                    45.64510976,
                    45.29954167,
                    45.22958318,
                    45.15937604,
                    45.08888449,
                    44.94650433,
                    44.15543954,
                    43.38333019,
                    42.62964342,
                    41.8938622,
                    41.17548486,
                    40.47402479,
                    34.29273306,
                    29.36422288,
                    25.39060768,
                    22.15209814,
                    19.48579418,
                    17.26936544,
                    15.42675776,
                ],
                "rtol": 0.03,
            }
        ],
    },
}


def _tutorial_entrypoints():
    entrypoints = []
    for tutorial_dir in sorted(TUTORIALS_ROOT.glob("tutorial*")):
        if not tutorial_dir.is_dir():
            continue

        candidates = [
            tutorial_dir / "tutorial.py",
            tutorial_dir / "build_xml.py",
            tutorial_dir / "tutorial.ipynb",
        ]
        for candidate in candidates:
            if candidate.exists():
                marks = []
                expected = (
                    ROOT
                    / "tests"
                    / "regression_tests"
                    / "tutorials"
                    / "flow"
                    / candidate.parent.name
                    / "results_true.res"
                )
                if not expected.exists() and candidate.parent.name not in PINET_VALUE_REFERENCES:
                    marks.append(
                        pytest.mark.xfail(
                            reason=f"{candidate.parent.name} has no results_true.res or PINET-tests reference.",
                            strict=True,
                        )
                    )
                entrypoints.append(
                    pytest.param(candidate, expected, id=candidate.parent.name, marks=marks)
                )
                break

    return entrypoints


def _copy_tutorial_inputs(src_dir, dst_dir):
    def ignore(_dir, names):
        ignored = {
            "__pycache__",
            ".ipynb_checkpoints",
            "circuits.h5",
            "bindings.so",
            "output.res",
        }
        ignored.update(name for name in names if name.endswith(".xml"))
        ignored.update(name for name in names if name.endswith(".txt"))
        ignored.update(name for name in names if name.endswith(".png"))
        ignored.update(name for name in names if name.endswith("_error.res"))
        ignored.update(name for name in names if name.startswith("faces_owned_"))
        ignored.update(name for name in names if name.startswith("face_reindex_"))
        ignored.update(name for name in names if name.startswith("ghosts_"))
        ignored.update(name for name in names if name.startswith("partition_"))
        return ignored

    shutil.copytree(src_dir, dst_dir, ignore=ignore)


def _run_python_script(script_path):
    runpy.run_path(str(script_path), run_name="__main__")


def _run_notebook(notebook_path):
    nbformat = pytest.importorskip("nbformat")
    notebook = nbformat.read(notebook_path, as_version=4)
    namespace = {"__name__": "__main__"}

    for index, cell in enumerate(notebook.cells):
        if cell.cell_type != "code" or not cell.source.strip():
            continue

        code = compile(cell.source, f"{notebook_path}::cell-{index}", "exec")
        exec(code, namespace)


def _reset_opensd_registries():
    Circuit._registry.clear()
    HSlab._registry.clear()
    LumpedMass._registry.clear()
    hslab.comps.clear()


def _opensd_executable():
    configured = os.environ.get("OPENSD_EXEC")
    if configured:
        return configured

    if os.name == "nt":
        return None

    candidates = [
        ROOT / "build" / "opensd",
        ROOT / "build" / "opensd.exe",
    ]
    for candidate in candidates:
        if candidate.exists():
            return str(candidate)

    return None


def _compare_results(expected, actual, tutorial_name):
    """Make sure the current results agree with the reference values."""
    assert actual.exists(), f"{tutorial_name} did not write output.res"

    expected_results = _read_results(expected)
    actual_results = _read_results(actual)

    expected_time = expected_results.get("time(s)")
    actual_time = actual_results.get("time(s)")
    assert expected_time is not None, f"{expected} is missing time(s)"
    assert actual_time is not None, f"{actual} is missing time(s)"

    missing = [column for column in expected_results if column not in actual_results]
    assert not missing, f"Missing result columns: {', '.join(missing)}"

    for column, expected_values in expected_results.items():
        if column == "time(s)":
            continue
        actual_values = np.interp(expected_time, actual_time, actual_results[column])
        np.testing.assert_allclose(
            actual_values,
            expected_values,
            rtol=1.0e-5,
            atol=1.0e-7,
            err_msg=f"{tutorial_name} column {column} differs from results_true.res",
        )


def _read_results(path):
    assert path.exists(), f"{path} does not exist"

    lines = [line.strip() for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]
    assert lines, f"{path} is empty"

    delimiter = "," if "," in lines[0] else None
    columns = [part.strip() for part in lines[0].split(delimiter) if part.strip()]
    rows = []
    for line in lines[1:]:
        values = [part.strip() for part in line.split(delimiter) if part.strip()]
        if len(values) != len(columns):
            continue
        rows.append([float(value) for value in values])

    assert rows, f"{path} does not contain any result rows"
    data = np.asarray(rows)
    return {column: data[:, index] for index, column in enumerate(columns)}


def _last_values(results, columns):
    missing = [column for column in columns if column not in results]
    assert not missing, f"Missing result columns: {', '.join(missing)}"
    return np.asarray([results[column][-1] for column in columns])


def _compare_pinet_values(actual, tutorial_name):
    reference = PINET_VALUE_REFERENCES.get(tutorial_name)
    assert reference is not None, f"{tutorial_name} is missing results_true.res and PINET value reference"

    results = _read_results(actual)
    for check in reference.get("checks", []):
        actual_values = _last_values(results, check["columns"])
        actual_values = actual_values * check.get("scale", 1.0) + check.get("offset", 0.0)
        if check.get("transform") == "absolute_difference":
            assert len(actual_values) == 2, "absolute_difference checks require exactly two columns"
            actual_values = np.asarray([abs(actual_values[0] - actual_values[1])])
        expected_values = np.asarray(check["expected"])
        np.testing.assert_array_almost_equal(
            actual_values,
            expected_values,
            decimal=check["decimal"],
        )

    for check in reference.get("time_checks", []):
        required = ["time(s)", check["column"]]
        missing = [column for column in required if column not in results]
        assert not missing, f"Missing result columns: {', '.join(missing)}"
        sample_times = np.asarray(check["times"]) + check.get("time_shift", 0.0)
        actual_values = np.interp(sample_times, results["time(s)"], results[check["column"]])
        actual_values = actual_values * check.get("scale", 1.0) + check.get("offset", 0.0)
        expected_values = np.asarray(check["expected"])
        if "rtol" in check or "atol" in check:
            np.testing.assert_allclose(
                actual_values,
                expected_values,
                rtol=check.get("rtol", 0.0),
                atol=check.get("atol", 0.0),
            )
        else:
            np.testing.assert_array_almost_equal(
                actual_values,
                expected_values,
                decimal=check["decimal"],
            )

    for check in reference.get("profile_checks", []):
        profile = _last_values(results, check["columns"])
        profile = profile * check.get("scale", 1.0) + check.get("offset", 0.0)
        actual_values = np.interp(check["sample_positions"], check["positions"], profile)
        expected_values = np.asarray(check["expected"])
        np.testing.assert_array_almost_equal(
            actual_values,
            expected_values,
            decimal=check["decimal"],
        )


@pytest.mark.parametrize(("entrypoint", "expected"), _tutorial_entrypoints())
def test_tutorial_results_agree_with_reference(monkeypatch, tmp_path, entrypoint, expected):
    tutorial_name = entrypoint.parent.name
    assert expected.exists() or tutorial_name in PINET_VALUE_REFERENCES, (
        f"{tutorial_name} is missing results_true.res and PINET-tests reference"
    )

    opensd_exec = _opensd_executable()
    if opensd_exec is None:
        pytest.skip("OpenSD executable is not available; set OPENSD_EXEC to run tutorial regressions")

    work_dir = tmp_path / tutorial_name
    _copy_tutorial_inputs(entrypoint.parent, work_dir)
    copied_entrypoint = work_dir / entrypoint.name

    def run_with_test_executable(*args, **kwargs):
        kwargs["opensd_exec"] = opensd_exec
        mpi_args = os.environ.get("OPENSD_MPI_ARGS")
        kwargs["mpi_args"] = shlex.split(mpi_args) if mpi_args else None
        return executor.run(*args, **kwargs)

    monkeypatch.setattr(opensd, "run", run_with_test_executable)
    monkeypatch.setenv("MPLBACKEND", "Agg")
    monkeypatch.chdir(work_dir)
    monkeypatch.syspath_prepend(str(ROOT))
    monkeypatch.syspath_prepend(str(work_dir))
    _reset_opensd_registries()
    sys.modules.pop("scripts", None)

    if copied_entrypoint.suffix == ".ipynb":
        _run_notebook(copied_entrypoint)
    else:
        _run_python_script(copied_entrypoint)

    generated_xml = sorted(path.name for path in work_dir.glob("*.xml"))
    assert generated_xml, f"{entrypoint} did not generate any XML input files"
    assert "settings.xml" in generated_xml
    output = work_dir / "output.res"
    if tutorial_name in PINET_VALUE_REFERENCES:
        _compare_pinet_values(output, tutorial_name)
    elif expected.exists():
        _compare_results(expected, output, tutorial_name)
