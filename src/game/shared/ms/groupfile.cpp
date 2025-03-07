#include "Platform.h"
#ifdef _WIN32
#include "PlatformWin.h"
#include "logger.h"
#else
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include "extdll.h"
#endif

#include "groupfile.h"
#include "msfileio.h"
#include <memory>

//Deuplicated from msdebug.h
#ifdef NOT_HLDLL
#define msnew new
#elif DEV_BUILD
void* operator new(size_t size, const char* pszSourceFile, int LineNum);
void operator delete(void* ptr, const char* pszSourceFile, int LineNum);
#define msnew new (__FILE__, __LINE__)
#else
#define msnew new
#endif

/*-----------
	CGameGroupFile
------------*/
#ifndef NOT_HLDLL
#include "filesystem_shared.h"

CGameGroupFile::CGameGroupFile() : cFile()
{
}

CGameGroupFile::~CGameGroupFile()
{
	cFile.Close();
}

bool CGameGroupFile::Open(const char* pszFilename)
{
	//Load group files from config directories only (avoids loading downloaded content)
	if (!cFile.FileExists(pszFilename))
		return false;

	if (!cFile.Open(pszFilename, "rb", "GAMECONFIG"))
		return false;

	pakHeader_t Header;

	cFile.Read(&Header, sizeof(Header));

	if (Header.MagicNumber != 1262698832)
	{
		cFile.Close();
		return false;
	}

	cFile.Seek(Header.DirectoryOffset, FILESYSTEM_SEEK_HEAD);

	//Read entries
	for (int i = 0; i < Header.DirectoryCount; i++)
	{
		pakDirectory_t Entry;
		cFile.Read(&Entry, sizeof(pakDirectory_t));

		m_EntryList.push_back(Entry);
	}

	//Seek to head so we're not left dangling someplace where it might cause problems
	cFile.Seek(0, FILESYSTEM_SEEK_HEAD);

	return true;
}

void CGameGroupFile::Close()
{
	cFile.Close();
	m_EntryList.clear();
}

bool CGameGroupFile::ReadEntry(const char* pszName, byte* pBuffer, unsigned long& DataSize)
{
	msstring EntryName = pszName;
	ReplaceChar(EntryName, '\\', '/');

	for (int i = 0; i < m_EntryList.size(); i++)
	{
		pakDirectory_t Entry = m_EntryList[i];
		if (strcmp(Entry.cFilename, EntryName) != 0)
			continue;

		DataSize = Entry.FileSize;
		if (pBuffer)
		{
			cFile.Seek(Entry.FileOffset, FILESYSTEM_SEEK_HEAD);
			byte* _pBuffer = msnew byte[Entry.FileSize];
			bool bSuccess = false;

			if (Entry.FileSize == cFile.Read(_pBuffer, Entry.FileSize))
			{
				memcpy(pBuffer, _pBuffer, DataSize);
				bSuccess = true;
			}

			delete[] _pBuffer;

			//Seek to head so we're not left dangling someplace where it might cause problems
			cFile.Seek(0, FILESYSTEM_SEEK_HEAD);

			return bSuccess;
		}

		return true;
	}

	return false;
}

#endif