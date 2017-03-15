#include "hdrz.h"

#include <sys/stat.h>
// #include <unistd.h>
#include <windows.h>
#include <assert.h>
#include <shlwapi.h>

#ifdef _WIN32
static const wchar_t fileSeparator = L'\\';
static const wchar_t fileWrongSeparator = L'/';
#else // _WIN
static const wchar_t fileSeparator = L'/';
static const wchar_t fileWrongSeparator = L'\\';
#endif // _WIN

namespace hdrz
{
	bool verbose = false;

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	PreviouslyIncludedFile::PreviouslyIncludedFile(const std::wstring& filePath, bool onceOnly)
		: m_filePath(filePath)
		, m_onceOnly(onceOnly)
	{
	}

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
		: m_comments(false)
		, m_incDirs(NULL)
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
	int Context::resolveInclusion(sz inclusionSpec, bool quoted, std::wstring& resolvedFileDir, std::wstring& resolvedFilePath) const
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
				absFileName += inclusionSpec;
				if(fileExists(absFileName.c_str()))
				{
					resolvedFileDir = rItem.m_fileDir;
					resolvedFilePath = absFileName;
					canonicalizeFilePath(resolvedFilePath);
					return HDRZ_ERR_OK;
				}
			}
		}

		// Along the path that's specified by each /I compiler option.
		for(size_t i = 0; i < m_incDirsCount; ++i)
		{
			sz incDir = m_incDirs[i];
			absFileName = incDir;
			absFileName += fileSeparator;
			absFileName += inclusionSpec;
			if(fileExists(absFileName.c_str()))
			{
				resolvedFileDir = incDir;
				resolvedFilePath = absFileName;
				canonicalizeFilePath(resolvedFilePath);
				return HDRZ_ERR_OK;
			}
		}

		// Along the paths that are specified by the INCLUDE environment variable.
		// TODO

		// Could not resolve the inclusion
		resolvedFileDir.clear();
		resolvedFilePath.clear();
		return HDRZ_ERR_OK;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	PreviouslyIncludedFile* Context::findPreviousInclude(sz absoluteFilePath)
	{
		for(size_t i = 0; i < m_prevIncluded.size(); ++i)
		{
			PreviouslyIncludedFile& prevInc = m_prevIncluded[i];
			if(prevInc.m_filePath == absoluteFilePath)
			{
				return &prevInc;
			}
		}
		return NULL;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int detectOncePragma(sz line, bool& detected)
	{
		detected = false;

		sz readPos = line;
		skipSpaces(readPos);
		if(*readPos == '#')
		{
			++readPos;
			skipSpaces(readPos);
			if(wcsncmp(readPos, L"pragma", HDRZ_STR_LEN(L"pragma")) == 0)
			{
				readPos += HDRZ_STR_LEN(L"pragma");
				skipSpaces(readPos);
				if(wcsncmp(readPos, L"once", HDRZ_STR_LEN(L"once")) == 0)
				{
					skipSpaces(readPos);
					if((readPos[0] == 0) || ((readPos[0] == '/') && (readPos[1] == '/')))
					{
						detected = true;
					}
				}
			}
		}
		return HDRZ_ERR_OK;
	}

	//----------------------------------------------------------------------------------------------------------------------
	// detect pattern like [ ]#[ ]ifndef[ ][_]<fileNameNoExt>?
	//----------------------------------------------------------------------------------------------------------------------
	int detectOnceGuard1(sz line, sz filenameNoExt, size_t filenameNoExtLength, bool& detected)
	{
		detected = false;

		sz readPos = line;
		skipSpaces(readPos);
		if(*readPos == '#')
		{
			++readPos;
			skipSpaces(readPos);
			if(wcsncmp(readPos, L"ifndef", HDRZ_STR_LEN(L"ifndef")) == 0)
			{
				readPos += HDRZ_STR_LEN(L"ifndef");
				skipSpaces(readPos);
				while(*readPos == '_')
				{
					++readPos;
				}
				if(_wcsnicmp(readPos, filenameNoExt, filenameNoExtLength) == 0)
				{
// 					skipSpaces(readPos);
// 					if((readPos[0] == 0) || ((readPos[0] == '/') && (readPos[1] == '/')))
					{
						detected = true;
					}
				}
			}
		}
		return HDRZ_ERR_OK;
	}

	//----------------------------------------------------------------------------------------------------------------------
	// detect pattern like [ ]#[ ]define[ ][_]<fileNameNoExt>?
	//----------------------------------------------------------------------------------------------------------------------
	int detectOnceGuard2(sz line, sz filenameNoExt, size_t filenameNoExtLength, bool& detected)
	{
		detected = false;

		sz readPos = line;
		skipSpaces(readPos);
		if(*readPos == '#')
		{
			++readPos;
			skipSpaces(readPos);
			if(wcsncmp(readPos, L"define", HDRZ_STR_LEN(L"define")) == 0)
			{
				readPos += HDRZ_STR_LEN(L"define");
				skipSpaces(readPos);
				while(*readPos == '_')
				{
					++readPos;
				}
				if(_wcsnicmp(readPos, filenameNoExt, filenameNoExtLength) == 0)
				{
// 					skipSpaces(readPos);
// 					if((readPos[0] == 0) || ((readPos[0] == '/') && (readPos[1] == '/')))
					{
						detected = true;
					}
				}
			}
		}
		return HDRZ_ERR_OK;
	}

	//----------------------------------------------------------------------------------------------------------------------
	// detect pattern like [ ]#[ ]endif[ ]//[ ][_]<fileNameNoExt>?
	//----------------------------------------------------------------------------------------------------------------------
	int detectOnceGuard3(sz line, sz filenameNoExt, size_t filenameNoExtLength, bool& detected)
	{
		detected = false;

		sz readPos = line;
		skipSpaces(readPos);
		if(*readPos == '#')
		{
			++readPos;
			skipSpaces(readPos);
			if(wcsncmp(readPos, L"endif", HDRZ_STR_LEN(L"endif")) == 0)
			{
				readPos += HDRZ_STR_LEN(L"endif");
				skipSpaces(readPos);
				if((readPos[0] == '/') && ((readPos[1] == '/') || (readPos[1] == '*')))
				{
					readPos += 2;
				}
				skipSpaces(readPos);
				while(*readPos == '_')
				{
					++readPos;
				}
				if(_wcsnicmp(readPos, filenameNoExt, filenameNoExtLength) == 0)
				{
//					skipSpaces(readPos);
					{
						detected = true;
					}
				}
			}
		}
		return HDRZ_ERR_OK;
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
			return HDRZ_ERR_OK;
		}
		++fileStart;
		while(isSpace(*fileStart))
		{
			++fileStart;
		}
		if(wcsncmp(fileStart, L"include", HDRZ_STR_LEN(L"include")) != 0)
		{
			return HDRZ_ERR_OK;
		}
		fileStart += HDRZ_STR_LEN(L"include");
		while(isSpace(*fileStart))
		{
			++fileStart;
		}
		const wchar_t quoteOpen = *fileStart;
		if((quoteOpen != '\"') && (quoteOpen != '<'))
		{
			return HDRZ_ERR_OK;
		}
		const wchar_t quoteClose = ((quoteOpen == '\"') ? '\"' : '>');
		++fileStart;
		sz fileEnd = fileStart;
		while(*fileEnd != quoteOpen)
		{
			if(isEndOfLine(fileEnd))
			{
				return HDRZ_ERR_OK;
			}
			++fileEnd;
		}

		fileNameStart = fileStart;
		fileNameLength = fileEnd - fileStart;
		return HDRZ_ERR_OK;
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
			return HDRZ_ERR_OK;
		}
		++fileStart;
		while(IsSpace(*fileStart))
		{
			++fileStart;
		}
		if(wcsncmp(fileStart, L"include", HDRZ_STR_LEN(L"include")) != 0)
		{
			return HDRZ_ERR_OK;
		}
		fileStart += HDRZ_STR_LEN(L"include");
		while(IsSpace(*fileStart))
		{
			++fileStart;
		}
		const wchar_t quoteOpen = *fileStart;
		if((quoteOpen != '\"') && (quoteOpen != '<'))
		{
			return HDRZ_ERR_OK;
		}
		const wchar_t quoteClose = ((quoteOpen == '\"') ? '\"' : '>');
		++fileStart;
		sz fileEnd = fileStart;
		while(*fileEnd != quoteOpen)
		{
			if(IsEndOfLine(fileEnd))
			{
				return HDRZ_ERR_OK;
			}
			++fileEnd;
		}

		fileNameStart = fileStart;
		fileNameLength = fileEnd - fileStart;
		return HDRZ_ERR_OK;
	}
*/
	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int handleIncludeLine(Context& ctxt, std::wostream& out, sz line, sz inclusionSpec, bool quoted)
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
			hdrzReturnIfError(ctxt.resolveInclusion(inclusionSpec, quoted, absIncFileDir, absIncFilePath), L"error resolving inclusion " << ctxt.m_walkStack.getTop().m_filePath);
		}
		if(absIncFilePath.empty())
		{
			if(ctxt.m_comments)
			{
				out << L"// HDRZ : include left as-is since no file was found" << std::endl;
			}
			out << line << std::endl;
		}
		else
		{
			PreviouslyIncludedFile* prevFile = ctxt.findPreviousInclude(absIncFilePath.c_str());
			bool prevIncluded = (prevFile != NULL);
			bool prevOnceOnly = ((prevFile != NULL) && prevFile->m_onceOnly);

			if(prevOnceOnly)
			{
				if(ctxt.m_comments)
				{
					out << L"// HDRZ : include skipped as it should be included once only" << std::endl;
				}
				out << L"// " << line << std::endl;
			}
			else
			{
				if(prevIncluded)
				{
					hdrzReturnIfError(walkFile(ctxt, out, absIncFilePath.c_str(), NULL), L"error walking previously included file " << absIncFilePath << L" included in " << ctxt.getCurrentFilePath());
				}
				else
				{
					bool detectOnce = false;
					hdrzReturnIfError(walkFile(ctxt, out, absIncFilePath.c_str(), &detectOnce), L"error walking file " << absIncFilePath << L" included in " << ctxt.getCurrentFilePath());
					ctxt.m_prevIncluded.push_back(PreviouslyIncludedFile(absIncFilePath, detectOnce));
				}
			}
		}

		return HDRZ_ERR_OK;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int walkFileStream(Context& ctxt, std::wostream& out, std::wistream& in, bool* detectOnce)
	{
		// Once detection init
		bool detectOnceNeeded = (detectOnce != NULL);
		bool detectedOnceGuard1 = false;
		bool detectedOnceGuard2 = false;
		bool detectedOnceGuard3 = false;
		wchar_t fileNameNoExt [MAX_PATH] = { 0 };
		size_t filenameNoExtLength = 0;
		if(detectOnceNeeded)
		{
			const std::wstring& fileName = ctxt.getCurrentFilePath();
			strCpy(fileNameNoExt, fileName.c_str());
			wchar_t* dot = wcschr(fileNameNoExt, L'.');
			if(dot != NULL)
			{
				*dot = 0;
				filenameNoExtLength = dot - sz(fileNameNoExt);
			}
			else
			{
				filenameNoExtLength = fileName.length();
			}
			detectOnce = false;
		}

		// Lines iteration
		static const size_t lineLengthMax = 2048;
		int lineIndex = 0;
		wchar_t line [lineLengthMax];
		
		while((in.bad() || in.eof()) == false)
		{
			in.getline(line, lineLengthMax);

			if(detectOnceNeeded)
			{
				// detect pragma once
				bool detectedOncePragma = false;
				hdrzReturnIfError(detectOncePragma(line, detectedOncePragma), L"error detecting once pragma while walking line " << lineIndex << L" in file " << ctxt.getCurrentFilePath());
				if(detectedOncePragma)
				{
					*detectOnce = true;
					detectOnceNeeded = false;
					continue;
				}

				// detect guards
				if(detectedOnceGuard2)
				{
					hdrzReturnIfError(detectOnceGuard3(line, fileNameNoExt, filenameNoExtLength, detectedOnceGuard3), L"error detecting once guard3 while walking line " << lineIndex << L" in file " << ctxt.getCurrentFilePath());
					if(detectedOnceGuard3)
					{
						*detectOnce = true;
						detectOnceNeeded = false;
						continue;
					}
				}
				else if(detectedOnceGuard1)
				{
					hdrzReturnIfError(detectOnceGuard2(line, fileNameNoExt, filenameNoExtLength, detectedOnceGuard2), L"error detecting once guard2 while walking line " << lineIndex << L" in file " << ctxt.getCurrentFilePath());
					if(detectedOnceGuard2)
					{
						continue;
					}
				}
				else
				{
					hdrzReturnIfError(detectOnceGuard1(line, fileNameNoExt, filenameNoExtLength, detectedOnceGuard1), L"error detecting once guard1 while walking line " << lineIndex << L" in file " << ctxt.getCurrentFilePath());
					if(detectedOnceGuard1)
					{
						continue;
					}
				}
			}

			// detect include
			sz fileNameStart;
			size_t fileNameLength;
			hdrzReturnIfError(detectIncludeLine(line, fileNameStart, fileNameLength), L"error detecting include line while walking line " << lineIndex << L" in file " << ctxt.getCurrentFilePath());
			bool detectedInclude = (fileNameStart != NULL);
			if(detectedInclude)
			{
				// Include line found
				wchar_t inclusionSpec [MAX_PATH];
				hdrzReturnIfError(strNCpy(inclusionSpec, fileNameStart, fileNameLength), L"error copying include file name while walking line " << lineIndex << L" in file " << ctxt.getCurrentFilePath());
				bool quoted = (*(fileNameStart + fileNameLength) == L'\"');
				hdrzReturnIfError(handleIncludeLine(ctxt, out, line, inclusionSpec, quoted), L"error handling include of " << inclusionSpec << L" in " << ctxt.getCurrentFilePath());
				continue;
			}

			// basic line, simply copied
			out << line << std::endl;
		}
		return HDRZ_ERR_OK;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int walkFile(Context& ctxt, std::wostream& out, sz filePath, bool* detectOnce)
	{
		assert(filePath != NULL);
		assert(filePath[0] != 0);
		assert(filePathIsAbsolute(filePath));

		// Could be removed ?
		std::wstring canonFilePath(filePath);
		canonicalizeFilePath(canonFilePath);

		std::wstring fileDir, fileName;
		splitFilePathToDirAndName(canonFilePath.c_str(), fileDir, fileName);
		if(fileDir.empty() || fileName.empty())
		{
			hdrzLogError(L"error splitting following path to directory & name : " << fileName);
			return HDRZ_ERR_INVALID_FILE_PATH;
		}

		int ret = 0;
		ctxt.m_walkStack.push(fileDir, fileName);
		if(ctxt.m_comments)
		{
			out << L"// HDRZ : begin of " << ctxt.getCurrentFilePath() << std::endl;
		}

		std::wifstream in;
		in.open(filePath);
		if((in.is_open() == false) || in.bad())
		{
			hdrzLogError(L"error opening read file " << canonFilePath);
			ret = HDRZ_ERR_OPEN_IN_FILE;
		}
		else
		{
			ret = walkFileStream(ctxt, out, in, detectOnce);
			in.close();
		}

		if(ctxt.m_comments)
		{
			out << L"// HDRZ : end of " << ctxt.getCurrentFilePath() << std::endl;
		}
		ctxt.m_walkStack.pop();

		return ret;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int process(const Input& in)
	{
		int err = HDRZ_ERR_OK;

		Context ctxt;
		ctxt.m_comments = in.m_comments;
		ctxt.m_incDirs = in.m_incDirs;
		ctxt.m_incDirsCount = in.m_incDirsCount;
		for(size_t i = 0; i < in.m_definesCount; ++i)
		{
			ctxt.m_defined.push_back(in.m_defines[i]);
		}

		std::wofstream out;
		out.open(in.m_outFile, std::wofstream::out);
		if((out.is_open() == false) || out.bad())
		{
			hdrzLogError(L"error opening output file " << in.m_outFile);
			return HDRZ_ERR_OPEN_OUT_FILE;
		}

		for(size_t i = 0; i < in.m_srcFilesCount; ++i)
		{
			sz srcFile = in.m_srcFiles[i];

			std::wstring resolvedSrcFileDir;
			std::wstring resolvedSrcFilePath;
			hdrzReturnIfError(ctxt.resolveInclusion(srcFile, true, resolvedSrcFileDir, resolvedSrcFilePath), L"error resolving source file " << srcFile);
			if(resolvedSrcFilePath.empty())
			{
				err = HDRZ_ERR_IN_FILE_NOT_FOUND;
				break;
			}
			canonicalizeFilePath(resolvedSrcFilePath);
			hdrzReturnIfError(walkFile(ctxt, out, resolvedSrcFilePath.c_str(), NULL), L"error walking source file #" << i << " " << srcFile);
		}
		out.close();

		return err;
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
		wchar_t tmpBuff1 [MAX_PATH];
		strCpy(tmpBuff1, in);
		wchar_t* val = tmpBuff1;
		while(*val != 0)
		{
			if(*val == fileWrongSeparator)
			{
				*val = fileSeparator;
			}
			++val;
		}

		wchar_t tmpBuff2 [MAX_PATH];
		// PathCchCanonicalize
		// PathCchCanonicalizeEx
		if(PathCanonicalize(tmpBuff2, tmpBuff1) == TRUE)
		{
			out = tmpBuff2;
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
	int getUnquoted(sz arg, wchar_t* buffer)
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

		return HDRZ_ERR_OK;
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
	void skipSpaces(wchar_t const*& p)
	{
		while(isSpace(*p))
		{
			++p;
		}
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
