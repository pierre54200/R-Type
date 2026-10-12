set -euo pipefail

SRC_DIR=/app
BUILD_DIR=/build/coverage

echo "[coverage] compilation avec instrumentation..."
cmake -S "$SRC_DIR" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCOVERAGE=ON > /dev/null
cmake --build "$BUILD_DIR" -j"$(nproc)"

if [ ! -x "$BUILD_DIR/tests" ]; then
    echo "[coverage] aucun test compile : ajoute un fichier .spec.cpp ou .test.cpp"
    exit 1
fi

find "$BUILD_DIR" -name '*.gcda' -delete

echo "[coverage] lancement des tests..."
"$BUILD_DIR/tests" --gtest_brief=1 || echo "[coverage] des tests echouent, couverture calculee quand meme"

echo
gcovr \
    --root "$SRC_DIR" \
    --filter "$SRC_DIR/src/" \
    --exclude '.*\.(spec|test)\.(cpp|cc|cxx)$' \
    --exclude "$SRC_DIR/src/main.cpp" \
    --txt \
    --print-summary \
    "$BUILD_DIR"