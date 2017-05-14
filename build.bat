@echo off

IF NOT EXIST build mkdir build
pushd build

set CompilerFlags=-MTd -nologo -GR- -EHsc -Oi -W4 -FC -Z7 -DWINDOWS=1

echo Compiling Test Variants
cl %CompilerFlags% ../main.cpp /link Kernel32.lib -OUT:test_.exe
cl -DVERBOSE %CompilerFlags% ../main.cpp /link Kernel32.lib -OUT:test_v.exe
cl -DDEBUG %CompilerFlags% ../main.cpp /link Kernel32.lib -OUT:test_d.exe
cl -DVERBOSE -DDEBUG %CompilerFlags% ../main.cpp /link Kernel32.lib -OUT:test_vd.exe

echo Compiling Test Picker
cl %CompilerFlags% ../test_with_options.cpp /link Kernel32.lib -OUT:test.exe
