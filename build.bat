@echo off

IF NOT EXIST build mkdir build
pushd build

set CompilerFlags=-MTd -nologo -GR- -EHsc -Oi -W4 -FC -Z7 -DWINDOWS=1

cl %CompilerFlags% ../main.cpp /link Kernel32.lib
