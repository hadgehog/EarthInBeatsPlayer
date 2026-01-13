@echo OFF
SETLOCAL

echo Building assimp for UWP x64...

set SOURCE_DIR="assimp"
set BUILD_DIR="build\uwp_x64"

echo %SOURCE_DIR%
echo %BUILD_DIR%

cd %SOURCE_DIR%

if not exist %BUILD_DIR% (
    echo Creating directory %BUILD_DIR%...
    mkdir "%BUILD_DIR%"
)

echo Starting CMake configuration for UWP x64...

cmake -S . -B %BUILD_DIR% ^
-G "Visual Studio 17 2022" -A x64 ^
-DCMAKE_SYSTEM_NAME=WindowsStore ^
-DCMAKE_SYSTEM_VERSION=10.0.26100 ^
-DASSIMP_BUILD_ASSIMP_TOOLS=OFF ^
-DASSIMP_BUILD_TESTS=OFF ^
-DBUILD_SHARED_LIBS=OFF

if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    pause
    exit /b %ERRORLEVEL%
)

echo Configuration complete. Starting build...

cmake --build %BUILD_DIR% --config Debug

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b %ERRORLEVEL%
)

echo ASSIMP build successful! Libraries are located in %BUILD_DIR%
pause
ENDLOCAL