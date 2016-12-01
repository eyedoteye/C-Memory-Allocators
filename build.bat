@echo off

IF NOT EXIST build mkdir build
pushd build

set CompilerFlags=-MTd -nologo -GR- -EHsc -Oi -WX -W4 -FC -Z7 -I ../includes/

cl %CompilerFlags% ../*.cpp