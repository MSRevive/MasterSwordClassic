#ifndef MSSHARED_GROUPFILE_H
#define MSSHARED_GROUPFILE_H
//Groupfile... its just like a pakfile

#include "stackstring.h"
typedef unsigned long ulong;

struct pakHeader_t
{
	int MagicNumber;
	unsigned int DirectoryOffset;
	unsigned int DirectoryCount;
};

struct pakDirectory_t
{
	char cFilename[56];
	unsigned int FileOffset;
	unsigned int FileSize;
};

#ifndef NOT_HLDLL
#include "filesystem_shared.h"

/**
*	@brief Class to read the group file in the game
*	This class handles efficient and secure loading of the game group file.
*	It loads only the header data and keeps a handle to the file to read and decrypt scripts on demand.
*/
class CGameGroupFile
{
public:
	CGameGroupFile();
	~CGameGroupFile();

	bool IsOpen() const { return cFile.IsOpen(); }

	/**
	*	@brief Loads the group file from a given file
	*	If the group file was previously loaded, all data is first purged
	*/
	bool Open(const char* pszFilename);

	/**
	*	@brief Manually close the file and purge all header data
	*/
	void Close();

	bool ReadEntry(const char* pszName, byte* pBuffer, unsigned long& DataSize);

private:
	CFile cFile;
	std::vector<pakDirectory_t> m_EntryList;
};

#endif
#endif