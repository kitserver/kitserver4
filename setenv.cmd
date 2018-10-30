@echo off
echo Setting kitserver compile environment
@call "c:\vs60ee\vc98\bin\vcvars32.bat"
set DXSDK=c:\dxsdk81
set STLPORT=c:\stlport-4.6.2
set INCLUDE=%DXSDK%\include;%STLPORT%\stlport;%INCLUDE%
set LIB=%DXSDK%\lib;%STLPORT%\lib;%LIB%
echo Environment set

