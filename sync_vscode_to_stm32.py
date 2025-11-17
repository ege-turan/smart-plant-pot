# Copies contents of vscode final_project folder to stm32 final_project folder
VSCODE_FOLDER = "./final_project"
STM32_FOLDER = "../../../STM32CubeIDE/workspace_1.19.0/final_project"
VSCODE_TO_STM32 = True

import shutil

if VSCODE_TO_STM32:
    # Delete contents of STM32_FOLDER
    shutil.rmtree(STM32_FOLDER)
    # Copy contents of VSCODE_FOLDER to STM32_FOLDER
    shutil.copytree(VSCODE_FOLDER, STM32_FOLDER)
else:
    # Delete contents of VSCODE_FOLDER
    shutil.rmtree(VSCODE_FOLDER)
    # Copy contents of STM32_FOLDER to VSCODE_FOLDER
    shutil.copytree(STM32_FOLDER, VSCODE_FOLDER)
