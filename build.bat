@echo off
if not exist build mkdir build

set compiler_args=^
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
-fcolor-diagnostics

set linker_args=user32.lib gdi32.lib winmm.lib
set dll_pdb_file=isometric_game_%date:~-4,4%%date:~-10,2%%date:~-7,2%_%time:~0,2%%time:~3,2%%time:~6,2%.pdb

pushd build

del *.pdb > NUL 2> NUL

clang-cl %compiler_args% -LD ../src/isometric_game.cpp /link -incremental:no -PDB:%dll_pdb_file% && echo [32mGame DLL build successfull[0m || echo [31mGame DLL build failed[0m

clang-cl %compiler_args% ../src/win32_isometric_game.cpp /link -incremental:no %linker_args% && echo [32mPlatform build successfull[0m || echo [31mPlatform build failed[0m

popd
