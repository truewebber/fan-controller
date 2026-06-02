#!/usr/bin/env bash
set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
BASE="$REPO/config/config.yaml"
LOCAL="$REPO/config/config.local.yaml"
DST="$REPO/data/config.json"

if [ -f "$LOCAL" ]; then
    yq -o json '. * load("'"$LOCAL"'")' "$BASE" > "$DST"
    echo "Written: $DST (merged with config.local.yaml)"
else
    yq -o json . "$BASE" > "$DST"
    echo "Written: $DST"
fi
