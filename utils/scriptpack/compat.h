//credit to https://stackoverflow.com/a/29828907
#pragma once

#include <iostream>
#include <string>
#include <sys/stat.h> // stat
#include <errno.h>    // errno, ENOENT, EEXIST
#if defined(_WIN32)
#include <direct.h>   // _mkdir
#endif

namespace Compat 
{
	bool isDirExist(const std::string& path)
	{
	#if defined(_WIN32)
		struct _stat info;
		if (_stat(path.c_str(), &info) != 0)
		{
			return false;
		}
		return (info.st_mode & _S_IFDIR) != 0;
	#else 
		struct stat info;
		if (stat(path.c_str(), &info) != 0)
		{
			return false;
		}
		return (info.st_mode & S_IFDIR) != 0;
	#endif
	}

	bool makePath(const std::string& path)
	{
	#if defined(_WIN32)
		int ret = _mkdir(path.c_str());
	#else
		mode_t mode = 0755;
		int ret = mkdir(path.c_str(), mode);
	#endif
		if (ret == 0)
			return true;

		switch (errno)
		{
		case ENOENT:
			// parent didn't exist, try to create it
			{
				int pos = path.find_last_of('/');
				if (pos == std::string::npos)
	#if defined(_WIN32)
					pos = path.find_last_of('\\');
				if (pos == std::string::npos)
	#endif
					return false;
				if (!makePath( path.substr(0, pos) ))
					return false;
			}
			// now, try to create again
	#if defined(_WIN32)
			return 0 == _mkdir(path.c_str());
	#else 
			return 0 == mkdir(path.c_str(), mode);
	#endif

		case EEXIST:
			// done!
			return isDirExist(path);

		default:
			return false;
		}
	}
}