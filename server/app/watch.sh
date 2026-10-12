#!/usr/bin/env bash
# watch.sh pour une compilation automatique au runtime
set -u

SRC_DIR=/app
BUILD_DIR=/build 
BIN="$BUILD_DIR/server"
TESTS_BIN="$BUILD_DIR/tests"
RUN_TESTS="${RUN_TESTS:-1}"
PID=""

configure() {
    echo "[watch] configuration CMake..."
    cmake -S "$SRC_DIR" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Debug
}

build() {
    echo "[watch] compilation..."
    cmake --build "$BUILD_DIR" -j"$(nproc)"
}

run_tests() {
    [ "$RUN_TESTS" = "1" ] || return 0
    if [ ! -x "$TESTS_BIN" ] || ! find "$SRC_DIR/src" -name '*.spec.c*' | grep -q .; then
        echo "[watch] aucun test"
        return 0
    fi
    echo "[watch] tests..."
    if GTEST_COLOR=yes "$TESTS_BIN" --gtest_brief=1; then
        echo "[watch] tests OK"
    else
        echo "[watch] TESTS EN ECHEC (le serveur est relance quand meme)"
    fi
}

stop() {
    if [ -n "$PID" ] && kill -0 "$PID" 2>/dev/null; then
        kill "$PID"
        wait "$PID" 2>/dev/null
    fi
    PID=""
}

start() {
    "$BIN" &
    PID=$!
    echo "[watch] serveur lance (pid $PID)"
}

trap 'stop; exit 0' INT TERM

if [ ! -f "$BUILD_DIR/build.ninja" ]; then
    configure || echo "[watch] echec de la configuration CMake"
fi

if build; then
    run_tests
    start
else
    echo "[watch] echec de la compilation initiale, en attente de corrections..."
fi

while true; do
    inotifywait -q -r -e close_write,create,delete,move \
        --include '(\.(cpp|cc|cxx|hpp|h|hh|tpp|ipp)|CMakeLists\.txt|\.cmake)$' "$SRC_DIR" >/dev/null &
    wait $!
    sleep 0.2

    [ -f "$BUILD_DIR/build.ninja" ] || configure

    if build; then
        run_tests
        stop
        start
    else
        echo "[watch] echec de la compilation, l'ancien serveur reste actif"
    fi
done