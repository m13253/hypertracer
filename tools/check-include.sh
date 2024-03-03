#!/bin/bash

iwyu_tool.py -p ../builddir .. -- -Xiwyu --mapping_file="$PWD/iwyu_mapping.imp"
