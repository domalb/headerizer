%~d0
cd %~dp0

..\bin\vc14\x86\Debug\hdrz.exe -p -v -c -f="%~dp0bullet\LinearMath\*.cpp" -i="%~dp0bullet\LinearMath" -o="%~dp0bulletLinearMath.hdrz.h"

pause