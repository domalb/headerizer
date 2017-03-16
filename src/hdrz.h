#ifndef _hdrz_
#define _hdrz_

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#define HDRZ_STR_LEN(str) ((sizeof(str) / sizeof(str[0])) - 1)

#define HDRZ_WIN_EOL L"\r\n"
#define HDRZ_UNIX_EOL L"\n"

#define HDRZ_ERR_OK 0
#define HDRZ_ERR_UNQUOTE_ARG_LENGTH -2
#define HDRZ_ERR_UNQUOTE_DETECT -3
#define HDRZ_ERR_DETECTINCLUDE_LINE -4
#define HDRZ_ERR_STRCPY_TRUNCATE -5
#define HDRZ_ERR_STRCPY_LENGTH -6
#define HDRZ_ERR_STRCPY_UNEXPECTED -7
#define HDRZ_ERR_OPEN_OUT_FILE -8
#define HDRZ_ERR_OPEN_IN_FILE -9
#define HDRZ_ERR_MULTIPLE_WORK_DIRS -10
#define HDRZ_ERR_MULTIPLE_DST_FILES -11
#define HDRZ_ERR_NO_OUT_FILE -12
#define HDRZ_ERR_INVALID_FILE_PATH -13
#define HDRZ_ERR_IN_FILE_NOT_FOUND -14
#define HDRZ_ERR_CANT_GET_EXE_DIR -15

#define hdrzLogInfo(x_msg) if(hdrz::verbose) { std::wcout << L"HDRZ: " << x_msg << std::endl; }
#define hdrzLogError(x_msg) if(hdrz::verbose) { std::wcerr << L"HDRZ: " << x_msg << std::endl; }

#define hdrzReturnIfError(expression, msg) _hdrzReturnIfError(expression, msg, __err##__LINE__)
#define _hdrzReturnIfError(expression, msg, errName) \
	{ \
		int errName = (expression); \
		if(errName < 0) \
		{ \
			hdrzLogError(msg); \
			return errName; \
		} \
	}

namespace hdrz
{
	typedef const wchar_t* sz;

	extern bool verbose;

	struct Input
	{
		bool m_comments;
		sz* m_defines;
		size_t m_definesCount;
		sz* m_incDirs;
		size_t m_incDirsCount;
		sz* m_srcFiles;
		size_t m_srcFilesCount;
		sz m_outFile;
	};
	int process(const Input& in);
}

#endif // _hdrz_
