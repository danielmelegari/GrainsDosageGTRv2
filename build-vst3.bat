@echo off
rem ============================================================
rem  GrainsDosage - build the .vst3 bundle locally, NO installer.
rem  Requirements: CMake >= 3.25, Visual Studio 2022 (C++), git.
rem  The Steinberg VST3 SDK is cloned into %TEMP% and never
rem  touches your system. Result: build\VST3\Release\GrainsDosage.vst3
rem ============================================================
setlocal
cd /d "%~dp0"

where cmake >nul 2>&1 || (echo [ERROR] cmake not found in PATH. Install "cmake" from https://cmake.org/download/ & exit /b 1)
where git   >nul 2>&1 || (echo [ERROR] git not found in PATH. & exit /b 1)

set "SDK=%TEMP%\vst3sdk"
if not exist "%SDK%\.git" (
  echo [1/3] Downloading pinned Steinberg VST3 SDK to %SDK% ...
  git clone --no-checkout https://github.com/steinbergmedia/vst3sdk.git "%SDK%" || exit /b 1
)
pushd "%SDK%"
git checkout 3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96 || exit /b 1
git submodule update --init --recursive --depth 1 || exit /b 1
popd

echo [2/3] Configuring...
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
  -DVST3_SDK_ROOT="%SDK%" ^
  -DSMTG_CREATE_PLUGIN_LINK=OFF ^
  -DSMTG_RUN_VST_VALIDATOR=OFF ^
  -DSMTG_USE_STATIC_CRT=ON || exit /b 1

echo [3/3] Building Release...
cmake --build build --config Release --parallel || exit /b 1

echo.
echo ============ DONE - no installation needed ============
echo Plugin bundle: "%cd%\build\VST3\Release\GrainsDosage.vst3"
echo Copy that whole folder to:  C:\Program Files\Common Files\VST3\
echo then rescan plugins in your DAW. To remove later, delete it.
echo ========================================================
endlocal
