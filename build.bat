@echo off
if not exist build mkdir build

set compiler_args=^
-MTd ^
-nologo ^
-GR- ^
-EHa- ^
-Od -Oi ^
-WX -W4 ^
-DSLOW -DINTERNAL -DUNITY_BUILD ^
-FC ^
-Zi ^
-diagnostics:caret ^
-wd4201 ^
-wd4100 ^
-wd4505

set linker_args=user32.lib gdi32.lib winmm.lib
set dll_pdb_file=learning_gamedev_%date:~-4,4%%date:~-10,2%%date:~-7,2%_%time:~0,2%%time:~3,2%%time:~6,2%.pdb

pushd build

del *.pdb > NUL 2> NUL

cl %compiler_args% -LD ../src/learning_gamedev.cpp /link -incremental:no -PDB:%dll_pdb_file% && echo [32mGame DLL build successfull[0m || echo [31mGame DLL build failed[0m

cl %compiler_args% ../src/win32_learning_gamedev.cpp /link -incremental:no %linker_args% && echo [32mPlatform build successfull[0m || echo [31mPlatform build failed[0m

popd
