# OpenSD requirements

Version-controlled requirements for the OpenSD project. Each area uses stable IDs for review and traceability.

| Document | ID prefix | Scope |
|----------|-----------|--------|
| [opensd-solver.md](opensd-solver.md) | `SOL-###` | Physics, numerics, components, I/O, Python API, C++ solver |
| [../reactflow/opensd-web/requirements/opensd-web-gui.md](../reactflow/opensd-web/requirements/opensd-web-gui.md) | `GUI-###` | Web model graph and postprocess viewer |

## Conventions

- **Must** — required for the described release or workflow.
- **Should** — strongly desired; may be deferred with documented rationale.
- Each item lists **acceptance criteria** and **implementation** pointers (files/modules).

## Maintenance

1. Assign the next free ID in the relevant document.
2. Update the traceability appendix when behavior changes.
3. Keep theory details in [docs/source/theory](../docs/source/theory/index.rst); requirements state *what* the solver/GUI shall do, not full derivations.

## Optional tooling (not installed by default)

For stricter traceability (baselines, validation, HTML export), consider:

- [Doorstop](https://github.com/doorstop-dev/doorstop) — YAML/text requirements with CLI
- [StrictDoc](https://github.com/strictdoc-project/strictdoc) — Markdown-based requirements and links

The Markdown files here are the source of truth unless the project adopts one of these tools later.
