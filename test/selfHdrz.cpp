// HDRZ : begin of C:\Users\secator\Documents\GitHub\headerizer\src\hdrzImpl.cpp
// HDRZ : begin of C:\Users\secator\Documents\GitHub\headerizer\src\hdrzImpl.h
// HDRZ : begin of C:\Users\secator\Documents\GitHub\headerizer\src\hdrz.h

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

#ifdef _WIN32
	static const wchar_t fileSeparator = L'\\';
	static const wchar_t fileWrongSeparator = L'/';
#else // _WIN
	static const wchar_t fileSeparator = L'/';
	static const wchar_t fileWrongSeparator = L'\\';
#endif // _WIN

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


// HDRZ : end of C:\Users\secator\Documents\GitHub\headerizer\src\hdrz.h

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

// Syntax comments
// File : in c:\\foo\\bar\\fuzz.h
// * 'file path' stands for c:\\foo\\bar\\fuzz.h
// * 'file name' stands for fuzz.h
// * 'file dir' stands for c:\\foo\\bar

namespace hdrz
{
	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	struct PreviouslyIncludedFile
	{
		PreviouslyIncludedFile(const std::wstring& filePath, bool onceOnly);

		std::wstring m_filePath;
		bool m_onceOnly;
	};

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	struct WalkItem
	{
		WalkItem(const std::wstring& fileDir, const std::wstring& fileName);

		std::wstring m_fileDir;
		std::wstring m_fileName;
		std::wstring m_filePath;
	};

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	struct WalkStack
	{
		bool empty() const { return m_items.empty(); }
		size_t size() const { return m_items.size(); }
		void push(const std::wstring& fileDir, const std::wstring& fileName);
		void pop() { m_items.pop_back(); }
		const WalkItem& getTop() const { return m_items[m_items.size() - 1]; }
		WalkItem& getTop() { return m_items[m_items.size() - 1]; }
		WalkItem& getBottom() { return m_items[0]; }
		const WalkItem& operator [] (size_t index) const { return m_items[index]; }
		WalkItem& operator [] (size_t index) { return m_items[index]; }

		std::vector<WalkItem> m_items;
	};

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	struct Context
	{
		Context();
		/// Locate the file for an inclusion like #include "../foo.h" or #include <framework/foo.h>
		/// \param[in] inclusionSpec  Content of the inclusion instruction, without quotes, like '../foo.h' or 'framework/foo.h'
		/// \param[in] quoted  Use of double quotes (true) or angle-brackets (false)
		/// \param[out] resolvedFileDir  Directory where the include spec could be resolved. Empty if the include spec could not be resolved.
		/// \param[out] resolvedFilePath  Canonicalized resolved file path. Empty if the include spec could not be resolved.
		/// \return  Error code, 0 if no error encountered.
		int resolveInclusion(sz inclusionSpec, bool quoted, std::wstring& resolvedFileDir, std::wstring& resolvedFilePath) const;
		PreviouslyIncludedFile* findPreviousInclude(sz absoluteFilePath);
		const std::wstring& getCurrentFilePath() const { return m_walkStack.getTop().m_filePath; }
		const std::wstring& getCurrentFileName() const { return m_walkStack.getTop().m_fileName; }

		bool m_comments;
		sz* m_incDirs;
		size_t m_incDirsCount;
		std::vector<std::wstring> m_defined;
		std::vector<PreviouslyIncludedFile> m_prevIncluded;

		WalkStack m_walkStack;
	};

	int detectOncePragma(sz line, bool& detected);
	int detectOnceGuard1(sz line, sz filenameNoExt, size_t filenameNoExtLength, bool& detected);
	int detectOnceGuard2(sz line, sz filenameNoExt, size_t filenameNoExtLength, bool& detected);
	int detectOnceGuard3(sz line, sz filenameNoExt, size_t filenameNoExtLength, bool& detected);
	int detectIncludeLine(sz line, sz& fileNameStart, size_t& fileNameLength);
	int handleIncludeLine(Context& ctxt, std::wostream& out, sz line, sz inclusionSpec, bool quoted);
	int walkFileStream(Context& ctxt, std::wostream& out, std::wistream& in, bool* detectOnce);
	int walkFile(Context& ctxt, std::wostream& out, sz filePath, bool* detectOnce);

	// Utils
	bool filePathIsAbsolute(sz path);
	void splitFilePathToDirAndName(sz filePath, std::wstring& fileDir, std::wstring& fileName);
	void canonicalizeFilePath(sz in, std::wstring& out);
	void canonicalizeFilePath(std::wstring& inOut);
	bool fileExists(sz fileName);
	bool isSpace(wchar_t c);
	sz skipChars(sz val, wchar_t c);
	sz skipSpaces(sz& val);
	sz skipSequence(bool caseSensitive, sz val, sz sequence, size_t sequenceLength);
	template <size_t sequenceLength> sz skipSequence(bool caseSensitive, sz val, wchar_t const (&sequence)[sequenceLength]) { return skipSequence(caseSensitive, val, sequence, sequenceLength - 1); }
	bool isEndOfLine(sz line);

	//----------------------------------------------------------------------------------------------------------------------
	//
	//----------------------------------------------------------------------------------------------------------------------
	template <size_t t_stLength>
	inline int strCpy(wchar_t(&dst)[t_stLength], sz src)
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
					hdrzLogError(L"error copying string : STRUNCATE");
					return HDRZ_ERR_STRCPY_TRUNCATE;
				}
			break;
			case ERANGE:
				{
					hdrzLogError(L"error copying string : ERANGE");
					return HDRZ_ERR_STRCPY_LENGTH;
				}
			break;
			default:
				{
					hdrzLogError(L"error copying string : " << copyErr);
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
	inline int strNCpy(wchar_t(&dst)[t_stLength], sz src, size_t length)
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
					hdrzLogError(L"error copying " << length << L" of string : STRUNCATE");
					return HDRZ_ERR_STRCPY_TRUNCATE;
				}
			break;
			case ERANGE:
				{
					hdrzLogError(L"error copying " << length << L" of string : ERANGE");
					return HDRZ_ERR_STRCPY_LENGTH;
				}
			break;
			default:
				{
					hdrzLogError(L"error copying " << length << L" of string : " << copyErr);
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
	inline int strCat(wchar_t(&dst)[t_stLength], sz src)
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
				hdrzLogError(L"error appending string : STRUNCATE");
				return HDRZ_ERR_STRCPY_TRUNCATE;
			}
			break;
			case ERANGE:
			{
				hdrzLogError(L"error appending string : ERANGE");
				return HDRZ_ERR_STRCPY_LENGTH;
			}
			break;
			default:
			{
				hdrzLogError(L"error appending string : " << copyErr);
				return HDRZ_ERR_STRCPY_UNEXPECTED;
			}
			break;
			}
		}
	}
}

// HDRZ : end of C:\Users\secator\Documents\GitHub\headerizer\src\hdrzImpl.h

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
			const std::wstring& fileName = ctxt.getCurrentFileName();
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
		wchar_t line [lineLengthMax];
		
		while((in.bad() || in.eof()) == false)
		{
			in.getline(line, lineLengthMax);

			sz firstNonSpace(line);
			if(*skipSpaces(firstNonSpace) == 0)
			{
				// empty line, simply copied
				out << line << std::endl;
				continue;
			}

			if(detectOnceNeeded)
			{
				// detect once pragma
				bool detectedOncePragma = false;
				hdrzReturnIfError(detectOncePragma(line, detectedOncePragma), L"error detecting once pragma while walking line " << lineIndex << L" in file " << ctxt.getCurrentFilePath());
				if(detectedOncePragma)
				{
					*detectOnce = true;
					detectOnceNeeded = false;
					continue;
				}

				// detect once guards
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

		// setup context
		Context ctxt;
		ctxt.m_comments = in.m_comments;
		ctxt.m_incDirs = in.m_incDirs;
		ctxt.m_incDirsCount = in.m_incDirsCount;
		for(size_t i = 0; i < in.m_definesCount; ++i)
		{
			ctxt.m_defined.push_back(in.m_defines[i]);
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
				hdrzReturnIfError(ctxt.resolveInclusion(srcFile, true, resolvedSrcFileDir, resolvedSrcFilePath), L"error resolving source file " << srcFile);
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
			outFile = resolvedOutFilePath.c_str();
		}

		// open output file
		std::wofstream out;
		out.open(outFile, std::wofstream::out);
		if((out.is_open() == false) || out.bad())
		{
			hdrzLogError(L"error opening output file " << in.m_outFile);
			return HDRZ_ERR_OPEN_OUT_FILE;
		}

		// walk source files
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
			hdrzReturnIfError(walkFile(ctxt, out, resolvedSrcFilePath.c_str(), NULL), L"error walking source file #" << i << " " << srcFile);
		}

		// close output file
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

// HDRZ : end of C:\Users\secator\Documents\GitHub\headerizer\src\hdrzImpl.cpp
// HDRZ : begin of C:\Users\secator\Documents\GitHub\headerizer\src\hdrzMain.cpp
// HDRZ : include skipped as it should be included once only
// #include "hdrz.h"
// HDRZ : begin of C:\Users\secator\Documents\GitHub\headerizer\src\hdrzArgs.h
#ifndef _hdrzArgs_
#define _hdrzArgs_

#define HDRZ_ARG_INCLUDE_DIR L"-i="
#define HDRZ_ARG_SRC_FILE L"-f="
#define HDRZ_ARG_WORK_DIR L"-w="
#define HDRZ_ARG_OUT_FILE L"-o="
#define HDRZ_ARG_WIN_EOL L"-weol"
#define HDRZ_ARG_UNIX_EOL L"-ueol"
#define HDRZ_ARG_VERBOSE L"-v"
#define HDRZ_ARG_PAUSE L"-p"
#define HDRZ_ARG_COMMENTS L"-c"

#endif // _hdrzArgs_

// HDRZ : end of C:\Users\secator\Documents\GitHub\headerizer\src\hdrzArgs.h

#include <windows.h>

//----------------------------------------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------------------------------------
int hdrzUnquoteArg(const wchar_t* arg, wchar_t* buffer)
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
			hdrzLogError(L"invalid argument length");
			return HDRZ_ERR_UNQUOTE_ARG_LENGTH;
		}
		else if(val[argLength - 1] != L'"')
		{
			hdrzLogError(L"quote detection error for argument " << arg);
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
int wmain(int argc, wchar_t* argv[] /*, wchar_t* envp[]*/)
{
	static const size_t HDRZ_ARG_INCLUDE_DIR_LENGTH = HDRZ_STR_LEN(HDRZ_ARG_INCLUDE_DIR);
	static const size_t HDRZ_ARG_SRC_FILE_LENGTH = HDRZ_STR_LEN(HDRZ_ARG_SRC_FILE);
	static const size_t HDRZ_ARG_WORK_DIR_LENGTH = HDRZ_STR_LEN(HDRZ_ARG_WORK_DIR);
	static const size_t HDRZ_ARG_OUT_FILE_LENGTH = HDRZ_STR_LEN(HDRZ_ARG_OUT_FILE);

	// Test for pause argument
	for(int i = 1; i < argc; ++i)
	{
		const wchar_t* arg = argv[i];
		if((arg != NULL) && (_wcsicmp(arg, HDRZ_ARG_PAUSE) == 0))
		{
			system("pause");
			break;
		}
	}

	// Test for verbose argument
	for(int i = 1; i < argc; ++i)
	{
		const wchar_t* arg = argv[i];
		if((arg != NULL) && (_wcsicmp(arg, HDRZ_ARG_VERBOSE) == 0))
		{
			std::wcout << L"verbose mode detected" << std::endl;
			hdrz::verbose = true;
		}
	}

	// Parse arguments
	bool comments = false;
	std::vector<std::wstring> argIncDirs;
	std::vector<std::wstring> argSrcDirs;
	std::vector<std::wstring> argSrcFiles;
	std::wstring workDir;
	wchar_t acOutFile [MAX_PATH] = { 0 };
	const wchar_t* eol = HDRZ_ARG_WIN_EOL;
	for(int i = 1; i < argc; ++i)
	{
		const wchar_t* arg = argv[i];
		if(arg == NULL)
		{
			continue;
		}
		else if(_wcsicmp(arg, HDRZ_ARG_PAUSE) == 0)
		{
			continue;
		}
		else if(_wcsicmp(arg, HDRZ_ARG_VERBOSE) == 0)
		{
			continue;
		}
		else if(_wcsicmp(arg, HDRZ_ARG_COMMENTS) == 0)
		{
			comments = true;
		}
		else if(_wcsicmp(arg, HDRZ_ARG_WIN_EOL) == 0)
		{
			eol = HDRZ_ARG_WIN_EOL;
		}
		else if(_wcsicmp(arg, HDRZ_ARG_UNIX_EOL) == 0)
		{
			eol = HDRZ_ARG_UNIX_EOL;
		}
		else if(_wcsnicmp(arg, HDRZ_ARG_WORK_DIR, HDRZ_ARG_WORK_DIR_LENGTH) == 0)
		{
			if(workDir.empty() == false)
			{
				hdrzLogError(L"multiple working directories detected");
				return HDRZ_ERR_MULTIPLE_WORK_DIRS;
			}
			wchar_t buffer [MAX_PATH];
			hdrzReturnIfError(hdrzUnquoteArg(arg + HDRZ_ARG_WORK_DIR_LENGTH, buffer), L"error unquoting argument " << i << " " << arg);
		}
		else if(_wcsnicmp(arg, HDRZ_ARG_INCLUDE_DIR, HDRZ_ARG_INCLUDE_DIR_LENGTH) == 0)
		{
			wchar_t buffer [MAX_PATH];
			hdrzReturnIfError(hdrzUnquoteArg(arg + HDRZ_ARG_INCLUDE_DIR_LENGTH, buffer), L"error unquoting argument " << i << " " << arg);
			argIncDirs.push_back(buffer);
		}
		else if(_wcsnicmp(arg, HDRZ_ARG_SRC_FILE, HDRZ_ARG_SRC_FILE_LENGTH) == 0)
		{
			wchar_t buffer[MAX_PATH];
			hdrzReturnIfError(hdrzUnquoteArg(arg + HDRZ_ARG_SRC_FILE_LENGTH, buffer), L"error unquoting argument " << i << " " << arg);
			hdrz::sz firstWildCard = wcschr(buffer, L'*');
			if(firstWildCard != NULL)
			{
//				hdrz::sz lastSeparator = wcsrchr(buffer, hdrz::fileSeparator);
				// TODO : check lastSeparator vs firstWildCard
				// TODO : list folder
			}
			else
			{
				argSrcFiles.push_back(buffer);
			}
		}
		else if(_wcsnicmp(arg, HDRZ_ARG_OUT_FILE, HDRZ_ARG_OUT_FILE_LENGTH) == 0)
		{
			if(acOutFile[0] != 0)
			{
				hdrzLogError(L"multiple destination files detected");
				return HDRZ_ERR_MULTIPLE_DST_FILES;
			}
			int unquiote = hdrzUnquoteArg(arg + HDRZ_ARG_OUT_FILE_LENGTH, acOutFile);
			if(unquiote != 0)
			{
				return unquiote;
			}
		}
	}

	// Build process input
	std::vector<hdrz::sz> incDirNames;
	incDirNames.reserve(argIncDirs.size());
	for(size_t i = 0; i < argIncDirs.size(); ++i)
	{
		incDirNames.push_back(argIncDirs[i].c_str());
	}
	std::vector<hdrz::sz> srcFileNames;
	srcFileNames.reserve(argSrcFiles.size());
	for(size_t i = 0; i < argSrcFiles.size(); ++i)
	{
		srcFileNames.push_back(argSrcFiles[i].c_str());
	}
	hdrz::Input in;
	memset(&in, 0, sizeof(in));
	in.m_comments = comments;
	in.m_incDirs = incDirNames.data();
	in.m_incDirsCount = incDirNames.size();
	in.m_srcFiles = srcFileNames.data();
	in.m_srcFilesCount = srcFileNames.size();
	in.m_outFile = acOutFile;

	// Invoke process
	int process = hdrz::process(in);

	 return process;
}

// HDRZ : end of C:\Users\secator\Documents\GitHub\headerizer\src\hdrzMain.cpp
