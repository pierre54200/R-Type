#!/usr/bin/env bash
#
# Compile et lance le serveur, puis recompile et relance a chaque modification.
# Tourne directement sur la machine, sans Docker.
#
# Usage :
#   ./watch.sh                 (ou npm start)
#   RUN_TESTS=0 ./watch.sh     pour ne pas lancer les tests a chaque build
#
# Surveillance : inotifywait (Linux, paquet inotify-tools) ou fswatch (macOS,
# brew install fswatch). Sans l'un des deux, le script scrute les fichiers chaque seconde.

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
SRC_DIR="$ROOT/server"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
BIN="$BUILD_DIR/r-type_server"
TESTS_BIN="$BUILD_DIR/tests"
RUN_TESTS="${RUN_TESTS:-1}"
WATCH_REGEX='(\.(cpp|cc|cxx|hpp|h|hh|tpp|ipp)|CMakeLists\.txt|\.cmake)$'
FIND_FILTER=( \( -name '*.cpp' -o -name '*.cc' -o -name '*.cxx'
                 -o -name '*.hpp' -o -name '*.h' -o -name '*.hh'
                 -o -name '*.tpp' -o -name '*.ipp'
                 -o -name 'CMakeLists.txt' -o -name '*.cmake' \) )
PID=""
WATCHER_PID=""

log() { printf '[watch] %s\n' "$*"; }
die() { printf '[watch] erreur : %s\n' "$*" >&2; exit 1; }

command -v cmake >/dev/null || die "cmake n'est pas installe"
[ -f "$ROOT/CMakeLists.txt" ] || die "CMakeLists.txt introuvable a la racine"

# Variables d'environnement du projet (SERVER_PORT...)
if [ -f "$ROOT/.env" ]; then
    set -a
    # shellcheck disable=SC1091
    . "$ROOT/.env"
    set +a
fi

GENERATOR=()
command -v ninja >/dev/null && GENERATOR=(-G Ninja)
JOBS="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

if command -v inotifywait >/dev/null && inotifywait --help 2>&1 | grep -q -- '--include'; then
    WATCHER=inotify
elif command -v fswatch >/dev/null; then
    WATCHER=fswatch
else
    WATCHER=poll
    log "ni inotifywait (>= 3.20) ni fswatch trouve, scrutation toutes les secondes"
fi

# ---------- build ----------

configure() {
    log "configuration CMake..."
    cmake -S "$ROOT" -B "$BUILD_DIR" "${GENERATOR[@]}" -DCMAKE_BUILD_TYPE=Debug
}

build() {
    [ -f "$BUILD_DIR/CMakeCache.txt" ] || configure || return 1
    log "compilation..."
    cmake --build "$BUILD_DIR" -j"$JOBS"
}

run_tests() {
    [ "$RUN_TESTS" = "1" ] || return 0
    if [ ! -x "$TESTS_BIN" ] \
        || [ -z "$(find "$SRC_DIR/src" -type f \( -name '*.spec.c*' -o -name '*.test.c*' \) -print -quit)" ]; then
        log "aucun test"
        return 0
    fi
    log "tests..."
    if GTEST_COLOR=yes "$TESTS_BIN" --gtest_brief=1; then
        log "tests OK"
    else
        log "TESTS EN ECHEC (le serveur est relance quand meme)"
    fi
}

# ---------- serveur ----------

start_server() {
    (cd "$ROOT" && exec "$BIN") &
    PID=$!
    log "serveur lance (pid $PID)"
}

stop_server() {
    [ -n "$PID" ] || return 0
    if kill -0 "$PID" 2>/dev/null; then
        kill "$PID" 2>/dev/null
        # 3 secondes pour s'arreter proprement, sinon SIGKILL
        for _ in $(seq 30); do
            kill -0 "$PID" 2>/dev/null || break
            sleep 0.1
        done
        kill -0 "$PID" 2>/dev/null && kill -9 "$PID" 2>/dev/null
    fi
    wait "$PID" 2>/dev/null
    PID=""
}

# ---------- surveillance ----------

snapshot() {
    find "$SRC_DIR" "$ROOT/CMakeLists.txt" -type f "${FIND_FILTER[@]}" -print 2>/dev/null | sort
}

wait_poll() {
    local marker="$BUILD_DIR/.watch-stamp" before
    touch "$marker"
    before="$(snapshot)"
    while :; do
        sleep 1
        [ "$(snapshot)" != "$before" ] && return 0
        [ -n "$(find "$SRC_DIR" "$ROOT/CMakeLists.txt" -type f "${FIND_FILTER[@]}" -newer "$marker" -print -quit 2>/dev/null)" ] && return 0
    done
}

wait_change() {
    case "$WATCHER" in
        inotify)
            inotifywait -q -r -e close_write,create,delete,move \
                --include "$WATCH_REGEX" "$SRC_DIR" "$ROOT/CMakeLists.txt" >/dev/null 2>&1 &
            ;;
        fswatch)
            fswatch -1 -r -E -e '.*' -i "$WATCH_REGEX" "$SRC_DIR" "$ROOT/CMakeLists.txt" >/dev/null 2>&1 &
            ;;
        poll)
            wait_poll &
            ;;
    esac
    WATCHER_PID=$!
    wait "$WATCHER_PID"
    WATCHER_PID=""
    # Laisse l'editeur finir d'ecrire (sauvegardes en plusieurs etapes)
    sleep 0.3
}

cleanup() {
    [ -n "$WATCHER_PID" ] && kill "$WATCHER_PID" 2>/dev/null
    stop_server
    log "arret"
    exit 0
}
trap cleanup INT TERM

# ---------- boucle principale ----------

mkdir -p "$BUILD_DIR"

if build; then
    run_tests
    start_server
else
    log "echec de la compilation initiale, en attente de corrections..."
fi

while :; do
    wait_change
    log "changement detecte"
    if build; then
        run_tests
        stop_server
        start_server
    else
        log "echec de la compilation, l'ancien serveur reste actif"
    fi
done