#include "hdrz.h"

#define HDRZ_ARG_INCLUDE_DIR L"-i="
static const size_t HDRZ_ARG_INCLUDE_DIR_LENGTH = sizeof(HDRZ_ARG_INCLUDE_DIR) / sizeof(HDRZ_ARG_INCLUDE_DIR[0]);
#define HDRZ_ARG_SRC_DIR L"-d="
static const size_t HDRZ_ARG_SRC_DIR_LENGTH = sizeof(HDRZ_ARG_SRC_DIR) / sizeof(HDRZ_ARG_SRC_DIR[0]);
#define HDRZ_ARG_SRC_FILE L"-f="
static const size_t HDRZ_ARG_SRC_FILE_LENGTH = sizeof(HDRZ_ARG_SRC_FILE) / sizeof(HDRZ_ARG_SRC_FILE[0]);
#define HDRZ_ARG_WIN_EOL L"-weol"
#define HDRZ_ARG_UNIX_EOL L"-ueol"
#define HDRZ_ARG_VERBOSE L"-v"
#define HDRZ_ARG_PAUSE L"-p"

//----------------------------------------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------------------------------------
int wmain(int argc, wchar_t *argv[] /*, wchar_t *envp[]*/)
{
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
	bool verbose = false;
	for(int i = 1; i < argc; ++i)
	{
		const wchar_t* arg = argv[i];
		if((arg != NULL) && (_wcsicmp(arg, HDRZ_ARG_VERBOSE) == 0))
		{
			std::wcout << L"verbose mode detected" << std::endl;
			verbose = true;
		}
	}

	// Parse arguments
	std::vector<std::wstring> arg_inc_dirs;
	std::vector<std::wstring> arg_src_dirs;
	std::vector<std::wstring> arg_src_files;
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
		else if(_wcsicmp(arg, HDRZ_ARG_WIN_EOL) == 0)
		{
			eol = HDRZ_ARG_WIN_EOL;
		}
		else if(_wcsicmp(arg, HDRZ_ARG_UNIX_EOL) == 0)
		{
			eol = HDRZ_ARG_UNIX_EOL;
		}
		else if(_wcsnicmp(arg, HDRZ_ARG_INCLUDE_DIR, HDRZ_ARG_INCLUDE_DIR_LENGTH) == 0)
		{
			wchar_t acBuff [MAX_PATH];
			int iUnquote = hdrz::GetUnquoted(arg + HDRZ_ARG_INCLUDE_DIR_LENGTH, acBuff, verbose);
			if(iUnquote != 0)
			{
				return iUnquote;
			}
			arg_inc_dirs.push_back(acBuff);
		}
		else if(_wcsnicmp(arg, HDRZ_ARG_SRC_DIR, HDRZ_ARG_SRC_DIR_LENGTH) == 0)
		{
			wchar_t acBuff[MAX_PATH];
			int iUnquote = hdrz::GetUnquoted(arg + HDRZ_ARG_INCLUDE_DIR_LENGTH, acBuff, verbose);
			if(iUnquote != 0)
			{
				return iUnquote;
			}
			arg_src_dirs.push_back(acBuff);
		}
		else if(_wcsnicmp(arg, HDRZ_ARG_SRC_FILE, HDRZ_ARG_SRC_FILE_LENGTH) == 0)
		{
			wchar_t acBuff[MAX_PATH];
			int iUnquote = hdrz::GetUnquoted(arg + HDRZ_ARG_INCLUDE_DIR_LENGTH, acBuff, verbose);
			if(iUnquote != 0)
			{
				return iUnquote;
			}
			arg_src_files.push_back(acBuff);
		}
		else
		{
		}
	}

	// Check arguments

	// Write tmp file

	 return 0;
}
