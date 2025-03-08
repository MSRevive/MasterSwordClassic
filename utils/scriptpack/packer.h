#ifndef PACKER_H
#define PACKER_H

#include "sharedutil.h"
#include "msfileio.h"
#include "groupfile.h"

typedef unsigned char byte;

class Packer 
{
public:
	Packer(char *wDir, char *rDir, char *oDir);
	void readDirectory(char *pszName, bool cooked = false);
	void processScripts();
	void packScripts();
  
private:
	char m_WorkDir[MAX_PATH];
	char m_RootDir[MAX_PATH];
	char m_CookedDir[MAX_PATH];
	char m_OutDir[MAX_PATH];
	msstringlist m_StoredFiles;
	msstringlist m_CookedFiles;
	
	void doParser(byte *buffer, size_t bufferSize, char *name, char *create, bool errOnly);
};

#endif