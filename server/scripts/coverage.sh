#!/usr/bin/env bash
# Mesure de couverture des tests (necessite gcovr : pip install gcovr / apt install gcovr)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
SRC_DIR="$ROOT/server"
BUILD_DIR="$ROOT/build-coverage"

command -v gcovr >/dev/null || { echo "[coverage] gcovr n'est pas installe" >&2; exit 1; }

GENERATOR=()
command -v ninja >/dev/null && GENERATOR=(-G Ninja)
JOBS="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

echo "[coverage] compilation avec instrumentation..."
cmake -S "$ROOT" -B "$BUILD_DIR" "${GENERATOR[@]}" -DCMAKE_BUILD_TYPE=Debug -DCOVERAGE=ON > /dev/null
cmake --build "$BUILD_DIR" -j"$JOBS"

if [ ! -x "$BUILD_DIR/tests" ]; then
    echo "[coverage] aucun test compile : ajoute un fichier .spec.cpp ou .test.cpp"
    exit 1
fi

find "$BUILD_DIR" -name '*.gcda' -delete

echo "[coverage] lancement des tests..."
"$BUILD_DIR/tests" --gtest_brief=1 || echo "[coverage] des tests echouent, couverture calculee quand meme"

echo
gcovr \
    --root "$ROOT" \
    --filter "$SRC_DIR/src/" \
    --exclude '.*\.(spec|test)\.(cpp|cc|cxx)$' \
    --exclude "$SRC_DIR/src/main.cpp" \
    --txt \
    --print-summary \
    "$BUILD_DIR"