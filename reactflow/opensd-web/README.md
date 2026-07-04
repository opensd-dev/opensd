# OpenSD web GUI (`opensd-web`)

Browser-based viewer for OpenSD **geometry** (XML) and **results** (HDF5).

## Quick start

```bash
npm install
cp .env.example .env
# edit .env and set a private OPENSD_WEB_USERNAME and OPENSD_WEB_PASSWORD
npm run dev
```

Open the URL shown in the terminal (typically `http://localhost:5173`).

Access control is enabled by default. The server returns `503` until
`OPENSD_WEB_USERNAME` and `OPENSD_WEB_PASSWORD` are configured. To disable the
login prompt for private local development only, set `OPENSD_WEB_AUTH=off`.

## Network access

The development and preview servers bind to `0.0.0.0`, so other computers on
the same network can open the app with the host computer's IP address:

```text
http://<host-ip-address>:5173
```

Use the username and password from `.env` when the browser prompts for login.
On Windows, run `ipconfig` and use the IPv4 address for the active network
adapter. On WSL/Linux, run `hostname -I`.

## Features

- **Model** — Import `geometry.xml`, interactive graph (fluid nodes, pipes, BCs, hslabs).
- **Solver** — Edit/import/export `settings.xml` and run `/mnt/c/codes/opensd/build/opensd` in a selected OpenSD working directory.
- **Postprocess** — Import HDF5, plot node variables along pipes.

- **Help** - Press `F1` for browser tutorials covering tutorial import, browser model building, solver runs, and postprocessing.

## Requirements

Solver working directories may be anywhere under `/mnt/c` by default. Set `OPENSD_WORKING_ROOTS` to a comma-separated list of absolute WSL roots to narrow or extend the allowed locations.

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
