#!/bin/bash
# Watches src/ for changes and re-runs the linter.
# Used by the VS Code background task for inline error reporting.

cd "$(dirname "$0")/.."
SRC_DIR="src"

echo "Enatrio lint watcher started"

# Initial run
uv run --with tree-sitter --with tree-sitter-c python3 tools/lint.py "$SRC_DIR" --warn 2>&1
echo "[lint-watch] scan complete"

# Watch for changes using inotifywait if available, otherwise poll
if command -v inotifywait > /dev/null 2>&1; then
    while true; do
        inotifywait -r -q -e modify,create,delete --include '\.(c|h)$' "$SRC_DIR" > /dev/null 2>&1
        sleep 0.3
        uv run --with tree-sitter --with tree-sitter-c python3 tools/lint.py "$SRC_DIR" --warn 2>&1
        echo "[lint-watch] scan complete"
    done
else
    # Fallback: poll every 3 seconds
    LAST_HASH=""
    while true; do
        sleep 3
        HASH=$(find "$SRC_DIR" -name '*.c' -o -name '*.h' | sort | xargs md5sum 2>/dev/null | md5sum)
        if [ "$HASH" != "$LAST_HASH" ]; then
            LAST_HASH="$HASH"
            uv run --with tree-sitter --with tree-sitter-c python3 tools/lint.py "$SRC_DIR" --warn 2>&1
            echo "[lint-watch] scan complete"
        fi
    done
fi
