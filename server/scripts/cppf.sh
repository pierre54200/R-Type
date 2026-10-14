#
# Generateur de fichiers C++ facon Angular CLI.
#
# Usage :
#   npm run g c Test/test       -> server/src/Test/test.component.{hpp,cpp,test.cpp}
#   npm run g s game/score
#   npm run g cl utils/vector2 --dry-run
#   npm run g c ui/menu --skip-tests  -> sans le fichier de test
#
# La configuration (types, suffixes, templates) est dans app/scripts/cppf.json.
# Necessite jq (apt install jq / brew install jq).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
CONFIG="$SCRIPT_DIR/cppf.json"

die() { echo "erreur : $*" >&2; exit 1; }

usage() {
    sed -n '3,11p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
    exit 1
}

DRY_RUN=0
SKIP_TESTS=0
ARGS=()
for a in "$@"; do
    case "$a" in
        --dry-run) DRY_RUN=1 ;;
        --skip-tests) SKIP_TESTS=1 ;;
        --help|-h) usage ;;
        --*) die "option inconnue : $a" ;;
        *) ARGS+=("$a") ;;
    esac
done

# Options passees via npm sans "--" (npm run g c test --skip-tests)
[ "${npm_config_skip_tests:-}" = "true" ] && SKIP_TESTS=1
[ "${npm_config_dry_run:-}" = "true" ] && DRY_RUN=1

[ "${#ARGS[@]}" -eq 3 ] || usage
case "${ARGS[0]}" in g|generate) ;; *) usage ;; esac
TYPE_ARG="${ARGS[1]}"
PATH_ARG="${ARGS[2]}"

command -v jq >/dev/null || die "jq n'est pas installe (apt install jq / brew install jq)"
[ -f "$CONFIG" ] || die "cppf.json introuvable dans server/scripts"

TYPE=$(jq -r --arg t "$TYPE_ARG" '
    .schematics | to_entries[]
    | select(.key == $t or .value.alias == $t) | .key' "$CONFIG" | head -n 1)

if [ -z "$TYPE" ]; then
    available=$(jq -r '.schematics | to_entries
        | map(if .value.alias then "\(.key) (\(.value.alias))" else .key end)
        | join(", ")' "$CONFIG")
    die "type inconnu '$TYPE_ARG'. Types disponibles : $available"
fi

cfg() { jq -r --arg t "$TYPE" "$1 // empty" "$CONFIG"; }

SRC_ROOT="$ROOT/$(cfg '.sourceRoot')"
TPL_DIR="$SCRIPT_DIR/$(cfg '.templatesDir')"
SUFFIX="$(cfg '.schematics[$t].suffix')"
CLASS_SUFFIX="$(cfg '.schematics[$t].classSuffix')"

PATH_ARG="${PATH_ARG#/}"
PATH_ARG="${PATH_ARG%/}"
[ -n "$PATH_ARG" ] || die "chemin vide"
case "/$PATH_ARG/" in */../*) die "chemin invalide : '$PATH_ARG'" ;; esac

REL_DIR="$(dirname "$PATH_ARG")"
[ "$REL_DIR" = "." ] && REL_DIR=""
RAW_NAME="$(basename "$PATH_ARG")"

NAME=$(printf '%s' "$RAW_NAME" \
    | sed -E 's/([a-z0-9])([A-Z])/\1-\2/g' \
    | tr '_ ' '--' | tr '[:upper:]' '[:lower:]' \
    | sed -E 's/-+/-/g; s/^-|-$//g')

[[ "$NAME" =~ ^[a-z][a-z0-9-]*$ ]] || die "nom invalide : '$RAW_NAME' (lettres, chiffres, - ou _, et commencer par une lettre)"

CLASS_NAME=""
IFS='-' read -ra PARTS <<< "$NAME"
for p in "${PARTS[@]}"; do CLASS_NAME+="${p^}"; done
CLASS_NAME+="$CLASS_SUFFIX"

FILE_STEM="$NAME${SUFFIX:+.$SUFFIX}"
OUT_DIR="$SRC_ROOT${REL_DIR:+/$REL_DIR}"
INCLUDE_PATH="${REL_DIR:+$REL_DIR/}$FILE_STEM.hpp"
GUARD="$(printf '%s' "$INCLUDE_PATH" | tr -c '[:alnum:]' '_' | tr '[:lower:]' '[:upper:]')_"

# Tous les fichiers gardent le suffixe du type : test.component.hpp, test.component.test.cpp
target_for() {
    printf '%s/%s.%s' "$OUT_DIR" "$FILE_STEM" "$1"
}

render() {
    local c
    c="$(cat "$1")"
    c="${c//\{\{ClassName\}\}/$CLASS_NAME}"
    c="${c//\{\{fileName\}\}/$FILE_STEM}"
    c="${c//\{\{name\}\}/$NAME}"
    c="${c//\{\{header\}\}/$FILE_STEM.hpp}"
    c="${c//\{\{includePath\}\}/$INCLUDE_PATH}"
    c="${c//\{\{guard\}\}/$GUARD}"
    c="${c//\{\{type\}\}/$TYPE}"
    local left
    left=$(grep -o '{{[^}]*}}' <<< "$c" | head -n 1 || true)
    [ -z "$left" ] || die "variable de template inconnue dans $(basename "$1") : $left"
    printf '%s\n' "$c"
}

mapfile -t EXTS < <(jq -r --arg t "$TYPE" --argjson skip "$SKIP_TESTS" '
    .schematics[$t].files | keys_unsorted[]
    | select($skip == 0 or ((startswith("spec") or startswith("test")) | not))' "$CONFIG")
[ "${#EXTS[@]}" -gt 0 ] || die "aucun fichier defini pour le type '$TYPE' dans cppf.json"

for ext in "${EXTS[@]}"; do
    tpl="$TPL_DIR/$(jq -r --arg t "$TYPE" --arg e "$ext" '.schematics[$t].files[$e]' "$CONFIG")"
    [ -f "$tpl" ] || die "template introuvable : ${tpl#"$ROOT"/}"
    target="$(target_for "$ext")"
    [ ! -e "$target" ] || die "${target#"$ROOT"/} existe deja, rien n'a ete cree"
    render "$tpl" > /dev/null
done

for ext in "${EXTS[@]}"; do
    tpl="$TPL_DIR/$(jq -r --arg t "$TYPE" --arg e "$ext" '.schematics[$t].files[$e]' "$CONFIG")"
    target="$(target_for "$ext")"
    rel="${target#"$ROOT"/}"
    if [ "$DRY_RUN" -eq 1 ]; then
        echo "CREATE $rel (dry-run)"
    else
        mkdir -p "$OUT_DIR"
        render "$tpl" > "$target"
        echo "CREATE $rel ($(wc -c < "$target" | tr -d ' ') bytes)"
    fi
done
