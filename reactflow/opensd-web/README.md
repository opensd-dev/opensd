# OpenSD web GUI (`opensd-web`)

Browser-based viewer for OpenSD **geometry** (XML) and **results** (HDF5).

## Quick start

```bash
npm install
npm run dev
```

Open the URL shown in the terminal (typically `http://localhost:5173`).

## Features

- **Model** — Import `geometry.xml`, interactive graph (fluid nodes, pipes, BCs, hslabs).
- **Postprocess** — Import HDF5, plot node variables along pipes.

## Requirements

Functional requirements are maintained under [requirements/](requirements/):

- [opensd-web-gui.md](requirements/opensd-web-gui.md) — GUI behavior (`GUI-###`)

Solver requirements: [../../requirements/opensd-solver.md](../../requirements/opensd-solver.md) (`SOL-###`).

## Stack

- [React](https://react.dev/) + [Vite](https://vitejs.dev/)
- [React Flow](https://reactflow.dev/) v11
- [h5wasm](https://github.com/usnistgov/h5wasm) for HDF5 in the browser

## Build

```bash
npm run build
npm run preview
```
