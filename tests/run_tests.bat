@echo off
REM Helper script to build and run host-side unit tests within the devcontainer
REM Usage: run_tests.bat

setlocal enabledelayedexpansion

set IMAGE_NAME=n64flashcartmenu-sc64deployer
set PROJECT_DIR=%~dp0..

echo Building devcontainer image...
docker build --progress=plain -t %IMAGE_NAME% -f .devcontainer\flashcart\Dockerfile.sc64deployer . > nul 2>&1
if errorlevel 1 (
    echo [FAIL] Could not build devcontainer image.
    exit /b 1
)

echo Running unit tests...
docker run --rm -v "%PROJECT_DIR%:/workspaces/N64FlashcartMenu" ^
    -w /workspaces/N64FlashcartMenu/tests ^
    %IMAGE_NAME% ^
    bash -lc "make -B test"

if errorlevel 1 (
    echo [FAIL] Tests failed!
    exit /b 1
)

echo.
echo [PASS] All tests passed!
