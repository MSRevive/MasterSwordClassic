#include "msfileio.h"

#ifndef NOT_HLDLL
#include "extdll.h"
#include "util.h"
#else
#include "sharedutil.h"
#endif
#include "filesystem_shared.h"

//
//	Implementation of CGameFile and CPlayer_DataBuffer
//

bool CGameFile::OpenWrite(const char* pszFileName)
{
	m_File.open(pszFileName, std::ios_base::out | std::ios_base::binary);
	if (!m_File.is_open())
		return false;

	Reset();
	return true;
}

bool CGameFile::OpenRead(const char* pszFileName)
{
	m_File.open(pszFileName, std::ios_base::in | std::ios_base::binary);
	if (!m_File.is_open())
		return false;

	Reset();
	return true;
}

void CGameFile::Close()
{
	m_File.flush();
	m_File.close();
}

void CGameFile::Write(void* pvData, size_t Size)
{
	m_File.write((const char*)pvData, Size);
}

bool CGameFile::Read(void* pvData, size_t Size)
{
	if (pvData == NULL)
		return false;

	if (Size)
		((char*)pvData)[0] = 0;

	m_File.read((char*)pvData, Size);
	return !m_File.bad();
}

void CGameFile::ReadString(char* Data, size_t length)
{
	byte ReadIn = 0;
	Data[0] = 0;
	int i = 0;
	do
	{
		if (!Read(&ReadIn, 1))
			break;
		Data[i++] = ReadIn;
	} while (ReadIn && (i < length));
}

size_t CGameFile::GetFileSize()
{
	m_File.seekg(0, std::ios::cur);
	int CurPos = m_File.tellg();
	m_File.seekg(0, std::ios::end);
	int Size = m_File.tellg();
	m_File.seekg(CurPos, std::ios::beg);
	return Size;
}

CMemFile::CMemFile()
{
	m_BufferSize = 0;
	m_ReadOffset = 0;
	m_WriteOffset = 0;
	m_Buffer = nullptr;
}

CMemFile::CMemFile(int Alloc) : CMemFile()
{
	Open(Alloc);
}

CMemFile::~CMemFile()
{
	Dealloc();
}

void CMemFile::Open(int iAlloc)
{
	m_ReadOffset = m_WriteOffset = 0;
	Alloc(iAlloc);
}

void CMemFile::Alloc(int Alloc)
{
	Dealloc();
	m_Buffer = new byte[Alloc];
	m_BufferSize = Alloc;
}

void CMemFile::Dealloc()
{
	delete[] m_Buffer;
	m_Buffer = nullptr;
	m_BufferSize = 0;
}

void CMemFile::SetBuffer(byte* pNewBuffer, size_t Size)
{
	Dealloc();
	Alloc(Size);
	memcpy(m_Buffer, pNewBuffer, Size);
}

void CMemFile::Close()
{
	Dealloc();
	m_ReadOffset = m_WriteOffset = 0;
}

void CMemFile::Reset()
{
	m_ReadOffset = m_WriteOffset = 0;
}

void CMemFile::Write(void* pvData, size_t Size)
{
	memcpy(&m_Buffer[m_WriteOffset], pvData, Size);
	m_WriteOffset += Size;
	//m_BufferSize += Size;
}

bool CMemFile::Read(void* pvData, size_t Size)
{
	if (m_ReadOffset + Size > m_BufferSize)
		return false;

	memcpy(pvData, &m_Buffer[m_ReadOffset], Size);
	m_ReadOffset += Size;
	return true;
}

void CMemFile::WriteToFile(const char* pszFileName)
{
	if (!CGameFile::OpenWrite(pszFileName))
		return;

	CGameFile::Write(m_Buffer, m_BufferSize);
	CGameFile::Close();
}

bool CMemFile::ReadFromGameFile(const char* pszFileName)
{
	// Load a half-life engine file - could be compressed in a package
	const auto fileContents = FileSystem_LoadFileIntoBuffer(pszFileName, FileContentFormat::Binary);
	
	if (fileContents.empty())
		return false;

	Dealloc();
	m_BufferSize = fileContents.size();
	m_Buffer = new byte[m_BufferSize];
	memcpy(m_Buffer, fileContents.data(), m_BufferSize);

	return true;
}

bool CMemFile::ReadFromFile(const char* pszFileName)
{
	bool Success = CGameFile::OpenRead(pszFileName);
	if (!Success)
		return false;

	Dealloc();
	m_BufferSize = CGameFile::GetFileSize();
	m_Buffer = new byte[m_BufferSize];

	bool bResult = CGameFile::Read(m_Buffer, m_BufferSize);

	CGameFile::Close();
	return bResult;
}

size_t CMemFile::GetFileSize()
{
	return m_BufferSize;
}

void CPlayer_DataBuffer::WriteToFile(const char* pszFileName, const char* OpenFlags, bool WriteBackup)
{
	//Print( "WriteToFile Size: %i\n", m_BufferSize );
	CMemFile::WriteToFile(pszFileName);
	if (WriteBackup) //Write backup
		CMemFile::WriteToFile(BACKUP_NAME(pszFileName));
}

bool CPlayer_DataBuffer::ReadFromFile(const char* pszFileName, const char* OpenFlags, bool ReadBackup)
{
	//Print( "ReadFromFile Size: %i\n", CGameFile::GetFileSize( ) );
	bool Success = CMemFile::ReadFromFile(pszFileName);
	if ((!Success || !CMemFile::GetFileSize()) && ReadBackup) //File doesn't exist or is zero length, try reading backup file
		Success = CMemFile::ReadFromFile(BACKUP_NAME(pszFileName));
	return Success;
}