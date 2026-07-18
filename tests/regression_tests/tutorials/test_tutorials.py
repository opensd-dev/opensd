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
        actual_values = np.interp(check["times"], results["time(s)"], results[check["column"]])
        actual_values = actual_values * check.get("scale", 1.0) + check.get("offset", 0.0)
        expected_values = np.asarray(check["expected"])
        if "rtol" in check:
            np.testing.assert_allclose(actual_values, expected_values, rtol=check["rtol"], atol=0.0)
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
