@echo off
set FSL_GLES_EMULATOR_PATH=c:\AdrenoSDK
set FSL_GLES_INCLUDE_PATH=%FSL_GLES_EMULATOR_PATH%\Development\Inc
set FSL_GLES_LIB_PATH=%FSL_GLES_EMULATOR_PATH%\Development\Lib\x64
set FSL_GLES_BIN_PATH=%FSL_GLES_EMULATOR_PATH%\Bin
set FSL_GLES_LIB_EGL=libEGL.lib
set FSL_GLES_LIB_GLES=libGLESv2.lib
set FSL_GLES_NAME=Qualcomm
set PATH=%FSL_GLES_BIN_PATH%;FSL_GLES_BIN_PATH\x64;%PATH%
