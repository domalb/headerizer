// HDRZ : begin of C:\Users\secator\Documents\GitHub\headerizer\test\basic\main1.h
main1 1
// HDRZ : begin of C:\Users\secator\Documents\GitHub\headerizer\test\basic\a.h
a 1
a 2
a 3

// HDRZ : end of C:\Users\secator\Documents\GitHub\headerizer\test\basic\a.h
main1 2
// HDRZ : include left as-is since no file was found
#include "../test/b.h"
main1 3
// HDRZ : begin of C:\Users\secator\Documents\GitHub\headerizer\test\basic\d\d.h
d1
// HDRZ : begin of C:\Users\secator\Documents\GitHub\headerizer\test\basic\e.h
e1
e2
e3

// HDRZ : end of C:\Users\secator\Documents\GitHub\headerizer\test\basic\e.h
d2
   # include <sys_header.h>
d3

// HDRZ : end of C:\Users\secator\Documents\GitHub\headerizer\test\basic\d\d.h
main1 4
// HDRZ : include left as-is since no file was found
#include "missing_file.h"
main1 5
// HDRZ : begin of C:\Users\secator\Documents\GitHub\headerizer\test\basic\onceGuard1.h

onceGuard1 1
onceGuard1 2
onceGuard1 3

// HDRZ : end of C:\Users\secator\Documents\GitHub\headerizer\test\basic\onceGuard1.h
// HDRZ : include skipped as it should be included once only
// #include "onceGuard1.h"
main1 6
// HDRZ : begin of C:\Users\secator\Documents\GitHub\headerizer\test\basic\onceGuard2.h

onceGuard2 1
onceGuard2 2
onceGuard2 3


// HDRZ : end of C:\Users\secator\Documents\GitHub\headerizer\test\basic\onceGuard2.h
// HDRZ : include skipped as it should be included once only
// #include "onceGuard2.h"
main1 7
// HDRZ : begin of C:\Users\secator\Documents\GitHub\headerizer\test\basic\oncePragma1.h

oncePragma1 1
oncePragma1 2
oncePragma1 3

// HDRZ : end of C:\Users\secator\Documents\GitHub\headerizer\test\basic\oncePragma1.h
// HDRZ : include skipped as it should be included once only
// #include "oncePragma1.h"
main1 8
// HDRZ : begin of C:\Users\secator\Documents\GitHub\headerizer\test\basic\oncePragma2.h
oncePragma2 1
oncePragma2 2
oncePragma2 3

// HDRZ : end of C:\Users\secator\Documents\GitHub\headerizer\test\basic\oncePragma2.h
// HDRZ : include skipped as it should be included once only
// #include "oncePragma2.h"
main1 9

// HDRZ : end of C:\Users\secator\Documents\GitHub\headerizer\test\basic\main1.h
