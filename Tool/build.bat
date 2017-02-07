@echo off

pushd "S:\GameProject\Build"
cl -FC -Zi ..\Code\main.cpp user32.lib gdi32.lib
popd