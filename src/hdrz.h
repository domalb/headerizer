#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sys/stat.h>
// #include <unistd.h>
#include <windows.h>
// #include <shlwapi.h>

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
#define HDRZ_ERR_NO_DST_FILE -12
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

	struct context
	{
		/// Find given local file 
		/// \return  Absolute file name if found, empty string either
		std::wstring locateFile(sz localFileName)
		{
			std::wstring absFileName;
			for(size_t i; i < incDirsCount; ++i)
			{
				absFileName = incDirs[i];
				absFileName += localFileName;
				if(FileExists(absFileName.c_str()))
				{
					return absFileName;
				}
			}
			return std::wstring();
		}

		bool hasIncluded(sz absoluteFileName)
		{
			for(size_t i; i < included.size(); ++i)
			{
				if(included[i] == absoluteFileName)
				{
					return true;
				}
			}
			return false;
		}

		sz* incDirs;
		size_t incDirsCount;
		std::vector<std::wstring> defined;
		std::vector<std::wstring> included;
	};

	int WalkFile(context& ctxt, std::wostream& out, std::wistream& in, bool verbose, sz fileName);
	int WalkFile(context& ctxt, std::wostream& out, sz fileName, bool verbose);

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	bool PathIsAbsolute(sz fileName)
	{
		return (wcschr(fileName, L':') != NULL);
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	bool FileExists(sz fileName)
	{
// 		return (PathFileExistsW(fileName) != FALSE);
		DWORD dwAttribs = GetFileAttributesW(fileName);
		return ((dwAttribs != INVALID_FILE_ATTRIBUTES) && ((dwAttribs & FILE_ATTRIBUTE_DIRECTORY) == 0));
// 		stat tmp;
// 		return (stat(fileName, &tmp) == 0);
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	template <size_t t_stLength>
	int StrCpy(wchar_t(&dst)[t_stLength], sz src, bool verbose)
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
	template <size_t t_stLength>
	int StrCat(wchar_t(&dst)[t_stLength], sz src, bool verbose)
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
		++fileStart;
		while(IsSpace(*fileStart))
		{
			++fileStart;
		}
		if(wcsncmp(fileStart, L"include", HDRZ_STR_LEN(L"include")) != 0)
		{
			return 0;
		}
		fileStart += HDRZ_STR_LEN(L"include");
		while(IsSpace(*fileStart))
		{
			++fileStart;
		}
		const wchar_t quoteOpen = *fileStart;
		if((quoteOpen != '\"') && (quoteOpen != '<'))
		{
			return 0;
		}
		const wchar_t quoteClose = ((quoteOpen == '\"') ? '\"' : '>');
		++fileStart;
		sz fileEnd = fileStart;
		while(*fileEnd != quoteOpen)
		{
			if(IsEndOfLine(fileEnd))
			{
				return 0;
			}
			++fileEnd;
		}

		fileNameStart = fileStart;
		fileNameLength = fileEnd - fileStart;
		return 0;
	}
/*
	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int DetectDefineLine(sz line, sz& defineSymbolStart, size_t& defineSymbolLength, sz& defineValue, size_t& defineValue)
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
		++fileStart;
		while(IsSpace(*fileStart))
		{
			++fileStart;
		}
		if(wcsncmp(fileStart, L"include", HDRZ_STR_LEN(L"include")) != 0)
		{
			return 0;
		}
		fileStart += HDRZ_STR_LEN(L"include");
		while(IsSpace(*fileStart))
		{
			++fileStart;
		}
		const wchar_t quoteOpen = *fileStart;
		if((quoteOpen != '\"') && (quoteOpen != '<'))
		{
			return 0;
		}
		const wchar_t quoteClose = ((quoteOpen == '\"') ? '\"' : '>');
		++fileStart;
		sz fileEnd = fileStart;
		while(*fileEnd != quoteOpen)
		{
			if(IsEndOfLine(fileEnd))
			{
				return 0;
			}
			++fileEnd;
		}

		fileNameStart = fileStart;
		fileNameLength = fileEnd - fileStart;
		return 0;
	}
*/
	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int HandleIncludeLine(context& ctxt, std::wostream& out, sz line, sz incFileName, bool verbose, sz fileName)
	{
		// Find the absolute path to include
		std::wstring absIncFileName;
		if(PathIsAbsolute(incFileName))
		{
			if(FileExists(incFileName))
			{
				absIncFileName = incFileName;
			}
		}
		else
		{
			absIncFileName = ctxt.locateFile(incFileName);
		}
		if(absIncFileName.empty())
		{
			// Could not find the absolute file to include, the include is left as-is
			out << line;
		}
		else
		{
			// Checking the absolute file has not already been included
			if(ctxt.hasIncluded(incFileName) == false)
			{
				// Actually include the file
				ctxt.included.push_back(absIncFileName);
				hdrzReturnIfError(WalkFile(ctxt, out, incFileName, verbose), L"error walking file " << incFileName << L" included in " << fileName);
			}
		}

		return 0;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int WalkFile(context& ctxt, std::wostream& out, std::wistream& in, bool verbose, sz fileName)
	{
		int lineIndex = 0;

		static const size_t lineLengthMax = 2048;
		wchar_t line [lineLengthMax];
		
		while((in.bad() || in.eof()) == false)
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
				// Include line found
				wchar_t incFileName [MAX_PATH];
				hdrzReturnIfError(StrNCpy(incFileName, fileNameStart, fileNameLength, verbose), L"error copying include file name while walking line " << lineIndex << L" in file " << fileName);
				hdrzReturnIfError(HandleIncludeLine(ctxt, out, line, incFileName, verbose, fileName), L"error handling include of " << incFileName << L" in " << fileName);
			}
			else
			{
				// basic line
				out << line;
			}
		}
		return 0;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int WalkFile(context& ctxt, std::wostream& out, sz fileName, bool verbose)
	{
		std::wifstream in;
		in.open(fileName);
		if((in.is_open() == false) || in.bad())
		{
			if(verbose)
			{
				std::wcout << L"error opening input file " << fileName << std::endl;
			}
			return HDRZ_ERR_OPEN_IN_FILE;
		}

		int walk = WalkFile(ctxt, out, in, verbose, fileName);
		in.close();
		return walk;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int Process(const input& in, bool verbose)
	{
		context ctxt;
		for(size_t i = 0; i < in.definesCount; ++i)
		{
			ctxt.defined.push_back(in.defines[i]);
		}

		std::wofstream out;
		out.open(in.outFile, std::wofstream::out);
		if((out.is_open() == false) || out.bad())
		{
			if(verbose)
			{
				std::wcout << L"error opening output file " << in.outFile << std::endl;
			}
			return HDRZ_ERR_OPEN_OUT_FILE;
		}

		hdrzReturnIfError(WalkFile(out, srcFileName, verbose), L"error walking source file #" << i << " " << srcFileName);
		out.close();
		return 0;
	}
}
