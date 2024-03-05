#!/bin/bash
#
[ -n "$PYTHON" ] || PYTHON=python3
[ -n "$IWYU_TOOL" ] || IWYU_TOOL=iwyu_tool.py
PROJECT_DIR="$($PYTHON -c 'import os.path, sys; print(os.path.abspath(os.path.join(sys.argv[1], os.path.pardir)))' -- "$0")"
$IWYU_TOOL -p "$PROJECT_DIR/builddir" "$PROJECT_DIR" -- -Xiwyu --mapping_file="$PROJECT_DIR/tools/iwyu_mapping.imp"
