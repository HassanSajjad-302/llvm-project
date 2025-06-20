#!/usr/bin/env python3
import os
import shutil

copy_from = ('../' * 4) + 'ipc2978api/'

# Determine the directory where this script resides (include/clang/IPC2978)
source_dir = os.path.abspath(os.path.dirname(__file__))
# Compute the corresponding source directory: ../../../.. + /lib/IPC2978
include_dir = os.path.abspath(os.path.join(source_dir, '../../include/clang/IPC2978'))


shutil.copytree(copy_from + 'include', include_dir, dirs_exist_ok=True)
shutil.copytree(copy_from + 'src', source_dir, dirs_exist_ok=True)
# We'll process files in both include and source directories
roots = [source_dir, include_dir]

# Gather all header filenames in the include directory (top-level only)
include_files = [f for f in os.listdir(include_dir) if f.endswith(".hpp")]

files = []

# Iterate through the source and include directories
for root in roots:
    # Skipping the CMakeLists.txt and .py files
    files.extend([os.path.join(root, x) for x in os.listdir(root) if
                  not os.path.join(root, x).endswith(".txt") and not os.path.join(root, x).endswith(".py")])

for file in files:
    out_lines = []
    with open(file, 'r', encoding='utf-8') as f:
        lines = f.readlines()
        # Examine each line for an include directive
        for line in lines:
            if line.startswith('#include "'):
                out_lines.append(line.replace('#include "', '#include "clang/IPC2978/', 1))
            else:
                out_lines.append(line)
    with open(file, 'w', encoding='utf-8') as f:
        for line in out_lines:
            f.writelines(line)
