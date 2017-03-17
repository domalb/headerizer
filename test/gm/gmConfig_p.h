/*
    _____               __  ___          __            ____        _      __
   / ___/__ ___ _  ___ /  |/  /__  ___  / /_____ __ __/ __/_______(_)__  / /_
  / (_ / _ `/  ' \/ -_) /|_/ / _ \/ _ \/  '_/ -_) // /\ \/ __/ __/ / _ \/ __/
  \___/\_,_/_/_/_/\__/_/  /_/\___/_//_/_/\_\\__/\_, /___/\__/_/ /_/ .__/\__/
                                               /___/             /_/
                                             
  See Copyright Notice in gmMachine.h

*/

#ifndef _GMCONFIG_P_H_
#define _GMCONFIG_P_H_


#if __GNUC__ > 1                /* GNU C and GNU C++ define this.  */
#define HAS_BUILTIN_MEMCPY
#endif // __GNUC__

// pragmas
#if ((defined WIN32) || (defined _XBOX))
#	include <malloc.h> // alloca
#	define GM_CDECL              __cdecl
#	define _gmvsnprintf          _vsnprintf // vsnprintf
#	define _gmstricmp            stricmp
#	define _gmsnprintf           _snprintf
#	define GM_COMPILER_MSVC6
#	define GM_X86
#	pragma inline_recursion( on )
#	pragma auto_inline( on )
#	pragma inline_depth( 255 )
#	pragma warning(disable : 4514) // removing unused inline function
#	pragma warning(disable : 4100) // unreferenced formal parameter
#	pragma warning(disable : 4706) // assignment within conditional expression
#	pragma warning(disable : 4102) // unreferenced label
#	pragma warning(disable : 4710) // not inlined

// These two are for MSVS 2005 security consciousness until safe std lib funcs are available
#pragma warning(disable : 4996) // Deprecated functions
//#define _CRT_SECURE_NO_DEPRECATE // Allow old unsecure standard library functions, Disable some 'warning C4996 - function was deprecated'
//#define _USE_32BIT_TIME_T // So system binds can use int for timestamps

#elif (defined __ORBIS__)

#	undef HAS_BUILTIN_MEMCPY
#	include <string.h>
#	include <ctype.h>
#	define GM_CDECL
#	define _gmvsnprintf          vsnprintf
#	define _gmstricmp            strcasecmp
#	define _gmsnprintf           snprintf
	inline void strupr(char* buff) { while(*buff) { *buff = (char)toupper(*buff); ++buff; } }
	inline void strlwr(char* buff) { while(*buff) { *buff = (char)tolower(*buff); ++buff; } }

#elif (defined _DURANGO)

#	include <malloc.h> // alloca
#	define GM_CDECL              __cdecl
#	define _gmvsnprintf          _vsnprintf // vsnprintf
#	define _gmstricmp            stricmp
#	define _gmsnprintf           _snprintf
#	define GM_X86

#else // PLATFORM

#	include <alloca.h>
#	include <string.h>
#	include <ctype.h>
#	define GM_CDECL
#	define _gmvsnprintf          vsnprintf
#	define _gmstricmp            strcasecmp
#	define _gmsnprintf           snprintf
	inline void strupr(char* buff) { while(*buff) { *buff = (char)toupper(*buff); ++buff; } }
	inline void strlwr(char* buff) { while(*buff) { *buff = (char)tolower(*buff); ++buff; } }

#endif // PLATFORM

#include <new>
#include <cassert>
// system defines

#define GM_LITTLE_ENDIAN      1


// GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define GM_ENVIRONMENT64
#else
#define GM_ENVIRONMENT32
#endif

#else // MSVC

#if defined _DURANGO
#define GM_ENVIRONMENT64
#elif _WIN64
#define GM_ENVIRONMENT64
#elif defined _WIN32
#define GM_ENVIRONMENT32
#endif

#endif // Compiler


#ifdef _DEBUG
  #define GM_ASSERT(A)        assert(A)
#else //_DEBUG
  #define GM_ASSERT(A)
#endif //_DEBUG
#define GM_NL                 "\r\n" // "\n"
#define GM_INLINE             inline
#ifdef _DEBUG
  #define GM_DEBUG_BUILD
#endif // _DEBUG
#define GM_PRINTF             printf

//#define GM_CHECK_USER_BREAK_CALLBACK // Enable this only if a user break callback is set

#define GM_NEW( alloc_params ) new alloc_params
#define GM_PLACEMENT_NEW( alloc_params, address ) new(address) alloc_params

#define GM_DEFAULT_ALLOC_ALIGNMENT 4

#define GM_MAKE_ID32( a, b, c, d )  ( ((d)<<24) | ((c)<<16) | ((b)<<8) | (a))

#define GM_MIN_FLOAT32        -3.402823466e38f
#define GM_MAX_FLOAT32        3.402823466e38f

#define GM_MIN_FLOAT64        -1.7976931348623158e308
#define GM_MAX_FLOAT64        1.7976931348623158e308

#define GM_SMALLEST_FLOAT32   1.175494351e-38f
#define GM_SMALLEST_FLOAT64   2.2250738585072014e-308

#define GM_MIN_UINT8          0
#define GM_MAX_UINT8          255

#define GM_MIN_INT8           -128
#define GM_MAX_INT8           127

#define GM_MIN_UINT16         0
#define GM_MAX_UINT16         65535
#define GM_MIN_INT16          -32768
#define GM_MAX_INT16          32767

#define GM_MIN_UINT32         0
#define GM_MAX_UINT32         4294967295
#define GM_MIN_INT32          -2147483648
#define GM_MAX_INT32          2147483647

#define GM_MAX_INT            2147483647
#define GM_MAX_SHORT          32767

#define GM_MAX_CHAR_STRING    256
#define GM_MAX_PATH           256

// basic types
typedef const char * LPCTSTR;
typedef unsigned int gmuint;
typedef char gmint8;
typedef unsigned char gmuint8;
typedef short gmint16;
typedef unsigned short gmuint16;
typedef int gmint32;
typedef unsigned int gmuint32;
typedef float gmfloat;

#if defined(GM_ENVIRONMENT32)
typedef int gmptr; // machine pointer size as int
typedef unsigned int gmuptr; // machine pointer size as int
#elif defined(GM_ENVIRONMENT64)
typedef signed  long long gmptr; // machine pointer size as int
typedef unsigned long long gmuptr; // machine pointer size as int
#endif // ENVIRONMENT

#define GM_CRT_DEBUG
//#undef GM_CRT_DEBUG


#ifdef _MSC_VER
	#define GM_FORCEINLINE        __forceinline // inline
	#ifdef GM_CRT_DEBUG
	
	  #include <crtdbg.h>
	  #ifdef _DEBUG
		#define DEBUG_CLIENTBLOCK new (_CLIENT_BLOCK, __FILE__, __LINE__)

		#define SET_CRT_DEBUG_FIELD(a) _CrtSetDbgFlag( (a) | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) )
		#define CLEAR_CRT_DEBUG_FIELD(a) _CrtSetDbgFlag( ~(a) & _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) )
		#define _gmDumpLeaks() SET_CRT_DEBUG_FIELD( _CRTDBG_LEAK_CHECK_DF ) //Flag to dump memory leaks on exit
	  #else
		#define DEBUG_CLIENTBLOCK new

		#define _gmDumpLeaks() //Do nothing
	  #endif
	#endif //GM_CRT_DEBUG
#else
	#define GM_FORCEINLINE inline __attribute__((always_inline))
#endif //_MSC_VER


#endif // _GMCONFIG_P_H_
