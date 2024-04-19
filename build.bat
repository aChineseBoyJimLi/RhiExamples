@echo off
set currentDir=%~dp0
set buildDir=%currentDir%
set cmakeOutputDir=%currentDir%\Build
cmake -S %buildDir% -B %cmakeOutputDir% -G"Visual Studio 17 2022" -DUSE_VULKAN=ON -DWITH_RUNTIME_TEST=ON
pause