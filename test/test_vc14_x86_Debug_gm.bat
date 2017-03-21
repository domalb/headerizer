%~d0
cd %~dp0

..\bin\vc14\x86\Debug\hdrz.exe -v -c -f="%~dp0gm\*.cpp" -i="%~dp0gm" -o="%~dp0gmHdrz.h"

pause