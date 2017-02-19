#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

typedef const wchar_t* hdrzSZ;

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

#define HDRZ_WIN_EOL L"\n"
#define HDRZ_UNIX_EOL L"\n"

#define HDRZ_ERR_UNQUOTE_ARG_LENGTH -2
#define HDRZ_ERR_UNQUOTE_DETECT -3

//----------------------------------------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------------------------------------
int hdrzGetUnquoted(hdrzSZ arg, wchar_t* buffer)
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
			std::wcout << L"invalid argument length";
			return HDRZ_ERR_UNQUOTE_ARG_LENGTH;
		}
		else if(val[argLength - 1] != L'"')
		{
			std::wcout << L"quote detection error for argument " << arg;
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
int wmain(int argc, wchar_t *argv[] /*, wchar_t *envp[]*/)
{
	// Test for pause argument
	for(int i = 1; i < argc; ++i)
	{
		hdrzSZ arg = argv[i];
		if((arg != NULL) && (_wcsicmp(arg, HDRZ_ARG_PAUSE) == 0))
		{
			system("pause");
			break;
		}
	}

	// Parse arguments
	std::vector<std::string> arg_inc_dirs;
	std::vector<std::string> arg_src_dirs;
	std::vector<std::string> arg_src_files;
	bool verbose = false;
	const wchar_t* eol = HDRZ_ARG_WIN_EOL;
	for(int i = 1; i < argc; ++i)
	{
		hdrzSZ arg = argv[i];
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
			verbose = true;
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
			arg_inc_dirs.push_back(hdrzGetUnquoted(arg + HDRZ_ARG_INCLUDE_DIR_LENGTH));
		}
		else if(_wcsnicmp(arg, HDRZ_ARG_SRC_DIR, HDRZ_ARG_SRC_DIR_LENGTH) == 0)
		{
			arg_inc_dirs.push_back(hdrzGetUnquoted(arg + HDRZ_ARG_SRC_DIR_LENGTH));
		}
		else if(_wcsnicmp(arg, HDRZ_ARG_SRC_FILE, HDRZ_ARG_SRC_FILE_LENGTH) == 0)
		{
			arg_inc_dirs.push_back(hdrzGetUnquoted(arg + HDRZ_ARG_SRC_FILE_LENGTH));
		}
		else
		{
		}
	}

	// Check arguments

	// Write tmp file

	 return 0;
}
