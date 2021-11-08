@echo off
mkdir build
pushd build
cl -DSLOW=1 -DINTERNAL=1 -FC -Zi ../src/win32_isometric_game.cpp user32.lib gdi32.lib
popd
