#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(git -C "$script_dir/.." rev-parse --show-toplevel)"
cache_root="$repo_root/.opensd-binary-cache"
worktree_root="$repo_root/.opensd-build-worktrees"
rolling_worktree="$worktree_root/rolling"
venv_python="$repo_root/venv/bin/python"
overlay_paths=(
  "include/opensd/openmp_interface.h"
)

usage() {
  cat <<'EOF'
Usage:
  tools/opensd_build_cache.sh ensure [REF]
  tools/opensd_build_cache.sh build-last [N]
  tools/opensd_build_cache.sh path [REF]
  tools/opensd_build_cache.sh run REF [OPENSD_ARGS...]
  tools/opensd_build_cache.sh list

Examples:
  tools/opensd_build_cache.sh build-last 10
  OPENSD_EXEC="$(tools/opensd_build_cache.sh path HEAD~3)" ./venv/bin/python -m pytest ...
  tools/opensd_build_cache.sh run c59f721

The cache lives outside Git tracking in:
  .opensd-build-worktrees/rolling/build-cache/
  .opensd-binary-cache/<full-sha>/
EOF
}

resolve_sha() {
  git -C "$repo_root" rev-parse --verify "${1:-HEAD}^{commit}"
}

ensure_submodules() {
  local wt="$1"
  if [[ ! -f "$wt/externals/pugixml/CMakeLists.txt" || ! -f "$wt/externals/CoolProp/CMakeLists.txt" ]]; then
    git -C "$wt" submodule update --init --recursive
  fi
}

apply_local_overlays() {
  local wt="$1"
  for rel in "${overlay_paths[@]}"; do
    if [[ ! -e "$wt/$rel" && -e "$repo_root/$rel" ]]; then
      mkdir -p "$wt/$(dirname "$rel")"
      cp "$repo_root/$rel" "$wt/$rel"
    fi
  done
}

configure_and_build() {
  local wt="$1"
  local build_dir="$wt/build-cache"
  local cmake_args=(
    -S "$wt"
    -B "$build_dir"
    -DCMAKE_BUILD_TYPE=RelWithDebInfo
    -DOPENSD_USE_MPI=ON
  )

  if [[ -x "$venv_python" ]]; then
    cmake_args+=(-DPython_EXECUTABLE="$venv_python")
  fi

  cmake "${cmake_args[@]}"
  cmake --build "$build_dir" --target opensd
}

ensure_ref() {
  local ref="${1:-HEAD}"
  local sha short wt artifact meta build_log
  sha="$(resolve_sha "$ref")"
  short="${sha:0:12}"
  wt="$rolling_worktree"
  artifact="$cache_root/$sha"
  meta="$artifact/metadata.txt"
  build_log="$artifact/build.log"

  mkdir -p "$artifact" "$worktree_root"

  if [[ ! -d "$wt/.git" && ! -f "$wt/.git" ]]; then
    git -C "$repo_root" worktree add --detach "$wt" "$sha"
  else
    git -C "$wt" checkout --detach "$sha"
  fi

  ensure_submodules "$wt"
  apply_local_overlays "$wt"

  if [[ ! -x "$artifact/opensd-run" ]]; then
    if ! configure_and_build "$wt" > "$build_log" 2>&1; then
      echo "Build failed for $sha. Last 80 log lines:" >&2
      tail -80 "$build_log" >&2 || true
      return 1
    fi

    cp "$wt/build-cache/opensd" "$artifact/opensd"
    cp "$wt/build-cache/libopensd_core.so" "$artifact/libopensd_core.so"
    if [[ -f "$wt/build-cache/bindings.so" ]]; then
      cp "$wt/build-cache/bindings.so" "$artifact/bindings.so"
    fi
    cat > "$artifact/opensd-run" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="$here${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
exec "$here/opensd" "$@"
EOF
    chmod +x "$artifact/opensd-run"
  fi

  {
    echo "sha=$sha"
    echo "short=$short"
    echo "rolling_worktree=$wt"
    echo "executable=$artifact/opensd-run"
    echo "raw_executable=$artifact/opensd"
    echo "log=$build_log"
    for rel in "${overlay_paths[@]}"; do
      if [[ -e "$repo_root/$rel" ]] && ! git -C "$wt" ls-files --error-unmatch "$rel" >/dev/null 2>&1; then
        echo "overlay=$rel"
      fi
    done
    echo "built_at=$(date -Is)"
  } > "$meta"

  printf '%s\n' "$artifact/opensd-run"
}

path_ref() {
  local sha exe
  sha="$(resolve_sha "${1:-HEAD}")"
  exe="$cache_root/$sha/opensd-run"
  if [[ ! -x "$exe" ]]; then
    ensure_ref "$sha" >/dev/null
  fi
  printf '%s\n' "$exe"
}

build_last() {
  local n="${1:-10}"
  git -C "$repo_root" rev-list --max-count="$n" HEAD | while read -r sha; do
    echo "==> ensuring $sha"
    ensure_ref "$sha" >/dev/null
  done
}

list_cache() {
  if [[ ! -d "$cache_root" ]]; then
    echo "No cached OpenSD builds yet."
    return
  fi
  find "$cache_root" -mindepth 2 -maxdepth 2 -name metadata.txt -print | sort | while read -r meta; do
    awk -F= '
      $1=="short" { short=$2 }
      $1=="executable" { exe=$2 }
      $1=="built_at" { built=$2 }
      END { if (short) printf "%s  %s  %s\n", short, built, exe }
    ' "$meta"
  done
}

cmd="${1:-}"
shift || true

case "$cmd" in
  ensure) ensure_ref "${1:-HEAD}" ;;
  build-last) build_last "${1:-10}" ;;
  path) path_ref "${1:-HEAD}" ;;
  run)
    ref="${1:-HEAD}"
    shift || true
    exe="$(path_ref "$ref")"
    "$exe" "$@"
    ;;
  list) list_cache ;;
  -h|--help|help|"") usage ;;
  *)
    echo "Unknown command: $cmd" >&2
    usage >&2
    exit 2
    ;;
esac
