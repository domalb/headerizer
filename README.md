# Headerizer (HDRZ)
Convert C/C++ file sets to a single header (.h file)

## HDRZ syntax
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
Notifies begin/end of included files

`hdrz.exe C:\book1.xlsx C:\book2.xlsx -c`

### Include directory (-i)

Add a directory where included files should be searched.

Quotes are not mandatory.

### Source file (-f)

Quotes are not mandatory.

Multiple files can be added using this argument.

### Working directory (-w)

Quotes are not mandatory.

### Output file (-o)

Optional in the case of a single source file. In this situation the output file name is build by appending the source file name with th postfix ".hdrz.h"

Quotes are not mandatory.

### Windows End Of Line (-weol)

Optional.

### Unix End Of Line (-ueol)

Optional.
