@echo off

If "%1" == "clean" goto clean

etags *.c *.h

:build
pushd ..\build
msbuild /nologo littlec.sln
popd

goto eof

:clean
pushd ..\build
msbuild /nologo littlec.sln -t:Clean
popd

goto eof

:eof
