# CLI tool to sync folders between two locations.
import os
import shutil
import sys

# Sample command to copy from src to dst:
# python sync_vscode_to_stm32.py ./final_project ../../../STM32CubeIDE/workspace_1.19.0/final_project

# Sample command to copy back HAL updates (will overwrite any uncommitted local changes):
# python sync_vscode_to_stm32.py ./final_project ../../../STM32CubeIDE/workspace_1.19.0/final_project --reverse
src_folder = sys.argv[1]
dst_folder = sys.argv[2]

reverse = False
if len(sys.argv) > 3 and sys.argv[3] == "--reverse":
    reverse = True

if reverse:
    src, dst = dst_folder, src_folder
    print(f"Copying {src} -> {dst}")
else:
    src, dst = src_folder, dst_folder
    print(f"Copying {src} -> {dst}")

if not os.path.exists(src):
    print(f"Source folder '{src}' does not exist.")
    sys.exit(1)

if os.path.exists(dst):
    shutil.rmtree(dst)
shutil.copytree(src, dst)
print("Done.")
