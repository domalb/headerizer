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
#define HDRZ_ERR_OPEN_OUT_FILE -8
#define HDRZ_ERR_OPEN_IN_FILE -9
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

	int WalkFile(std::wostream& out, std::wistream& in, bool verbose, sz fileName);
	int WalkFile(std::wostream& out, sz fileName, bool verbose);

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	template <size_t t_stLength>
	int StrCpy(wchar_t(&dst)[t_stLength], sz src)
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
	int StrNCpy(wchar_t(&dst)[t_stLength], sz src, size_t length, bool verbose)
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
	int GetUnquoted(sz arg, wchar_t* buffer, bool verbose)
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
	bool IsEndOfLine(sz line)
	{
		return ((*line == 0) ||
				(*line == L'\n') ||
				((*line == L'\r') && (*(line+1) == L'\n')));
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int DetectIncludeLine(sz line, sz& fileNameStart, size_t& fileNameLength)
	{
		fileNameStart = NULL;
		fileNameLength = 0;

		sz fileStart = line;
		while(IsSpace(*fileStart))
		{
			++fileStart;
		}
		if(*fileStart != '#')
		{
			return 0;
		}
		while(IsSpace(*fileStart))
		{
			++fileStart;
		}
		if(*fileStart != '\"')
		{
			++fileStart;
			sz fileEnd = fileStart;
			while(*fileEnd != '\"')
			{
				if(IsEndOfLine(fileEnd))
				{
					return 0;
				}
				++fileEnd;
			}

			fileNameStart = fileStart;
			fileNameLength = fileEnd - fileStart;
		}
		return 0;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int WalkFile(std::wostream& out, std::wistream& in, bool verbose, sz fileName)
	{
		int lineIndex = 0;

		static const size_t lineLengthMax = 2048;
		wchar_t line [lineLengthMax];

		while(in.eof() == false)
		{
			in.getline(line, lineLengthMax);

			sz fileNameStart;
			size_t fileNameLength;
			int detect = DetectIncludeLine(line, fileNameStart, fileNameLength);
			if(detect < 0)
			{
				if(verbose)
				{
					std::wcout << L"error detecting include line while walking line " << lineIndex << L" in file " << fileName << std::endl;
				}
				return detect;
			}
			else if(fileNameStart != NULL)
			{
				wchar_t incFileName [MAX_PATH];
				hdrzReturnIfError(StrNCpy(incFileName, fileNameStart, fileNameLength, verbose), L"error copy include file name while walking line " << lineIndex << L" in file " << fileName);
				hdrzReturnIfError(WalkFile(out, incFileName, verbose), L"error walking file " << incFileName << L" included in " << fileName);
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
	int WalkFile(std::wostream& out, sz fileName, bool verbose)
	{
		std::wifstream in;
		in.open(fileName);
		if(in.bad())
		{
			if(verbose)
			{
				std::wcout << L"error opening input file " << fileName << std::endl;
			}
			return HDRZ_ERR_OPEN_IN_FILE;
		}

		int walk = WalkFile(out, in, verbose, fileName);
		in.close();

		return walk;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int ProcessFiles(sz* srcFileNames, size_t srcFileNamesCount, bool verbose)
	{
		for(size_t i = 0; i < srcFileNamesCount; ++i)
		{
			sz srcFileName = srcFileNames[i];

			std::wstring outFileName = L"hdrz_";
			outFileName += srcFileName;

			std::wofstream out;
			out.open(outFileName, std::wofstream::out);
			if(out.bad())
			{
				if(verbose)
				{
					std::wcout << L"error opening output file " << outFileName << std::endl;
				}
				return HDRZ_ERR_OPEN_OUT_FILE;
			}

			hdrzReturnIfError(WalkFile(out, srcFileName, verbose), L"error walking source file #" << i << " " << srcFileName);
			out.close();
		}
		return 0;

	}
}
