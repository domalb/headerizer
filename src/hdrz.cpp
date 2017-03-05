#include "hdrz.h"

#include <sys/stat.h>
// #include <unistd.h>
#include <windows.h>
#include <assert.h>
#include <shlwapi.h>

//static const wchar_t fileSeparator = L'/';
static const wchar_t fileSeparator = L'\\';

namespace hdrz
{
	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	WalkItem::WalkItem(const std::wstring& fileDir, const std::wstring& fileName)
		: m_fileDir(fileDir)
		, m_fileName(fileName)
	{
		m_filePath = fileDir;
		m_filePath += fileSeparator;
		m_filePath += fileName;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	void WalkStack::push(const std::wstring& fileDir, const std::wstring& fileName)
	{
		assert(filePathIsAbsolute(fileDir.c_str()));
		assert(filePathIsAbsolute(fileName.c_str()) == false);
		m_items.push_back(WalkItem(fileDir, fileName));
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	Context::Context()
		: m_incDirs(NULL)
		, m_incDirsCount(0)
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
	int Context::resolveInclusion(sz inclusionSpec, bool quoted, std::wstring& resolvedDir) const
	{
		assert(filePathIsAbsolute(inclusionSpec) == false);

		std::wstring absFileName;
		if(quoted)
		{
			// In the same directory as the file that contains the #include statement.
			// In the directories of the currently opened include files, in the reverse order in which they were opened.The search begins in the directory of the parent include file and continues upward through the directories of any grandparent include files.
			for(size_t i = m_walkStack.size(); i-- > 0;)
			{
				const WalkItem& rItem = m_walkStack[i];
				absFileName = rItem.m_fileDir;
				absFileName += fileSeparator;
				absFileName += rItem.m_fileName;
				if(fileExists(absFileName.c_str()))
				{
					resolvedDir = rItem.m_fileDir;
					return 0;
				}
			}
		}

		// Along the path that's specified by each /I compiler option.
		for(size_t i = 0; i < m_incDirsCount; ++i)
		{
			absFileName = m_incDirs[i];
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
	bool Context::hasIncluded(sz absoluteFileName) const
	{
		for(size_t i = 0; i < m_included.size(); ++i)
		{
			if(m_included[i] == absoluteFileName)
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
	int handleIncludeLine(Context& ctxt, std::wostream& out, sz line, sz inclusionSpec, bool quoted, bool verbose)
	{
		assert(ctxt.m_walkStack.empty() == false);

		// Find the absolute path to include
		std::wstring absIncFilePath;
		if(filePathIsAbsolute(inclusionSpec))
		{
			if(fileExists(inclusionSpec))
			{
				absIncFilePath = inclusionSpec;
			}
		}
		else
		{
			std::wstring absIncFileDir;
			hdrzReturnIfError(ctxt.resolveInclusion(inclusionSpec, quoted, absIncFileDir), L"Error resolving inclusion " << ctxt.m_walkStack.getTop().m_filePath);
			if(absIncFileDir.empty() == false)
			{
				absIncFilePath = absIncFileDir;
				absIncFilePath = fileSeparator;
				absIncFilePath += inclusionSpec;
			}
		}
		if(absIncFilePath.empty())
		{
			// Could not find the absolute file to include, the include is left as-is
			out << line << std::endl;
		}
		else
		{
			hdrzReturnIfError(walkFile(ctxt, out, absIncFilePath.c_str(), verbose),
				L"error walking file " << absIncFilePath << L" included in " << ctxt.m_walkStack.getTop().m_filePath);
		}

		return 0;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int walkFileStream(Context& ctxt, std::wostream& out, std::wistream& in, bool verbose)
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
					std::wcout << L"error detecting include line while walking line " << lineIndex << L" in file " << ctxt.m_walkStack.getTop().m_filePath << std::endl;
				}
				return detect;
			}
			else if(fileNameStart != NULL)
			{
				// Include line found
				wchar_t inclusionSpec [MAX_PATH];
				hdrzReturnIfError(strNCpy(inclusionSpec, fileNameStart, fileNameLength, verbose), L"error copying include file name while walking line " << lineIndex << L" in file " << ctxt.m_walkStack.getTop().m_filePath);
				bool quoted = (*(fileNameStart + fileNameLength) == L'\"');
				hdrzReturnIfError(handleIncludeLine(ctxt, out, line, inclusionSpec, quoted, verbose), L"error handling include of " << inclusionSpec << L" in " << ctxt.m_walkStack.getTop().m_filePath);
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
	int walkFile(Context& ctxt, std::wostream& out, sz filePath, bool verbose)
	{
		assert(filePath != NULL);
		assert(filePath[0] != 0);
		assert(filePathIsAbsolute(filePath));

		std::wstring canonFilePath(filePath);
		canonicalizeFilePath(canonFilePath);

		std::wstring fileDir, fileName;
		splitFilePathToDirAndName(canonFilePath.c_str(), fileDir, fileName);
		if(fileDir.empty() || fileName.empty())
		{
			if(verbose)
			{
				std::wcout << L"Error splitting following path to directory & name : " << fileName << std::endl;
			}
			return HDRZ_ERR_INVALID_FILE_PATH;
		}

		int ret = 0;
		ctxt.m_walkStack.push(fileDir, fileName);

		std::wifstream in;
		in.open(filePath);
		if((in.is_open() == false) || in.bad())
		{
			if(verbose)
			{
				std::wcout << L"error opening read file " << canonFilePath << std::endl;
			}
			ret = HDRZ_ERR_OPEN_IN_FILE;
		}
		else
		{
			ret = walkFileStream(ctxt, out, in, verbose);
			in.close();
		}

		ctxt.m_walkStack.pop();
		return ret;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int process(const Input& in, bool verbose)
	{
		Context ctxt;
		for(size_t i = 0; i < in.m_definesCount; ++i)
		{
			ctxt.m_defined.push_back(in.m_defines[i]);
		}

		std::wofstream out;
		out.open(in.m_outFile, std::wofstream::out);
		if((out.is_open() == false) || out.bad())
		{
			if(verbose)
			{
				std::wcout << L"error opening output file " << in.m_outFile << std::endl;
			}
			return HDRZ_ERR_OPEN_OUT_FILE;
		}

		for(size_t i = 0; i < in.m_srcFilesCount; ++i)
		{
			sz srcFile = in.m_srcFiles[i];
			hdrzReturnIfError(walkFile(ctxt, out, srcFile, verbose), L"error walking source file #" << i << " " << srcFile);
		}
		out.close();
		return 0;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	bool filePathIsAbsolute(sz fileName)
	{
		return (wcschr(fileName, L':') != NULL);
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	void splitFilePathToDirAndName(sz filePath, std::wstring& fileDir, std::wstring& fileName)
	{
		sz lastSeparator = wcsrchr(filePath, L'\\');
		if(lastSeparator == NULL)
		{
			fileDir.clear();
			fileName = filePath;
		}
		else
		{
			fileDir.append(filePath, lastSeparator - filePath);
			fileName = lastSeparator + 1;
		}
	}
	
	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	void canonicalizeFilePath(sz in, std::wstring& out)
	{
		wchar_t tmpBuff [MAX_PATH];
		if(PathCanonicalize(tmpBuff, in) == TRUE)
		{
			out = tmpBuff;
		}
		else
		{
			out.clear();
		}
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	void canonicalizeFilePath(std::wstring& inOut)
	{
		canonicalizeFilePath(inOut.c_str(), inOut);
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
