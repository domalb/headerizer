#include "hdrz.h"
#include "hdrzArgs.h"

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
