#include "hdrz.h"

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
		PreviouslyIncludedFile(sz filePath, bool onceOnly);
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
		int init(const Input& in);
		int process();
		int tryWalkFile(sz filePath, bool& skipped);
		int walkFile(sz filePath, bool* detectOnce);
		int walkFileStream(std::wistream& in, bool* detectOnce);
		int handleIncludeLine(sz line, sz inclusionSpec, bool quoted);

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
		void addPreviousInclude(const PreviouslyIncludedFile& val);

		std::wostream& comments(sz prefix = L"// ") const;
		std::wostream& out() const;

		bool m_onceGuards3;
		bool m_comments;
		bool m_content;
		sz* m_incDirs;
		size_t m_incDirsCount;
		sz* m_srcFiles;
		size_t m_srcFilesCount;
		std::vector<std::wstring> m_defined;
		std::vector<PreviouslyIncludedFile> m_prevIncluded;

		WalkStack m_walkStack;

		std::wstring m_outFilePath;
		std::wofstream* m_outStream;
		std::wostream* m_commentsStream;
	};

	int detectOncePragma(sz line, bool& detected);
	int detectOnceGuard1(sz line, sz filenameNoExt, size_t filenameNoExtLength, bool& detected);
	int detectOnceGuard2(sz line, sz filenameNoExt, size_t filenameNoExtLength, bool& detected);
	int detectOnceGuard3(sz line, sz filenameNoExt, size_t filenameNoExtLength, bool& detected);
	int detectIncludeLine(sz line, sz& fileNameStart, size_t& fileNameLength);
	int handleIncludeLine(Context& ctxt, std::wostream& out, sz line, sz inclusionSpec, bool quoted);

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
