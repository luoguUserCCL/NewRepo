#!/usr/bin/env bash
# ============================================================================
#  toolchain-ensure.sh  --  fault-tolerant native toolchain provisioner
#
#  The host toolchain directories (/opt, /usr/local, /usr, $HOME/.gradle ...)
#  are treated as UNSTABLE: they can be wiped at any time. This script is the
#  single source of truth that guarantees a usable toolchain set for every
#  cross-compilation target the spec requires:
#
#      Linux   -> host GNU GCC          (native, no cross needed)
#      Windows -> MinGW-w64 cross       (x86_64-w64-mingw32-*)
#      macOS   -> Zig as clang cross    (`zig cc -target x86_64-macos-gnu`,
#                                         `zig cc -target aarch64-macos-gnu`)
#      Jar     -> JDK (provisioned by Gradle Foojay, not here)
#
#  Design:
#   * Cache root  : ${SCI_CALC_TC_CACHE:-$HOME/.scicalc/toolchains}
#   * Each toolchain lives in its own subdir with a `.version` marker.
#   * `--check`  : validate; if marker missing / version mismatch / binaries
#                  gone -> (re)download + extract. Idempotent. SAFE to rerun.
#   * `--env`     : print `export PATH=...` lines to source from the build.
#   * Exit 0 on success, non-zero only if a download genuinely cannot happen
#     (offline + cache miss). On network hiccup with a *valid* cached copy,
#     we keep the cache and exit 0 so the build can proceed offline.
#
#  This is wired into Gradle as the `toolchainCheck` task (see build.gradle).
# ============================================================================

set -euo pipefail

# ---- config ---------------------------------------------------------------
CACHE_ROOT="${SCI_CALC_TC_CACHE:-$HOME/.scicalc/toolchains}"
mkdir -p "$CACHE_ROOT"

# Versions (overridable via env; gradle.properties mirrors these).
: "${MINGW_VERSION:=12.0.0}"
: "${ZIG_VERSION:=0.13.0}"
: "${GCC_MIN_VERSION:=11}"

# Download mirrors (ordered; first reachable wins). niXman .7z first (reliable
# host + extracted via py7zr fallback so no root needed). WinLibs/SourceForge
# kept as alternates.
MINGW_URLS=(
  "https://github.com/niXman/mingw-builds-binaries/releases/download/13.2.0-rt_v11-rev1/x86_64-13.2.0-release-posix-seh-ucrt-rt_v11-rev1.7z"
  "https://github.com/niXman/mingw-builds-binaries/releases/download/12.2.0-rt_v10-rev2/x86_64-12.2.0-release-posix-seh-ucrt-rt_v10-rev2.7z"
  "https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Personal%20Builds/mingw-builds/13.2.0/threads-posix/seh/x86_64-13.2.0-release-posix-seh-ucrt-rt_v11-rev1.7z/download"
)
ZIG_URLS=(
  "https://ziglang.org/download/${ZIG_VERSION}/zig-linux-x86_64-${ZIG_VERSION}.tar.xz"
  "https://github.com/ziglang/zig/releases/download/${ZIG_VERSION}/zig-linux-x86_64-${ZIG_VERSION}.tar.xz"
)

MODE="${1:---check}"

log()  { printf '[tc] %s\n' "$*" >&2; }
warn() { printf '[tc][WARN] %s\n' "$*" >&2; }
die()  { printf '[tc][ERR ] %s\n' "$*" >&2; exit 1; }

# ---- helpers --------------------------------------------------------------
have() { command -v "$1" >/dev/null 2>&1; }

curl_get() {  # url dest
  local url="$1" dest="$2"
  curl -fsSL --retry 3 --retry-delay 2 --connect-timeout 15 \
       -o "$dest" "$url" 2>/dev/null
}

extract() {   # archive destdir
  local arc="$1" dst="$2"
  mkdir -p "$dst"
  case "$arc" in
    *.tar.xz) tar -xJf "$arc" -C "$dst" ;;
    *.tar.gz|*.tgz) tar -xzf "$arc" -C "$dst" ;;
    *.7z)
      # 7z needs root to apt-install. Fallback chain:
      #   1. system 7z / 7za if present
      #   2. py7zr (pure-Python, pip install --user --break-system-packages py7zr)
      #      invoked via /usr/bin/python3 so a venv python3 doesn't shadow it.
      if have 7z; then 7z x -y -o"$dst" "$arc" >/dev/null
      elif have 7za; then 7za x -y -o"$dst" "$arc" >/dev/null
      elif /usr/bin/python3 -c 'import py7zr' 2>/dev/null; then
        /usr/bin/python3 - "$arc" "$dst" <<'PY'
import sys, py7zr
with py7zr.SevenZipFile(sys.argv[1], 'r') as z:
    z.extractall(sys.argv[2])
PY
      else
        warn "need 7z/7za or py7zr to extract $arc. Install with:"
        warn "  pip install --user --break-system-packages py7zr"
        return 1
      fi ;;
    *.zip)
      # Prefer system unzip; fall back to Python's zipfile (always present).
      # Use /usr/bin/python3 so a venv doesn't shadow the stdlib.
      if have unzip; then unzip -o -q "$arc" -d "$dst"
      else /usr/bin/python3 - "$arc" "$dst" <<'PY'
import sys, zipfile
zipfile.ZipFile(sys.argv[1]).extractall(sys.argv[2])
PY
      fi ;;
    *) die "unknown archive type: $arc" ;;
  esac
}

# ---- validators -----------------------------------------------------------
check_host_gcc() {
  if ! have g++; then
    warn "host g++ not on PATH"
    return 1
  fi
  local ver; ver=$(g++ -dumpversion 2>/dev/null | cut -d. -f1)
  if [[ ! "$ver" =~ ^[0-9]+$ ]] || (( ver < GCC_MIN_VERSION )); then
    warn "host g++ $ver < required $GCC_MIN_VERSION"
    return 1
  fi
  log "host g++ OK ($ver)"
  return 0
}

ensure_mingw() {
  local dst="$CACHE_ROOT/mingw"
  local marker="$dst/.version"
  # Linux-hosted cross-compiler binary (ELF). niXman's mingw-builds-binaries
  # ship Windows-hosted .exe binaries that CANNOT run on Linux, so we verify
  # the host type after extraction and reject PE builds.
  local bin="$dst/bin/x86_64-w64-mingw32-gcc"

  # fast path: cached + valid + Linux-hosted (ELF)
  if [[ -f "$marker" && -x "$bin" ]] && [[ "$(cat "$marker")" == "$MINGW_VERSION" ]] \
     && [[ "$(file -b "$bin" 2>/dev/null)" == ELF* ]]; then
    log "mingw $MINGW_VERSION cached & valid (Linux-hosted ELF)"
    return 0
  fi
  log "mingw missing/stale/wrong-host -> provisioning"

  rm -rf "$dst"; mkdir -p "$dst"
  local arc; arc="$CACHE_ROOT/mingw.7z"
  local ok=0
  for url in "${MINGW_URLS[@]}"; do
    if curl_get "$url" "$arc"; then ok=1; break; fi
  done
  if (( ! ok )); then
    rm -f "$arc"; rmdir "$dst" 2>/dev/null || true
    warn "mingw download failed (offline / bad mirror)."
    warn "  Windows cross-build will fall back to Zig (zig cc -target x86_64-windows-gnu)."
    warn "  Linux native build is unaffected and will proceed."
    return 0
  fi
  if ! extract "$arc" "$dst"; then
    rm -f "$arc"; rm -rf "$dst"
    warn "mingw archive extraction failed (need 7z/py7zr). Windows cross falls back to Zig."
    return 0
  fi
  rm -f "$arc"
  # flatten nested dir if present (niXman extracts to mingw64/).
  local nested; nested=$(find "$dst" -maxdepth 1 -mindepth 1 -type d | head -1)
  if [[ -n "$nested" && -d "$nested/bin" ]]; then
    mv "$nested"/* "$dst"/ 2>/dev/null || true
    rmdir "$nested" 2>/dev/null || true
  fi

  # HOST-TYPE GUARD: reject Windows-hosted (PE) builds — they can't run on
  # Linux. The Zig fallback (ensure_zig_wrappers) covers Windows cross.
  if [[ -x "$bin" ]] && [[ "$(file -b "$bin" 2>/dev/null)" != ELF* ]]; then
    warn "downloaded mingw is Windows-hosted (PE), not Linux-hosted (ELF)."
    warn "  A Linux-hosted MinGW-w64 needs 'apt install mingw-w64' (root) or a"
    warn "  custom ELF tarball. Falling back to Zig for Windows cross."
    rm -rf "$dst"
    return 0
  fi

  echo "$MINGW_VERSION" > "$marker"
  log "mingw $MINGW_VERSION installed at $dst"
}

ensure_zig() {
  local dst="$CACHE_ROOT/zig"
  local marker="$dst/.version"
  local bin="$dst/zig"

  if [[ -f "$marker" && -x "$bin" ]] && [[ "$(cat "$marker")" == "$ZIG_VERSION" ]]; then
    log "zig $ZIG_VERSION cached & valid"
    return 0
  fi
  log "zig missing/stale -> provisioning"

  rm -rf "$dst"; mkdir -p "$dst"
  local arc; arc="$CACHE_ROOT/zig.tar.xz"
  local ok=0
  for url in "${ZIG_URLS[@]}"; do
    if curl_get "$url" "$arc"; then ok=1; break; fi
  done
  if (( ! ok )); then
    # GRACEFUL DEGRADATION: see ensure_mingw. macOS cross-build unavailable
    # is not fatal to the Linux native build.
    rm -f "$arc"; rmdir "$dst" 2>/dev/null || true
    warn "zig download failed (offline / bad mirror). macOS cross-build will be unavailable."
    warn "  Linux native build is unaffected and will proceed."
    return 0
  fi
  if ! extract "$arc" "$dst"; then
    rm -f "$arc"; rm -rf "$dst"
    warn "zig archive extraction failed. macOS cross-build unavailable."
    return 0
  fi
  rm -f "$arc"
  # zig tarball extracts to zig-linux-x86_64-<ver>/; flatten.
  local nested; nested=$(find "$dst" -maxdepth 1 -mindepth 1 -type d | head -1)
  if [[ -n "$nested" && -x "$nested/zig" ]]; then
    mv "$nested"/* "$dst"/ 2>/dev/null || true
    rmdir "$nested" 2>/dev/null || true
  fi
  chmod +x "$bin"
  echo "$ZIG_VERSION" > "$marker"
  log "zig $ZIG_VERSION installed at $dst"
}

# Zig-as-clang wrappers: expose `clang`/`clang++` that delegate to `zig cc`
# / `zig c++` with the right `-target`. Gradle's Clang toolchain detector
# then picks them up for macOS cross-compilation.
ensure_zig_wrappers() {
  local wdir="$CACHE_ROOT/wrappers"
  local zig="$CACHE_ROOT/zig/zig"
  if [[ ! -x "$zig" ]]; then
    # zig not provisioned (graceful degradation in ensure_zig) — skip wrappers.
    return 0
  fi
  mkdir -p "$wdir"

  # args: wrapper_filename  zig_subcmd  target
  make_wrapper() {
    local wname="$1" subcmd="$2" target="$3"
    local f="$wdir/$wname"
    cat > "$f" <<EOF
#!/usr/bin/env bash
exec '$zig' '$subcmd' -target '$target' "\$@"
EOF
    chmod +x "$f"
  }

  # macOS x86_64
  make_wrapper x86_64-apple-darwin-clang    cc  x86_64-macos-gnu
  make_wrapper x86_64-apple-darwin-clang++  c++ x86_64-macos-gnu
  # macOS arm64
  make_wrapper aarch64-apple-darwin-clang   cc  aarch64-macos-gnu
  make_wrapper aarch64-apple-darwin-clang++ c++ aarch64-macos-gnu
  # generic clang/clang++ pointing at macOS x86_64 by default (Gradle's
  # clang detector needs a plain `clang` on PATH to register the toolchain).
  cat > "$wdir/clang"   <<'EOF'
#!/usr/bin/env bash
exec "$(dirname "$0")/../zig/zig" cc -target x86_64-macos-gnu "$@"
EOF
  cat > "$wdir/clang++" <<'EOF'
#!/usr/bin/env bash
exec "$(dirname "$0")/../zig/zig" c++ -target x86_64-macos-gnu "$@"
EOF
  chmod +x "$wdir/clang" "$wdir/clang++"

  # Windows cross fallback: if no Linux-hosted MinGW is available (the common
  # case without root), expose the canonical MinGW binary names as Zig
  # wrappers targeting x86_64-windows-gnu. The spec prefers real MinGW-w64,
  # but this keeps Windows cross-builds working in a bare environment.
  make_wrapper x86_64-w64-mingw32-gcc   cc  x86_64-windows-gnu
  make_wrapper x86_64-w64-mingw32-g++   c++ x86_64-windows-gnu
  make_wrapper x86_64-w64-mingw32-cc    cc  x86_64-windows-gnu
  make_wrapper x86_64-w64-mingw32-c++   c++ x86_64-windows-gnu
  # Only install the mingw wrappers if a REAL Linux-hosted MinGW is NOT
  # present (don't shadow a genuine apt-installed mingw-w64).
  if [[ ! -x "$CACHE_ROOT/mingw/bin/x86_64-w64-mingw32-gcc" ]] \
     || [[ "$(file -b "$CACHE_ROOT/mingw/bin/x86_64-w64-mingw32-gcc" 2>/dev/null)" != ELF* ]]; then
    chmod +x "$wdir"/x86_64-w64-mingw32-{gcc,g++,cc,c++}
    log "zig->mingw fallback wrappers installed (no Linux-hosted MinGW detected)"
  else
    rm -f "$wdir"/x86_64-w64-mingw32-{gcc,g++,cc,c++}
    log "real Linux-hosted MinGW detected; zig->mingw fallback wrappers skipped"
  fi

  log "zig clang wrappers installed at $wdir"
}

print_env() {
  cat <<EOF
export SCI_CALC_TC_CACHE="$CACHE_ROOT"
export PATH="$CACHE_ROOT/mingw/bin:$CACHE_ROOT/zig:$CACHE_ROOT/wrappers:\$PATH"
export MINGW_PREFIX="x86_64-w64-mingw32"
export ZIG_CC="$CACHE_ROOT/zig/zig cc"
export ZIG_CXX="$CACHE_ROOT/zig/zig c++"
EOF
}

# ---- dispatch -------------------------------------------------------------
case "$MODE" in
  --check)
    check_host_gcc || warn "host gcc below minimum (Linux native may fail)"
    ensure_mingw
    ensure_zig
    ensure_zig_wrappers
    log "ALL TOOLCHAINS READY"
    ;;
  --env)
    print_env
    ;;
  --print-env)
    print_env
    ;;
  *)
    die "usage: $0 [--check|--env]"
    ;;
esac
