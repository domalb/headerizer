#include "hdrz.h"

#define HDRZ_ARG_INCLUDE_DIR L"-i="
static const size_t HDRZ_ARG_INCLUDE_DIR_LENGTH = HDRZ_STR_LEN(HDRZ_ARG_INCLUDE_DIR);
#define HDRZ_ARG_SRC_DIR L"-d="
static const size_t HDRZ_ARG_SRC_DIR_LENGTH = HDRZ_STR_LEN(HDRZ_ARG_SRC_DIR);
#define HDRZ_ARG_SRC_FILE L"-f="
static const size_t HDRZ_ARG_SRC_FILE_LENGTH = HDRZ_STR_LEN(HDRZ_ARG_SRC_FILE);
#define HDRZ_ARG_WORK_DIR L"-w="
static const size_t HDRZ_ARG_WORK_DIR_LENGTH = HDRZ_STR_LEN(HDRZ_ARG_WORK_DIR);
#define HDRZ_ARG_OUT_FILE L"-o="
static const size_t HDRZ_ARG_OUT_FILE_LENGTH = HDRZ_STR_LEN(HDRZ_ARG_OUT_FILE);
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
	std::vector<std::wstring> argIncDirs;
	std::vector<std::wstring> argSrcDirs;
	std::vector<std::wstring> argSrcFiles;
	std::wstring workDir;
	wchar_t acDstFile [MAX_PATH] = { 0 };
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
		else if(_wcsnicmp(arg, HDRZ_ARG_WORK_DIR, HDRZ_ARG_WORK_DIR_LENGTH) == 0)
		{
			if(workDir.empty() == false)
			{
				if(verbose)
				{
					std::wcout << L"multiple working directories detected" << std::endl;
				}
				return HDRZ_ERR_MULTIPLE_WORK_DIRS;
			}
			wchar_t acBuff [MAX_PATH];
			int unquiote = hdrz::GetUnquoted(arg + HDRZ_ARG_INCLUDE_DIR_LENGTH, acBuff, verbose);
			if(unquiote != 0)
			{
				return unquiote;
			}
		}
		else if(_wcsnicmp(arg, HDRZ_ARG_INCLUDE_DIR, HDRZ_ARG_INCLUDE_DIR_LENGTH) == 0)
		{
			wchar_t acBuff [MAX_PATH];
			int unquiote = hdrz::GetUnquoted(arg + HDRZ_ARG_INCLUDE_DIR_LENGTH, acBuff, verbose);
			if(unquiote != 0)
			{
				return unquiote;
			}
			argIncDirs.push_back(acBuff);
		}
		else if(_wcsnicmp(arg, HDRZ_ARG_SRC_DIR, HDRZ_ARG_SRC_DIR_LENGTH) == 0)
		{
			wchar_t acBuff[MAX_PATH];
			int unquiote = hdrz::GetUnquoted(arg + HDRZ_ARG_INCLUDE_DIR_LENGTH, acBuff, verbose);
			if(unquiote != 0)
			{
				return unquiote;
			}
			argSrcDirs.push_back(acBuff);
		}
		else if(_wcsnicmp(arg, HDRZ_ARG_SRC_FILE, HDRZ_ARG_SRC_FILE_LENGTH) == 0)
		{
			wchar_t acBuff[MAX_PATH];
			int unquiote = hdrz::GetUnquoted(arg + HDRZ_ARG_INCLUDE_DIR_LENGTH, acBuff, verbose);
			if(unquiote != 0)
			{
				return unquiote;
			}
			argSrcFiles.push_back(acBuff);
		}
		else if(_wcsnicmp(arg, HDRZ_ARG_SRC_FILE, HDRZ_ARG_SRC_FILE_LENGTH) == 0)
		{
			wchar_t acBuff[MAX_PATH];
			int unquiote = hdrz::GetUnquoted(arg + HDRZ_ARG_INCLUDE_DIR_LENGTH, acBuff, verbose);
			if(unquiote != 0)
			{
				return unquiote;
			}
			argSrcFiles.push_back(acBuff);
		}
		else if(_wcsnicmp(arg, HDRZ_ARG_OUT_FILE, HDRZ_ARG_OUT_FILE_LENGTH) == 0)
		{
			if(acDstFile[0] != 0)
			{
				if(verbose)
				{
					std::wcout << L"multiple destination files detected" << std::endl;
				}
				return HDRZ_ERR_MULTIPLE_DST_FILES;
			}
			int unquiote = hdrz::GetUnquoted(arg + HDRZ_ARG_OUT_FILE_LENGTH, acDstFile, verbose);
			if(unquiote != 0)
			{
				return unquiote;
			}
		}
	}

	// Check
	if(acDstFile[0] == 0)
	{
		if(argSrcFiles.size() == 1)
		{
			hdrz::sz srcFileName = argSrcFiles[0].c_str();
			hdrzReturnIfError(hdrz::StrCpy(acDstFile, srcFileName, verbose), L"error building output filename");
			hdrzReturnIfError(hdrz::StrCat(acDstFile, L".hdrz", verbose), L"error adding tag to output filename");
			hdrz::sz srcFileExt = wcsrchr(srcFileName, '.');
			if(srcFileExt != NULL)
			{
				hdrzReturnIfError(hdrz::StrCat(acDstFile, srcFileExt, verbose), L"error adding extention to output filename");
			}
		}
		else
		{
			if(verbose)
			{
				std::wcout << L"no destination file" << std::endl;
			}
		}
		return HDRZ_ERR_NO_DST_FILE;
	}

	// Write tmp file
	std::vector<hdrz::sz> incDirNames;
	for(size_t i = 0; i < argIncDirs.size(); ++i)
	{
		incDirNames.push_back(argIncDirs[i].c_str());
	}
	std::vector<hdrz::sz> srcFileNames;
	for(size_t i = 0; i < argSrcFiles.size(); ++i)
	{
		srcFileNames.push_back(argSrcFiles[i].c_str());
	}

	hdrz::input in;
	memset(&in, 0, sizeof(in));
	in.incDirs = incDirNames.data();
	in.incDirsCount = incDirNames.size();
	in.srcFiles = srcFileNames.data();
	in.srcFilesCount = srcFileNames.size();
	in.dstFile = acDstFile;
	int process = hdrz::Process(in, verbose);

	 return process;
}
