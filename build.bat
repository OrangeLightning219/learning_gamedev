@echo off

if not exist build mkdir build

set compiler_args=^
-fcolor-diagnostics ^
-MT ^
-nologo ^
-GR- ^
-EHa- ^
-Od -Oi ^
-WX -W4 ^
-DSLOW -DINTERNAL -DUNITY_BUILD ^
-FC ^
-Zi ^
-Wno-writable-strings ^
-Wno-unused ^
-Wno-unused-parameter ^
-Wno-unused-variable ^
../src/win32_isometric_game.cpp ^
user32.lib gdi32.lib winmm.lib

pushd build

clang-cl %compiler_args% && echo [32mBuild successfull[0m || echo [31mBuild failed[0m

popd

