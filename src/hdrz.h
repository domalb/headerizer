#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#define HDRZ_WIN_EOL L"\r\n"
#define HDRZ_UNIX_EOL L"\n"

#define HDRZ_ERR_UNQUOTE_ARG_LENGTH -2
#define HDRZ_ERR_UNQUOTE_DETECT -3
#define HDRZ_ERR_DETECTINCLUDE_LINE -4
#define HDRZ_ERR_STRCPY_TRUNCATE -5
#define HDRZ_ERR_STRCPY_LENGTH -6
#define HDRZ_ERR_STRCPY_UNEXPECTED -7
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
	typedef const wchar_t* SZ;

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	template <size_t t_stLength>
	int StrCpy(wchar_t(&dst)[t_stLength], SZ src)
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
	int StrNCpy(wchar_t(&dst)[t_stLength], SZ src, size_t length)
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
	int GetUnquoted(SZ arg, wchar_t* buffer, bool verbose)
	{
		const wchar_t* val = arg;

		bool quotes = (val[0] == L'"');
		if(quotes)
		{
			++val;
		}

		wcscpy_s(buffer, MAX_PATH, val);

		if(quotes)
		{
			size_t argLength = wcslen(val);
			if(argLength < 2)
			{
				if(verbose)
				{
					std::wcout << L"invalid argument length" << std::endl;
				}
				return HDRZ_ERR_UNQUOTE_ARG_LENGTH;
			}
			else if(val[argLength - 1] != L'"')
			{
				if(verbose)
				{
					std::wcout << L"quote detection error for argument " << arg << std::endl;
				}
				return HDRZ_ERR_UNQUOTE_DETECT;
			}
			else
			{
				buffer[argLength - 1] = 0;
			}
		}

		return 0;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	bool IsSpace(wchar_t c)
	{
		return ((c == L' ') || (c == L'\t'));
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	bool IsEndOfLine(SZ line)
	{
		return ((*line == 0) ||
				(*line == L'\n') ||
				((*line == L'\r') && (*(line+1) == L'\n')));
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int DetectIncludeLine(SZ line, SZ& fileNameStart, size_t& fileNameLength, bool verbose)
	{
		fileNameStart = NULL;
		fileNameLength = 0;

		SZ fileStart = line;
		while(IsSpace(*fileStart))
		{
			++fileStart;
		}
		if(*fileStart != '#')
		{
			return false;
		}
		while(IsSpace(*fileStart))
		{
			++fileStart;
		}
		if(*fileStart != '\"')
		{
			++fileStart;
			SZ fileEnd = fileStart;
			while(*fileEnd != '\"')
			{
				if(IsEndOfLine(fileEnd))
				{
					return false;
				}
				++fileEnd;
			}

			fileNameStart = fileStart;
			fileNameLength = fileEnd - fileStart;
			return 0;
		}
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int WalkFile(std::wostream& out, std::wistream& in, bool verbose, SZ fileName)
	{
		int lineIndex = 0;

		static const size_t lineLengthMax = 2048;
		wchar_t line [lineLengthMax];

		while(in.eof() == false)
		{
			in.getline(line, lineLengthMax);

			SZ fileNameStart;
			size_t fileNameLength;
			int detect = DetectIncludeLine(line, fileNameStart, fileNameLength);
			if(detect < 0)
			{
				if(verbose)
				{
					std::wcout << L"error detecting include line while walking line " << lineIndex << " in file " << fileName << std::endl;
				}
				return detect;
			}
			if(fileNameStart != NULL)
			{
				wchar_t fileName [MAX_PATH];
				hdrzReturnIfError(StrNCpy(fileName, fileNameStart, fileNameLength), L"error copy include file name while walking line " << lineIndex << " in file " << fileName);
			}
			else
			{
				out << line;
			}
		}
		return 0;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int WalkFile(std::wostream& out, SZ fileName, bool verbose)
	{
		std::wifstream in;
		in.open(fileName);

		return WalkFile(out, in, verbose, fileName);
	}
}
