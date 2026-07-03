import runpy
import shutil
import sys
from pathlib import Path

import pytest

import opensd


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
                if candidate == TUTORIALS_ROOT / "tutorial11" / "tutorial.py":
                    marks.append(
                        pytest.mark.xfail(
                            raises=NameError,
                            reason="tutorial11 uses the legacy PINET/comp API.",
                            strict=True,
                        )
                    )
                elif candidate == TUTORIALS_ROOT / "tutorial13" / "tutorial.py":
                    marks.append(
                        pytest.mark.xfail(
                            raises=SystemExit,
                            reason="tutorial13 AFF length does not match the hslab increment count.",
                            strict=True,
                        )
                    )
                elif candidate == TUTORIALS_ROOT / "tutorial2" / "tutorial.ipynb":
                    marks.append(
                        pytest.mark.xfail(
                            raises=AttributeError,
                            reason="tutorial2 reaches the legacy Circuit.calc_temp path.",
                            strict=True,
                        )
                    )
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
