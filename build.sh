#!/usr/bin/env bash
# Builds BF Freecam.
#   ./build.sh          -> builds inside the official devkitA64 Docker image (needs git + Docker)
#   ./build.sh --local  -> builds with a local devkitPro install (needs git, make, $DEVKITPRO)
# Output: dist/emulator/BFFreecam/exefs/{subsdk9,main.npdm}
#         dist/atmosphere/atmosphere/contents/010028600EBDA000/exefs/{subsdk9,main.npdm}
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXLAUNCH_REPO="https://github.com/shadowninja108/exlaunch.git"
EXLAUNCH_COMMIT="f698816d6e198afb0029ad5c07d55e7017a620fe"
PINNED_IMAGE="devkitpro/devkita64@sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282"
FALLBACK_IMAGE="devkitpro/devkita64:latest"
TITLE_ID="010028600EBDA000"
WORK="$ROOT/.build/exlaunch"
DIST="$ROOT/dist"

MODE="docker"
if [[ "${1:-}" == "--local" ]]; then MODE="local"; fi

echo "==> Fetching ExLaunch @ ${EXLAUNCH_COMMIT:0:7}"
rm -rf "$ROOT/.build"
mkdir -p "$ROOT/.build"
git clone --quiet "$EXLAUNCH_REPO" "$WORK"
git -C "$WORK" checkout --quiet "$EXLAUNCH_COMMIT"

echo "==> Applying freecam sources"
cp -R "$ROOT/src/." "$WORK/"

echo "==> Building ($MODE)"
if [[ "$MODE" == "docker" ]]; then
  IMAGE="$PINNED_IMAGE"
  if ! docker pull --quiet "$IMAGE" >/dev/null 2>&1; then
    echo "    pinned image unavailable, using $FALLBACK_IMAGE"
    IMAGE="$FALLBACK_IMAGE"
  fi
  USER_ARGS=()
  if [[ "$(uname -s)" == "Linux" ]]; then USER_ARGS=(--user "$(id -u):$(id -g)"); fi
  docker run --rm ${USER_ARGS[@]+"${USER_ARGS[@]}"} -v "$ROOT:/work" -w /work/.build/exlaunch "$IMAGE" \
    bash -lc 'make -j"$(nproc)"'
else
  : "${DEVKITPRO:?Set DEVKITPRO to your devkitPro install, e.g. export DEVKITPRO=/opt/devkitpro}"
  make -C "$WORK" -j"$(nproc 2>/dev/null || echo 4)"
fi

for f in subsdk9 main.npdm; do
  [[ -f "$WORK/deploy/$f" ]] || { echo "Build output missing: deploy/$f" >&2; exit 1; }
done

echo "==> Packaging"
rm -rf "$DIST"
EMU="$DIST/emulator/BFFreecam/exefs"
ATMO="$DIST/atmosphere/atmosphere/contents/$TITLE_ID/exefs"
mkdir -p "$EMU" "$ATMO"
for f in subsdk9 main.npdm; do
  cp "$WORK/deploy/$f" "$EMU/$f"
  cp "$WORK/deploy/$f" "$ATMO/$f"
done

if command -v zip >/dev/null 2>&1; then
  (cd "$DIST/emulator" && zip -qr ../BFFreecam-emulator.zip BFFreecam)
  (cd "$DIST/atmosphere" && zip -qr ../BFFreecam-atmosphere.zip atmosphere)
fi

echo "==> Done. Files are in: $DIST"
