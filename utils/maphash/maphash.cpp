// map_hash.cpp : Defines the entry point for the console application.
// Program is a modified version of https://gist.github.com/xandout/8443812
//
#include <stdio.h>
#include <string>
#include <vector>
#include <windows.h>

#include "stream_safe.h"
#include "crc/crchash.h"
#include "rapidjson/document.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/writer.h"

using namespace std;
using namespace rapidjson;

string getFileExt(string fileName)
{
	size_t perd = fileName.find_last_of(".");
	string ext = fileName.substr(perd + 1);
	return ext;
}

string stripExt(string fileName)
{
	size_t lastdot = fileName.find_last_of(".");
	if (lastdot == std::string::npos)
		return fileName;
	return fileName.substr(0, lastdot);
}

// All of the hard work
void lsfiles(vector<string> &files, string folder)  // (c) http://stackoverflow.com/a/20847429/1009816
{
	files.clear();
	char search_path[MAX_PATH];
	_snprintf(search_path, MAX_PATH, "%s*.*", folder.c_str());

	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path, &fd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			// read all (real) files in current folder, delete '!' read other 2 default folder . and ..
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				files.push_back(fd.cFileName);
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}
}

int main(int argc, char *argv[])
{
	string where = "";
	string output = "";

	for (int i = 0; i < argc; i++)
	{
		if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--dir"))
		where = argv[i + 1] + string("\\");

		if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--out"))
			output = argv[i + 1] + string("\\");
	}

	vector<string> list;
	lsfiles(list, where); // Get contents of directory

	string fOutput = output + "maps.json";
	FILE* fp = fopen(fOutput.c_str(), "wb"); // non-Windows use "w"

	char writeBuffer[65536];
	FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
	Writer<FileWriteStream> fileStream(os);
	fileStream.StartObject();

	for (const string &val : list)
	{
		if (getFileExt(val.c_str()) == "bsp")
		{
			string fullpath = where + val.c_str();
			cout << "Writing file hash for: " << val.c_str() << endl;
			fileStream.Key(stripExt(val.c_str()).c_str());
			fileStream.Int64(GetFileCheckSum(fullpath.c_str()));
		}
	}

	fileStream.EndObject();
	fclose(fp);

	cout << "\n";
	cout << "Operation has finished. It is safe to close now." << endl;
	cout << "File Located in: " << fOutput.c_str() << endl;
	cout << "\n";
	cout << "Press 'enter' to close." << endl;
	cin.ignore();

	return 0;
}