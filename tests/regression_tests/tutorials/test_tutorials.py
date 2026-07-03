import runpy
import shutil
import sys
from pathlib import Path

import pytest

import opensd
from opensd.circuit import Circuit
from opensd.hslab import HSlab, LumpedMass
from opensd import hslab


ROOT = Path(__file__).resolve().parents[3]
TUTORIALS_ROOT = ROOT / "tutorials"


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
                entrypoints.append(pytest.param(candidate, id=candidate.parent.name, marks=marks))
                break

    return entrypoints


def _run_python_script(script_path):
    runpy.run_path(str(script_path), run_name="__main__")


def _run_notebook_until_solver_call(notebook_path):
    nbformat = pytest.importorskip("nbformat")
    notebook = nbformat.read(notebook_path, as_version=4)
    namespace = {"__name__": "__main__"}

    for index, cell in enumerate(notebook.cells):
        if cell.cell_type != "code" or not cell.source.strip():
            continue

        code = compile(cell.source, f"{notebook_path}::cell-{index}", "exec")
        exec(code, namespace)

        if "opensd.run" in cell.source:
            break


def _reset_opensd_registries():
    Circuit._registry.clear()
    HSlab._registry.clear()
    LumpedMass._registry.clear()
    hslab.comps.clear()


@pytest.mark.parametrize("entrypoint", _tutorial_entrypoints())
def test_tutorial_builds_inputs_and_reaches_solver(monkeypatch, tmp_path, entrypoint):
    run_calls = []

    def fake_run(*args, **kwargs):
        run_calls.append((args, kwargs))
        Path("output.res").write_text("mocked solver output\n", encoding="utf-8")

    monkeypatch.setattr(opensd, "run", fake_run)
    monkeypatch.chdir(tmp_path)
    monkeypatch.syspath_prepend(str(ROOT))
    monkeypatch.syspath_prepend(str(entrypoint.parent))
    _reset_opensd_registries()
    sys.modules.pop("scripts", None)

    for data_file in entrypoint.parent.glob("*.csv"):
        shutil.copy(data_file, tmp_path / data_file.name)

    if entrypoint.suffix == ".ipynb":
        _run_notebook_until_solver_call(entrypoint)
    else:
        _run_python_script(entrypoint)

    generated_xml = sorted(path.name for path in tmp_path.glob("*.xml"))
    assert generated_xml, f"{entrypoint} did not generate any XML input files"
    assert "settings.xml" in generated_xml
    assert run_calls, f"{entrypoint} did not call opensd.run()"
