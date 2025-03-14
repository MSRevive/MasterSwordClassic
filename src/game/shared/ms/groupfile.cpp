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

#ifdef _MSR_UTILS
IFileSystem* g_pFileSystem;
#endif // _MSR_UTILS

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

void Print(const char* szFmt, ...);

/*-----------
	CGroupFile - File packwout without encryption.
------------*/
void CGroupFile::Open(char* pszFileName)
{
	strncpy(m_FileName, pszFileName, sizeof(m_FileName));
	m_EntryList.clear();

	CMemFile GroupFile;
	if (GroupFile.ReadFromFile(m_FileName))
	{
		int HeaderSize;
		GroupFile.ReadInt(HeaderSize);

		CMemFile Headers;
		Headers.SetBuffer((GroupFile.m_Buffer + GroupFile.GetReadPtr()), HeaderSize);

		int HeaderEntries;
		Headers.ReadInt(HeaderEntries);

		// Read headers
		for (int i = 0; i < HeaderEntries; i++)
		{
			cachedentry_t entry;
			Headers.Read(&entry, sizeof(groupheader_t)); // Read only the groupheader_t part.  The cachedentry_t part is not stored
			entry.Data = NULL;
			m_EntryList.add(entry);
		}

		// Read existing data
		for (int i = 0; i < m_EntryList.size(); i++)
		{
			cachedentry_t& entry = m_EntryList[i];

			if (entry.DataSize == 0) // Boggus entry?
				continue;

			entry.Data = msnew byte[entry.DataSize];
			memcpy(entry.Data, (GroupFile.m_Buffer + entry.DataOfs), entry.DataSize);
		}

		m_IsOpen = true;
	}
}

void CGroupFile::Close()
{
	for (int i = 0; i < m_EntryList.size(); i++)
	{
		cachedentry_t& Entry = m_EntryList[i];

		delete[] Entry.Data;
		Entry.Data = NULL;
	}

	m_EntryList.clear();
	m_IsOpen = false;
}

//You must call Flush() to actually write the entries
bool CGroupFile::WriteEntry(const char* pszName, byte* pData, size_t DataSize)
{
	msstring EntryName = pszName;
	ReplaceChar(EntryName, '\\', '/');

	DeleteEntry(EntryName);

	cachedentry_t Entry;
	Entry.FileName = EntryName;

	//Entry.DataSize = Data.GetDataSize();
	Entry.DataSize = DataSize;

	Entry.Data = msnew byte[Entry.DataSize];
	memcpy(Entry.Data, pData, Entry.DataSize);

	//Data.GetData( Entry.Data );
	m_EntryList.add(Entry);

	return true;
}

bool CGroupFile::ReadEntry(const char* pszName, byte* pBuffer, size_t& DataSize)
{
	msstring EntryName = pszName;
	ReplaceChar(EntryName, '\\', '/');

	for (int i = 0; i < m_EntryList.size(); i++)
	{
		cachedentry_t& Entry = m_EntryList[i];

		if (Entry.FileName != EntryName)
			continue;

		if (pBuffer)
			memcpy(pBuffer, Entry.Data, Entry.DataSize);
		DataSize = Entry.DataSize;

		return true;
	}

	return false;
}

bool CGroupFile::DeleteEntry(const char* pszName)
{
	for (int i = 0; i < m_EntryList.size(); i++)
	{
		cachedentry_t& Entry = m_EntryList[i];

		if (Entry.FileName == pszName)
		{
			delete[] Entry.Data;
			Entry.Data = NULL;

			m_EntryList.erase(i);
			return true;
		}
	}

	return false;
}

void CGroupFile::Flush()
{
	//Use an ofstream so we don't have to precalculate the file size - Solokiller
	std::ofstream file(m_FileName, std::ios_base::out | std::ios_base::binary);

	if (!file.is_open())
		return;

	int HeaderSize = 0;

	//Open has to know how large the header chunk is in encrypted form. Write a dummy value now, update later
	//This replaces the use of the file size to decrypt the file, instead we know the size of the header and from that header get the sizes of the encrypted scripts
	file.write(reinterpret_cast<char*>(&HeaderSize), sizeof(int));

	const size_t uiHeaderOffset = file.tellp();
	int TotalEntries = m_EntryList.size();
	const size_t uiTotalHeaderSize = sizeof(groupfileheader_t) - sizeof(groupheader_t) + (sizeof(groupheader_t) * TotalEntries);
	groupfileheader_t* pHeaders = reinterpret_cast<groupfileheader_t*>(::operator new(uiTotalHeaderSize));
	pHeaders->TotalEntries = TotalEntries;

	//Write dummy data, update after script data has been written
	{
		CData Data;
		Data.SetData(reinterpret_cast<byte*>(pHeaders), uiTotalHeaderSize);
		file.write(reinterpret_cast<char*>(Data.GetData()), Data.GetDataSize());
	}

	for (int i = 0; i < TotalEntries; i++)
	{
		cachedentry_t& Entry = m_EntryList[i];
		Entry.DataOfs = static_cast<int>(file.tellp());

		CData Data;
		Data.SetData(Entry.Data, Entry.DataSize);

		file.write(reinterpret_cast<char*>(Data.GetData()), Data.GetDataSize()); //[X data]

		//Update entry info
		Entry.DataSize = Data.GetDataSize();
	}

	for (int i = 0; i < TotalEntries; i++)
	{
		pHeaders->Headers[i] = m_EntryList[i];
	}

	//Encrypt headers
	CData Data;
	Data.SetData(reinterpret_cast<byte*>(pHeaders), uiTotalHeaderSize);

	file.seekp(uiHeaderOffset);
	file.write(reinterpret_cast<char*>(Data.GetData()), Data.GetDataSize());

	file.seekp(0, std::ios_base::beg);

	//Write the encrypted header size
	HeaderSize = Data.GetDataSize();
	file.write(reinterpret_cast<char*>(&HeaderSize), sizeof(int));

	delete pHeaders;
	file.close();
}

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

	int HeaderSize = 0;
	if (sizeof(int) != cFile.Read(&HeaderSize, sizeof(int)))
	{
		cFile.Close();
		return false;
	}

	CMemFile Headers;
	{
		//CleanupMemory _HeaderData(msnew byte[HeaderSize]);
		//_HeaderData = std::unique_ptr<byte*>(new byte[HeaderSize]);
		std::unique_ptr<byte[]> _HeaderData = std::make_unique<byte[]>(HeaderSize);

		if (HeaderSize != cFile.Read(_HeaderData.get(), HeaderSize))
		{
			return false;
		}

		CData HeaderData;
		HeaderData.SetData(_HeaderData.get(), HeaderSize);
		Headers.SetBuffer(HeaderData.GetData(), HeaderData.GetDataSize());
	}

	int HeaderEntries;
	Headers.ReadInt(HeaderEntries);

	//Read headers
	for (int i = 0; i < HeaderEntries; i++)
	{
		cachedentry_t Entry;
		Headers.Read(&Entry, sizeof(groupheader_t));

		m_EntryList.add(Entry);
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
		groupheader_t& Entry = m_EntryList[i];
		if (Entry.FileName != EntryName)
			continue;

		DataSize = Entry.DataSize;
		//Decrypt on demand
		if (pBuffer)
		{
			cFile.Seek(Entry.DataOfs, FILESYSTEM_SEEK_HEAD);
			byte* _pBuffer = msnew byte[Entry.DataSize];
			bool bSuccess = false;

			if (Entry.DataSize == cFile.Read(_pBuffer, Entry.DataSize))
			{
				CData Data(_pBuffer, Entry.DataSize);

				//TODO: could check if DataSize matches Data.GetDataSize() here - Solokiller
				memcpy(pBuffer, Data.GetData(), DataSize);
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

// CGameGroupFile::CGameGroupFile()
// {
// }

// CGameGroupFile::~CGameGroupFile()
// {
// 	Close();
// }

// bool CGameGroupFile::Open(const char* pszFilename)
// {
// 	Close();

// 	//Load group files from config directories only (avoids loading downloaded content)
// 	if (!g_pFileSystem->FileExists(pszFilename))
// 	{
// 		return false;
// 	}

// 	m_hFile = g_pFileSystem->Open(pszFilename, "rb", "GAMECONFIG");

// 	if (FILESYSTEM_INVALID_HANDLE == m_hFile)
// 	{
// 		return false;
// 	}

// 	int HeaderSize = 0;
// 	if (sizeof(int) != g_pFileSystem->Read(&HeaderSize, sizeof(int), m_hFile))
// 	{
// 		Close();
// 		return false;
// 	}

// 	/*
// 	*	Helper to free memory automatically
// 	*	TODO: Should use C++11 std::unique_ptr instead
// 	*/
// 	struct CleanupMemory
// 	{
// 		byte* pMemory;

// 		CleanupMemory(byte* pMemory) : pMemory(pMemory)
// 		{
// 		}

// 		~CleanupMemory()
// 		{
// 			delete[] pMemory;
// 		}
// 	};

// 	CMemFile Headers;
// 	{
// 		CleanupMemory _HeaderData(msnew byte[HeaderSize]);

// 		if (HeaderSize != g_pFileSystem->Read(_HeaderData.pMemory, HeaderSize, m_hFile))
// 		{
// 			return false;
// 		}

// 		CData HeaderData;

// 		HeaderData.SetData(_HeaderData.pMemory, HeaderSize);

// 		Headers.SetBuffer(HeaderData.GetData(), HeaderData.GetDataSize());
// 	}

// 	int HeaderEntries;
// 	Headers.ReadInt(HeaderEntries);

// 	//Read headers
// 	for (int i = 0; i < HeaderEntries; i++)
// 	{
// 		cachedentry_t Entry;
// 		Headers.Read(&Entry, sizeof(groupheader_t));

// 		m_EntryList.add(Entry);
// 	}

// 	//Seek to head so we're not left dangling someplace where it might cause problems
// 	g_pFileSystem->Seek(m_hFile, 0, FILESYSTEM_SEEK_HEAD);

// 	return true;
// }

// void CGameGroupFile::Close()
// {
// 	if (FILESYSTEM_INVALID_HANDLE != m_hFile)
// 	{
// 		g_pFileSystem->Close(m_hFile);
// 		m_hFile = FILESYSTEM_INVALID_HANDLE;
// 	}

// 	m_EntryList.clear();
// }

// bool CGameGroupFile::ReadEntry(const char* pszName, byte* pBuffer, unsigned long& DataSize)
// {
// 	msstring EntryName = pszName;
// 	ReplaceChar(EntryName, '\\', '/');

// 	for (int i = 0; i < m_EntryList.size(); i++)
// 	{
// 		groupheader_t& Entry = m_EntryList[i];
// 		if (Entry.FileName != EntryName)
// 			continue;

// 		DataSize = Entry.DataSize;
// 		//Decrypt on demand
// 		if (pBuffer)
// 		{
// 			g_pFileSystem->Seek(m_hFile, Entry.DataOfs, FILESYSTEM_SEEK_HEAD);
// 			byte* _pBuffer = msnew byte[Entry.DataSize];
// 			bool bSuccess = false;

// 			if (Entry.DataSize == g_pFileSystem->Read(_pBuffer, Entry.DataSize, m_hFile))
// 			{
// 				CData Data(_pBuffer, Entry.DataSize);

// 				//TODO: could check if DataSize matches Data.GetDataSize() here - Solokiller
// 				memcpy(pBuffer, Data.GetData(), DataSize);
// 				bSuccess = true;
// 			}

// 			delete[] _pBuffer;

// 			//Seek to head so we're not left dangling someplace where it might cause problems
// 			g_pFileSystem->Seek(m_hFile, 0, FILESYSTEM_SEEK_HEAD);

// 			return bSuccess;
// 		}

// 		return true;
// 	}

// 	return false;
// }

#endif