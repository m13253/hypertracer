#!/bin/bash

set -e
[ -n "$PYTHON" ] || PYTHON=python3
PROJECT_DIR="$($PYTHON -c 'import os.path, sys; print(os.path.abspath(os.path.join(sys.argv[1], os.path.pardir, os.path.pardir)))' "$0")"
for i in *.trace
do
    OUTPUT="$(basename "$i" .trace).json"
    echo "Decoding $i -> $OUTPUT"
    $PYTHON "$PROJECT_DIR/tools/decode-trace.py" -o "$OUTPUT" -- "$i"
done
