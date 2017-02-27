#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#define HDRZ_STR_LEN(str) ((sizeof(str) / sizeof(str[0])) - 1)

#define HDRZ_WIN_EOL L"\r\n"
#define HDRZ_UNIX_EOL L"\n"

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
// errno_t copyErr = wcsncpy_s(file, MAX_PATH, fileStart, fileLength);
// if(copyErr != 0)
// {
// 	if(verbose)
// 	{
// 		switch(copyErr)
// 		{
// 		case STRUNCATE: std::wcout << L"error copying detected include file : STRUNCATE" << std::endl; break;
// 		case ERANGE: std::wcout << L"error copying detected include file : ERANGE" << std::endl; break;
// 		default: std::wcout << L"error copying detected include file : " << copyErr << std::endl; break;
// 		}
// 	}
// 	return HDRZ_ERR_DETECTINCLUDE_LINE;
// }

#define hdrzReturnIfError(expression, msg) _hdrzReturnIfError(expression, msg, __err##__LINE__)
#define _hdrzReturnIfError(expression, msg, errName) \
	{ \
		int errName = (expression); \
		if(errName < 0) \
		{ \
			if(verbose) \
			{ \
				std::wcout << msg << std::endl; \
			} \
			return errName; \
		} \
	}

namespace hdrz
{
	typedef const wchar_t* sz;

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	struct input
	{
		sz* defines;
		size_t definesCount;
		sz* incDirs;
		size_t incDirsCount;
		sz* srcFiles;
		size_t srcFilesCount;
		sz outFile;
	};

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	struct context
	{
		context();
		/// Find given local file 
		/// \return  Absolute file name if found, empty string either
		std::wstring locateFile(sz localFileName) const;
		bool hasIncluded(sz absoluteFileName) const;

		sz* incDirs;
		size_t incDirsCount;
		std::vector<std::wstring> defined;
		std::vector<std::wstring> included;
	};

	int WalkFile(context& ctxt, std::wostream& out, std::wistream& in, bool verbose, sz fileName);
	int WalkFile(context& ctxt, std::wostream& out, sz fileName, bool verbose);
	int DetectIncludeLine(sz line, sz& fileNameStart, size_t& fileNameLength);
	int HandleIncludeLine(context& ctxt, std::wostream& out, sz line, sz incFileName, bool verbose, sz fileName);
	int WalkFile(context& ctxt, std::wostream& out, std::wistream& in, bool verbose, sz fileName);
	int WalkFile(context& ctxt, std::wostream& out, sz fileName, bool verbose);
	int Process(const input& in, bool verbose);

	// Utils
	bool PathIsAbsolute(sz fileName);
	bool FileExists(sz fileName);
	int GetUnquoted(sz arg, wchar_t* buffer, bool verbose);
	bool IsSpace(wchar_t c);
	bool IsEndOfLine(sz line);

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	template <size_t t_stLength>
	inline int StrCpy(wchar_t(&dst)[t_stLength], sz src, bool verbose)
	{
		errno_t copyErr = wcscpy_s(dst, src);
		if(copyErr == 0)
		{
			return 0;
		}
		else
		{
			switch(copyErr)
			{
			case STRUNCATE:
			{
				if(verbose)
				{
					std::wcout << L"error copying detected include file : STRUNCATE" << std::endl;
				}
				return HDRZ_ERR_STRCPY_TRUNCATE;
			}
			break;
			case ERANGE:
			{
				if(verbose)
				{
					std::wcout << L"error copying detected include file : ERANGE" << std::endl;
				}
				return HDRZ_ERR_STRCPY_LENGTH;
			}
			break;
			default:
			{
				if(verbose)
				{
					std::wcout << L"error copying detected include file : " << copyErr << std::endl;
				}
				return HDRZ_ERR_STRCPY_UNEXPECTED;
			}
			break;
			}
		}
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	template <size_t t_stLength>
	inline int StrNCpy(wchar_t(&dst)[t_stLength], sz src, size_t length, bool verbose)
	{
		errno_t copyErr = wcsncpy_s(dst, src, length);
		if(copyErr == 0)
		{
			return 0;
		}
		else
		{
			switch(copyErr)
			{
			case STRUNCATE:
			{
				if(verbose)
				{
					std::wcout << L"error copying detected include file : STRUNCATE" << std::endl;
				}
				return HDRZ_ERR_STRCPY_TRUNCATE;
			}
			break;
			case ERANGE:
			{
				if(verbose)
				{
					std::wcout << L"error copying detected include file : ERANGE" << std::endl;
				}
				return HDRZ_ERR_STRCPY_LENGTH;
			}
			break;
			default:
			{
				if(verbose)
				{
					std::wcout << L"error copying detected include file : " << copyErr << std::endl;
				}
				return HDRZ_ERR_STRCPY_UNEXPECTED;
			}
			break;
			}
		}
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	template <size_t t_stLength>
	inline int StrCat(wchar_t(&dst)[t_stLength], sz src, bool verbose)
	{
		errno_t copyErr = wcscat_s(dst, src);
		if(copyErr == 0)
		{
			return 0;
		}
		else
		{
			switch(copyErr)
			{
			case STRUNCATE:
			{
				if(verbose)
				{
					std::wcout << L"error appending detected include file : STRUNCATE" << std::endl;
				}
				return HDRZ_ERR_STRCPY_TRUNCATE;
			}
			break;
			case ERANGE:
			{
				if(verbose)
				{
					std::wcout << L"error appending detected include file : ERANGE" << std::endl;
				}
				return HDRZ_ERR_STRCPY_LENGTH;
			}
			break;
			default:
			{
				if(verbose)
				{
					std::wcout << L"error appending detected include file : " << copyErr << std::endl;
				}
				return HDRZ_ERR_STRCPY_UNEXPECTED;
			}
			break;
			}
		}
	}
}
