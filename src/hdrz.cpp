#include "hdrz.h"

#include <sys/stat.h>
// #include <unistd.h>
#include <windows.h>
#include <assert.h>
// #include <shlwapi.h>

namespace hdrz
{
	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	context::context()
		: incDirs(NULL)
		, incDirsCount(0)
	{
	}

	//----------------------------------------------------------------------------------------------------------------------
	// https://msdn.microsoft.com/en-us/library/36k2cdd4.aspx
	// The difference between the two forms is the order in which the preprocessor searches for header files when the path is incompletely specified.
	// Quoted form : The preprocessor searches for include files in this order:
	// 1) In the same directory as the file that contains the #include statement.
	// 2) In the directories of the currently opened include files, in the reverse order in which they were opened.The search begins in the directory of the parent include file and continues upward through the directories of any grandparent include files.
	// 3) Along the path that's specified by each /I compiler option.
	// 4) Along the paths that are specified by the INCLUDE environment variable.
	// Angle-bracket form : The preprocessor searches for include files in this order:
	// 1) Along the path that's specified by each /I compiler option.
	// 2) When compiling occurs on the command line, along the paths that are specified by the INCLUDE environment variable.
	//----------------------------------------------------------------------------------------------------------------------
	int context::resolveInclusion(sz inclusionSpec, sz inclusionContainerFileDir, bool quoted, std::wstring& resolvedDir) const
	{
		assert(pathIsAbsolute(inclusionSpec) == false);
		assert(pathIsAbsolute(inclusionContainerFileDir));

		std::wstring absFileName;
		if(quoted)
		{
			// In the same directory as the file that contains the #include statement.
			absFileName = inclusionContainerFileDir;
			absFileName += inclusionSpec;
			if(fileExists(absFileName.c_str()))
			{
				resolvedDir = absFileName;
				return 0;
			}

			// In the directories of the currently opened include files, in the reverse order in which they were opened.The search begins in the directory of the parent include file and continues upward through the directories of any grandparent include files.
			// TODO

		}

		// Along the path that's specified by each /I compiler option.
		for(size_t i = 0; i < incDirsCount; ++i)
		{
			absFileName = incDirs[i];
			absFileName += inclusionSpec;
			if(fileExists(absFileName.c_str()))
			{
				resolvedDir = absFileName;
				return 0;
			}
		}

		// Along the paths that are specified by the INCLUDE environment variable.
		// TODO

		// Could not resolve the inclusion
		resolvedDir.clear();
		return 0;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	bool context::hasIncluded(sz absoluteFileName) const
	{
		for(size_t i = 0; i < included.size(); ++i)
		{
			if(included[i] == absoluteFileName)
			{
				return true;
			}
		}
		return false;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int detectIncludeLine(sz line, sz& fileNameStart, size_t& fileNameLength)
	{
		fileNameStart = NULL;
		fileNameLength = 0;

		sz fileStart = line;
		while(isSpace(*fileStart))
		{
			++fileStart;
		}
		if(*fileStart != '#')
		{
			return 0;
		}
		++fileStart;
		while(isSpace(*fileStart))
		{
			++fileStart;
		}
		if(wcsncmp(fileStart, L"include", HDRZ_STR_LEN(L"include")) != 0)
		{
			return 0;
		}
		fileStart += HDRZ_STR_LEN(L"include");
		while(isSpace(*fileStart))
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
			if(isEndOfLine(fileEnd))
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
	int handleIncludeLine(context& ctxt, std::wostream& out, sz line, sz inclusionSpec, bool quoted, bool verbose, sz fileDir, sz fileName)
	{
		assert(pathIsAbsolute(fileName) == false);
		assert(pathIsAbsolute(fileDir));

		// Find the absolute path to include
		std::wstring absIncFileName;
		if(pathIsAbsolute(inclusionSpec))
		{
			if(fileExists(inclusionSpec))
			{
				absIncFileName = inclusionSpec;
			}
		}
		else
		{
			std::wstring absIncDir;
			hdrzReturnIfError(ctxt.resolveInclusion(inclusionSpec, fileDir, quoted, absIncDir), L"Error resolving inclusion " << inclusionSpec << L" in " << fileName);
			if(absIncDir.empty() == false)
			{
				absIncFileName = absIncDir;
				absIncFileName += inclusionSpec;
			}
		}
		if(absIncFileName.empty())
		{
			// Could not find the absolute file to include, the include is left as-is
			out << line << std::endl;
		}
		else
		{
			// Checking the absolute file has not already been included
			if(ctxt.hasIncluded(absIncFileName) == false)
			{
				// Actually include the file
				ctxt.included.push_back(absIncFileName);
				hdrzReturnIfError(walkFile(ctxt, out, incFileName, verbose), L"error walking file " << incFileName << L" included in " << fileName);
			}
		}

		return 0;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int walkFileStream(context& ctxt, std::wostream& out, std::wistream& in, bool verbose, sz fileDir, sz fileName)
	{
		int lineIndex = 0;

		static const size_t lineLengthMax = 2048;
		wchar_t line [lineLengthMax];
		
		while((in.bad() || in.eof()) == false)
		{
			in.getline(line, lineLengthMax);

			sz fileNameStart;
			size_t fileNameLength;
			int detect = detectIncludeLine(line, fileNameStart, fileNameLength);
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
				hdrzReturnIfError(strNCpy(incFileName, fileNameStart, fileNameLength, verbose), L"error copying include file name while walking line " << lineIndex << L" in file " << fileName);
				hdrzReturnIfError(handleIncludeLine(ctxt, out, line, incFileName, verbose, fileDir, fileName), L"error handling include of " << incFileName << L" in " << fileName);
			}
			else
			{
				// basic line
				out << line << std::endl;
			}
		}
		return 0;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int walkFile(context& ctxt, std::wostream& out, sz fileName, bool verbose)
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

		int walk = walkFileStream(ctxt, out, in, verbose, fileName);
		in.close();
		return walk;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int process(const input& in, bool verbose)
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

		for(size_t i = 0; i < in.srcFilesCount; ++i)
		{
			sz srcFile = in.srcFiles[i];
			hdrzReturnIfError(walkFile(ctxt, out, srcFile, verbose), L"error walking source file #" << i << " " << srcFile);
		}
		out.close();
		return 0;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	bool pathIsAbsolute(sz fileName)
	{
		return (wcschr(fileName, L':') != NULL);
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	bool fileExists(sz fileName)
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
	int getUnquoted(sz arg, wchar_t* buffer, bool verbose)
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
	bool isSpace(wchar_t c)
	{
		return ((c == L' ') || (c == L'\t'));
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	bool isEndOfLine(sz line)
	{
		return ((*line == 0) ||
			(*line == L'\n') ||
			((*line == L'\r') && (*(line + 1) == L'\n')));
	}
}
