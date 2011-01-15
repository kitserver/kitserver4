@echo off
echo Setting kitserver compile environment
@call "c:\vs60\bin\vcvars32.bat"
set DXSDK=c:\dxsdk81
set INCLUDE=%DXSDK%\include;%INCLUDE%
set LIB=%DXSDK%\lib;%LIB%
echo Environment set

