#include "hdrzImpl.h"

#include <sys/stat.h>
#include <windows.h>
#include <assert.h>
#include <shlwapi.h>

namespace hdrz
{
	bool verbose = false;

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	bool getExecutableDirectory(wchar_t* directoryPath)
	{
		BOOL get = GetModuleFileNameW(NULL, directoryPath, MAX_PATH);
		if(get != FALSE)
		{
			wchar_t* found = wcsrchr(directoryPath, fileSeparator);
			if(found != NULL)
			{
				*found = 0;
			}
			return true;
		}

		return false;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	Input::Input()
		: m_onceGuards3(false)
		, m_comments(false)
		, m_content(true)
		, m_defines(NULL)
		, m_definesCount(0)
		, m_incDirs(NULL)
		, m_incDirsCount(0)
		, m_srcFiles(NULL)
		, m_srcFilesCount(0)
		, m_outFile(NULL)
	{

	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	PreviouslyIncludedFile::PreviouslyIncludedFile(sz filePath, bool onceOnly)
		: m_filePath(filePath)
		, m_onceOnly(onceOnly)
	{
	}

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
	int Context::init(const Input& in)
	{
		m_onceGuards3 = in.m_onceGuards3;
		m_comments = in.m_comments;
		m_content = in.m_content;
		m_incDirs = in.m_incDirs;
		m_incDirsCount = in.m_incDirsCount;
		m_srcFiles = in.m_srcFiles;
		m_srcFilesCount = in.m_srcFilesCount;
		m_outStream = NULL;

		for(size_t i = 0; i < in.m_definesCount; ++i)
		{
			m_defined.push_back(in.m_defines[i]);
		}

		// resolve output file path if required
		sz outFile = in.m_outFile;
		bool unspecifiedOutFile = ((outFile == NULL) || (*outFile == 0));
		std::wstring resolvedOutFilePath;
		if(unspecifiedOutFile || (filePathIsAbsolute(outFile) == false))
		{
			if(in.m_srcFilesCount == 1)
			{
				// out put file path built from the single source file path
				sz srcFile = in.m_srcFiles[0];
				std::wstring resolvedSrcFileDir;
				std::wstring resolvedSrcFilePath;
				hdrzReturnIfError(resolveInclusion(srcFile, true, resolvedSrcFileDir, resolvedSrcFilePath), L"error resolving source file " << srcFile);
				if(resolvedSrcFileDir.empty())
				{
					hdrzLogError(L"error resolving single source file path for building output file path");
					return HDRZ_ERR_IN_FILE_NOT_FOUND;
				}
				if(unspecifiedOutFile)
				{
					// append ".hdrz.h" to source file path
					resolvedOutFilePath = resolvedSrcFilePath;
					resolvedOutFilePath += L".hdrz.h";
				}
				else
				{
					// append relative output file path to source file directory
					resolvedOutFilePath = resolvedSrcFileDir;
					resolvedOutFilePath += fileSeparator;
					resolvedSrcFilePath += outFile;
					canonicalizeFilePath(resolvedSrcFilePath);
				}
			}
			else
			{
				wchar_t exeDir[MAX_PATH] = { 0 };
				if(getExecutableDirectory(exeDir) == false)
				{
					hdrzLogError(L"error getting executable directory for building output file path");
					return HDRZ_ERR_CANT_GET_EXE_DIR;
				}
				else
				{
					resolvedOutFilePath = sz(exeDir);
					resolvedOutFilePath += fileSeparator;
					if(unspecifiedOutFile)
					{
						// file is simply named "hdrz.h" in executable directory
						resolvedOutFilePath += L"hdrz.h";
					}
					else
					{
						// append relative output file path to executable directory
						resolvedOutFilePath += outFile;
						canonicalizeFilePath(resolvedOutFilePath);
					}
					hdrzLogInfo(L"output file path built from executable directory" << resolvedOutFilePath);
				}
			}
			m_outFilePath = resolvedOutFilePath;
		}
		else
		{
			m_outFilePath = outFile;
		}

		return HDRZ_ERR_OK;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int Context::process()
	{
		int err = HDRZ_ERR_OK;

		// open output file
		std::wofstream out;
		out.open(m_outFilePath.c_str(), std::wofstream::out);
		if((out.is_open() == false) || out.bad())
		{
			hdrzLogError(L"error opening output file " << m_outFilePath.c_str());
			return HDRZ_ERR_OPEN_OUT_FILE;
		}
		m_outStream = &out;

		// build comments stream
		class NullBuffer : public std::wstreambuf
		{
		public:
			int overflow(int c) { return c; }
		};
		static NullBuffer nullBuffer;
		static std::wostream nullStream(&nullBuffer);
		m_commentsStream = (m_comments ? &out : &nullStream);

		// walk source files
		for(size_t i = 0; i < m_srcFilesCount; ++i)
		{
			sz srcFile = m_srcFiles[i];

			std::wstring resolvedSrcFileDir;
			std::wstring resolvedSrcFilePath;
			hdrzReturnIfError(resolveInclusion(srcFile, true, resolvedSrcFileDir, resolvedSrcFilePath), L"error resolving source file " << srcFile);
			if(resolvedSrcFilePath.empty())
			{
				hdrzLogError(L"could not resolve source file " << srcFile);
				err = HDRZ_ERR_IN_FILE_NOT_FOUND;
				break;
			}
			bool skipped = false;
			hdrzReturnIfError(tryWalkFile(resolvedSrcFilePath.c_str(), skipped), L"error walking source file #" << i << " " << srcFile);
		}

		// close comments stream
		m_commentsStream = NULL;

		// close output file
		m_outStream = NULL;
		out.close();

		return err;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int Context::tryWalkFile(sz filePath, bool& skipped)
	{
		assert(filePathIsAbsolute(filePath));

		PreviouslyIncludedFile* prevFile = findPreviousInclude(filePath);
		bool prevIncluded = (prevFile != NULL);
		bool alreadyOnce = ((prevFile != NULL) && prevFile->m_onceOnly);
		skipped = alreadyOnce;

		if(alreadyOnce)
		{
			comments() << L"HDRZ : file skipped because it should be once only : " << filePath << std::endl;
		}
		else
		{
			if(prevIncluded)
			{
				hdrzReturnIfError(walkFile(filePath, NULL), L"error walking previously included file " << filePath << L" included in " << getCurrentFilePath());
			}
			else
			{
				bool detectOnce = false;
				hdrzReturnIfError(walkFile(filePath, &detectOnce), L"error walking file " << filePath << L" included in " << getCurrentFilePath());
				addPreviousInclude(PreviouslyIncludedFile(filePath, detectOnce));
			}
		}

		return HDRZ_ERR_OK;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int Context::walkFile(sz filePath, bool* detectOnce)
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
		m_walkStack.push(fileDir, fileName);
		comments() << L"HDRZ : begin of " << getCurrentFilePath() << std::endl;

		std::wifstream in;
		in.open(filePath);
		if((in.is_open() == false) || in.bad())
		{
			hdrzLogError(L"error opening read file " << canonFilePath);
			ret = HDRZ_ERR_OPEN_IN_FILE;
		}
		else
		{
			ret = walkFileStream(in, detectOnce);
			in.close();
		}

		comments() << L"HDRZ : end of " << getCurrentFilePath() << std::endl;

		m_walkStack.pop();

		return ret;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int Context::walkFileStream(std::wistream& in, bool* detectOnce)
	{
		// Once detection init
		bool detectOnceNeeded = (detectOnce != NULL);
		bool detectedOnceGuard1 = false;
		bool detectedOnceGuard2 = false;
		bool detectedOnceGuard3 = false;
		wchar_t fileNameNoExt[MAX_PATH] = { 0 };
		size_t filenameNoExtLength = 0;
		if(detectOnceNeeded)
		{
			const std::wstring& fileName = getCurrentFileName();
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
			*detectOnce = false;
		}

		// Lines iteration
		static const size_t lineLengthMax = 2048;
		int lineIndex = 0;
		wchar_t line[lineLengthMax];

		while((in.bad() || in.eof()) == false)
		{
			in.getline(line, lineLengthMax);

			// detect empty line
			sz firstNonSpace(line);
			if(*skipSpaces(firstNonSpace) == 0)
			{
				if(m_content)
				{
					out() << line << std::endl;
				}
				continue;
			}

			// detect include
			sz fileNameStart;
			size_t fileNameLength;
			hdrzReturnIfError(detectIncludeLine(line, fileNameStart, fileNameLength), L"error detecting include line while walking line " << lineIndex << L" in file " << getCurrentFilePath());
			bool detectedInclude = (fileNameStart != NULL);
			if(detectedInclude)
			{
				// Include line found
				wchar_t inclusionSpec[MAX_PATH];
				hdrzReturnIfError(strNCpy(inclusionSpec, fileNameStart, fileNameLength), L"error copying include file name while walking line " << lineIndex << L" in file " << getCurrentFilePath());
				bool quoted = (*(fileNameStart + fileNameLength) == L'\"');
				hdrzReturnIfError(handleIncludeLine(line, inclusionSpec, quoted), L"error handling include of " << inclusionSpec << L" in " << getCurrentFilePath());
				continue;
			}

			if(detectOnceNeeded)
			{
				// detect once pragma
				bool detectedOncePragma = false;
				hdrzReturnIfError(detectOncePragma(line, detectedOncePragma), L"error detecting once pragma while walking line " << lineIndex << L" in file " << getCurrentFilePath());
				if(detectedOncePragma)
				{
					*detectOnce = true;
					detectOnceNeeded = false;
					continue;
				}

				// detect once guards
				if(detectedOnceGuard2)
				{
					bool onceGuardsOk = false;
					if(m_onceGuards3)
					{
						hdrzReturnIfError(detectOnceGuard3(line, fileNameNoExt, filenameNoExtLength, detectedOnceGuard3), L"error detecting once guard3 while walking line " << lineIndex << L" in file " << getCurrentFilePath());
						if(detectedOnceGuard3)
						{
							// detected once guards 1, 2 & 3
							onceGuardsOk = true;
						}
					}
					else
					{
						// detected once guards 1 & 2
						onceGuardsOk = true;
					}
					if(onceGuardsOk)
					{
						*detectOnce = true;
						detectOnceNeeded = false;
					}
				}
				else if(detectedOnceGuard1)
				{
					hdrzReturnIfError(detectOnceGuard2(line, fileNameNoExt, filenameNoExtLength, detectedOnceGuard2), L"error detecting once guard2 while walking line " << lineIndex << L" in file " << getCurrentFilePath());
				}
				else
				{
					hdrzReturnIfError(detectOnceGuard1(line, fileNameNoExt, filenameNoExtLength, detectedOnceGuard1), L"error detecting once guard1 while walking line " << lineIndex << L" in file " << getCurrentFilePath());
				}
			}

			// basic line, simply copied
			if(m_content)
			{
				out() << line << std::endl;
			}
		}
		return HDRZ_ERR_OK;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	int Context::handleIncludeLine(sz line, sz inclusionSpec, bool quoted)
	{
		assert(m_walkStack.empty() == false);

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
			hdrzReturnIfError(resolveInclusion(inclusionSpec, quoted, absIncFileDir, absIncFilePath), L"error resolving inclusion " << getCurrentFilePath());
		}
		if(absIncFilePath.empty())
		{
			if(m_comments)
			{
				comments() << L"HDRZ : include left as-is since no file was found" << std::endl;
			}
			if(m_content)
			{
				out() << line << std::endl;
			}
		}
		else
		{
			bool skipped = false;
			hdrzReturnIfError(tryWalkFile(absIncFilePath.c_str(), skipped), L"error trying to walk file " << absIncFilePath << L" included in " << getCurrentFilePath());
		}

		return HDRZ_ERR_OK;
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
	void Context::addPreviousInclude(const PreviouslyIncludedFile& val)
	{
		assert(findPreviousInclude(val.m_filePath.c_str()) == NULL);

		m_prevIncluded.push_back(val);
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	std::wostream& Context::comments(sz prefix) const
	{
		assert(m_commentsStream != NULL);

		std::wostream& commentsStream = *m_commentsStream;
		for(size_t i = 1; i < m_walkStack.size(); ++i)
		{
			commentsStream << L"    ";
		}
		if(prefix != NULL)
		{
			commentsStream << prefix;
		}
		return commentsStream;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	std::wostream& Context::out() const
	{
		assert(m_outStream != NULL);
		
		return *m_outStream;
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
			if((readPos = skipSequence(true, readPos, L"pragma")) != NULL)
			{
				skipSpaces(readPos);
				if((readPos = skipSequence(true, readPos, L"once")) != NULL)
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
			if((readPos = skipSequence(true, readPos, L"ifndef")) != NULL)
			{
				skipSpaces(readPos);
				readPos = skipChars(readPos, L'_');
				if((readPos = skipSequence(false, readPos, filenameNoExt, filenameNoExtLength)) != NULL)
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
			if((readPos = skipSequence(true, readPos, L"define")) != NULL)
			{
				skipSpaces(readPos);
				readPos = skipChars(readPos, L'_');
				if((readPos = skipSequence(false, readPos, filenameNoExt, filenameNoExtLength)) != NULL)
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
			if((readPos = skipSequence(true, readPos, L"endif")) != NULL)
			{
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
				if((readPos = skipSequence(false, readPos, filenameNoExt, filenameNoExtLength)) != NULL)
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
	int process(const Input& in)
	{
		// setup context
		Context ctxt;
		hdrzReturnIfError(ctxt.init(in), L"error initializing context from input");
		hdrzReturnIfError(ctxt.process(), L"error processing");

		return HDRZ_ERR_OK;
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
	bool isSpace(wchar_t c)
	{
		return ((c == L' ') || (c == L'\t'));
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	sz skipChars(sz val, wchar_t c)
	{
		while(*val == c)
		{
			++val;
		}
		return val;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	sz skipSpaces(sz& val)
	{
		while(isSpace(*val))
		{
			++val;
		}
		return val;
	}

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	sz skipSequence(bool caseSensitive, sz val, sz sequence, size_t sequenceLength)
	{
		int compare = (caseSensitive ? wcsncmp(val, sequence, sequenceLength) : _wcsnicmp(val, sequence, sequenceLength));
		if(compare == 0)
		{
			return val + sequenceLength;
		}
		else
		{
			return NULL;
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
