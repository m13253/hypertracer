#!/bin/bash
#
[ -n "$PYTHON" ] || PYTHON=python3
PROJECT_DIR="$($PYTHON -c 'import os.path, sys; print(os.path.abspath(os.path.join(sys.argv[1], os.path.pardir)))' -- "$0")"
iwyu_tool.py -p "$PROJECT_DIR/builddir" "$PROJECT_DIR" -- -Xiwyu --mapping_file="$PROJECT_DIR/tools/iwyu_mapping.imp"
