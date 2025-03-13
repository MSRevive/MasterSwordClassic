#include "Platform.h"
#include <fstream>
#include "stackstring.h"

class CGameFile
{
public:
	virtual bool OpenWrite(const char* pszFileName);
	virtual bool OpenRead(const char* pszFileName);

	virtual void Close();
	virtual void Reset() { m_File.seekg(0, std::ios::beg); }

	virtual size_t GetFileSize();
	virtual bool Eof() { return m_File.eof(); }

	virtual void Write(void* pvData, size_t Size);
	void WriteByte(byte Data) { Write(&Data, 1); }
	void WriteShort(short Data) { Write(&Data, 2); }
	void WriteInt(int Data) { Write(&Data, sizeof(int)); }
	void WriteFloat(float Data) { Write(&Data, 8); }
	void WriteCoord(class Vector& Data) { Write(&Data, 8 * 3); }
	void WriteString(const char* Data) { Write((void*)Data, strlen(Data) + 1); }

	virtual bool Read(void* pvData, size_t Size);
	void ReadByte(byte& Data) { Read((void*)&Data, 1); }
	void ReadShort(short& Data) { Read((void*)&Data, 2); }
	void ReadInt(int& Data) { Read((void*)&Data, sizeof(int)); }
	void ReadFloat(float& Data) { Read((void*)&Data, 8); }
	void ReadCoord(Vector& Data) { Read((void*)&Data, 8 * 3); }
	void ReadString(char* Data, size_t length);

private:
	std::fstream m_File;
};

// General, load file into memory

class CMemFile : public CGameFile
{
public:
	byte* m_Buffer = nullptr;
	size_t m_BufferSize = 0;

	CMemFile();
	CMemFile(int Alloc);
	virtual ~CMemFile();

	void Open(int Alloc);
	void Close();
	void Reset();

	void Alloc(int Alloc);
	void Dealloc();
	void SetBuffer(byte* pNewBuffer, size_t Size);

	void Write(void* pvData, size_t Size);
	bool Read(void* pvData, size_t Size);
	size_t GetFileSize();

	void WriteToFile(const char* pszFileName);
	bool ReadFromFile(const char* pszFileName);
	bool ReadFromGameFile(const char* pszFileName);

	void SetReadPtr(size_t Ofs) { m_ReadOffset = Ofs; }
	void SetWritePtr(size_t Ofs) { m_WriteOffset = Ofs; }

	size_t GetReadPtr() { return m_ReadOffset; }
	size_t GetWritePtr() { return m_WriteOffset; }

	bool Eof() { return m_ReadOffset >= m_BufferSize; }

private:
	size_t m_ReadOffset, m_WriteOffset;
};

// Player Save File class

class CPlayer_DataBuffer : public CMemFile
{
public:
	CPlayer_DataBuffer() : CMemFile() {}
	CPlayer_DataBuffer(int Alloc) : CMemFile(Alloc) {}

	void WriteToFile(const char* pszFileName, const char* OpenFlags, bool WriteBackup);
	bool ReadFromFile(const char* pszFileName, const char* OpenFlags, bool ReadBackup);
};

#define BACKUP_NAME(a) (msstring(a) + ".backup")