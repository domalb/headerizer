%~d0
cd %~dp0

..\bin\vc14\x86\Debug\hdrz.exe -v -c -f="..\src\hdrzImpl.cpp" -f="..\src\hdrzMain.cpp" -o="%~dp0selfHdrz.h" -i="%~dp0src"

pause