import os
import runpy
import shlex
import shutil
import sys
import tomllib
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


REFERENCE_DATA_ROOT = Path(__file__).with_name("references")


def _load_reference_check(check):
    check = dict(check)
    points = check.pop("points", None)
    if points is not None:
        enabled_points = [point for point in points if point.get("enabled", True)]
        assert enabled_points, "reference point checks require at least one enabled point"
        first_point = enabled_points[0]
        if "time" in first_point:
            check["times"] = [point["time"] for point in enabled_points]
        if "position" in first_point:
            check["sample_positions"] = [point["position"] for point in enabled_points]
        check["expected"] = [point["expected"] for point in enabled_points]
    return check


def _load_pinet_value_references():
    references = {}
    for path in sorted(REFERENCE_DATA_ROOT.glob("tutorial*.toml")):
        data = tomllib.loads(path.read_text(encoding="utf-8"))
        tutorial_name = data.get("benchmark", {}).get("id", path.stem)
        references[tutorial_name] = {
            key: [_load_reference_check(check) for check in data.get(key, [])]
            for key in ("checks", "time_checks", "profile_checks")
            if data.get(key)
        }
    return references


PINET_VALUE_REFERENCES = _load_pinet_value_references()


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
    monkeypatch.syspath_prepend(str(ROOT / "opensd"))
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
