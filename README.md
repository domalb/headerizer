# Headerizer (HDRZ)

Convert C/C++ file sets to a single header (.h file)

## Purpose

C/C++ libraries are sometimes hard to reuse/integrate in other projects :
  - Binaries are not compiled with expected compiler options
  - Binaries are hard to link in target build
  - Sources require specific tools to be compiled (python, cmake, premake...)
  - Sources contain no project file for you IDE, rebuilding one would be tedious
  
To fix this, some libraries are available as a single header, which is extremely easy to integrate.
  
HDRZ aims at transforming a library split across multiple h/c/cpp files into a signle header, so that it could be reused/integrated very easily.

## Syntax
Basic command line :

`hdrz.exe -i="C:\my_lib\include" -f="C:\my_lib\src\impl.cpp`

### Pause (-p)

To enable easy debugger attachment, hdrz.exe can be paused at start using '-p' argument, e.g. :

`hdrz.exe -i="C:\my_lib\include" -f="C:\my_lib\src\impl.cpp -p`

### Verbose (-v)

Log more information using '-v' argument, e.g. :

`hdrz.exe -i="C:\my_lib\include" -f="C:\my_lib\src\impl.cpp -v`

Error are outputted to std::out.

### Comments (-c)

Insert comments related to HDRZ behavior in generated header.
Notifies for not found includes left as-is.
Notifies begin/end of included files.

`hdrz.exe -i="C:\my_lib\include" -i="C:\my_skd\include" -f="C:\my_lib\src\impl.cpp -c`

### Include directory (-i)

Add a directory where included files should be searched.

Quotes are not mandatory.

`hdrz.exe -i="C:\my_lib\include" -f="C:\my_lib\src\impl.cpp`

### Source file (-f)

Specify the source files to process.
Can contain a '*' wildcard character to select multiple files at once.
Quotes are not mandatory.

Multiple files can be added using this argument.

`hdrz.exe -i="C:\my_lib\include" -f="C:\my_lib\src\sub\sub.cpp -f="C:\my_lib\src\*.cpp`

### Exclude content (-x)

Do not integrate actual content in generated header.
Allows easier debugging with keeping comments only.

`hdrz.exe -i="C:\my_lib\include" -f="C:\my_lib\src\impl.cpp -x`

### Working directory (-og3)

Detect 'once guards' by requiring 3 specific lines (ifndef/define/endif).

In foo.h, once guards are typically
```cpp
#ifndef _FOO_H_
#define _FOO_H_
...
#endif // _FOO_H_
```
The 'endif' line does not always have a specific comment. By default, only the first two lines are required to trigger the 'once guards' condition. if the og3 option is set, the third line is required too.

`hdrz.exe -i="C:\my_lib\include" -f="C:\my_lib\src\impl.cpp -og3`

### Output file (-o)

Optional in the case of a single source file. In this situation the output file name is build by appending the source file name with th postfix ".hdrz.h"

Quotes are not mandatory.

`hdrz.exe -i="C:\my_lib\include" -f="C:\my_lib\src\impl.cpp -x`

### Windows End Of Line (-weol)

Optional.

`hdrz.exe -i="C:\my_lib\include" -f="C:\my_lib\src\impl.cpp -weol`

### Unix End Of Line (-ueol)

Optional.

`hdrz.exe -i="C:\my_lib\include" -f="C:\my_lib\src\impl.cpp -ueol`

