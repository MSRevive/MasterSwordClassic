// 
// Scriptpack.cpp : Defines the entry point for the console application.
//
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "cbase.h"
#include "packer.h"
#include "crc/crchash.h"
#include "tclap/CmdLine.h"

bool g_Verbose;
bool g_Release;
bool g_ErrFile;
bool g_FailOnErr;
bool g_NoPack;

int main(int argc, char** argv)
{
	try {
		//Program description
		TCLAP::CmdLine cmd("Packs via scripts for use with MSR", ' ', "0.9");
		
		char workDir[MAX_PATH];
		char _outDir[MAX_PATH];
		
		//String arguements
		char rootDir[MAX_PATH];
		_getcwd(rootDir, MAX_PATH);
		_snprintf(workDir, MAX_PATH, "%s\\scripts", rootDir);
		_snprintf(_outDir, MAX_PATH, "%s", rootDir);
		
		TCLAP::ValueArg<char*> oDirArg("o", "output", "Output Directory", false, _outDir, "The output directory for packed scripts");
		cmd.add(oDirArg);
		
		TCLAP::SwitchArg relSwitch("r", "release", "Release build, this will clean the scipts then pack them.", cmd, false);
		TCLAP::SwitchArg verboseSwitch("v", "verbose", "Verbose printing.", cmd, false);
		TCLAP::SwitchArg errFileSwitch("e", "errfile", "Print errors to text file.", cmd, false);
		TCLAP::SwitchArg failErrSwitch("f", "fail", "Exit on script error.", cmd, false);
		TCLAP::SwitchArg packSwitch("p", "pack", "Turn off script packing.", cmd, false);
		
		//Parse command line arguements
		cmd.parse(argc, argv);
		
		char *outDir = oDirArg.getValue();
		//set global vars
		g_Release = relSwitch.getValue();
		g_Verbose = verboseSwitch.getValue();
		g_ErrFile = errFileSwitch.getValue();
		g_FailOnErr = failErrSwitch.getValue();
		
		struct stat info;
		if(stat(workDir, &info) != 0)
		{
			printf("Error: work directory %s not found!\n", workDir);
			exit(-1);
		}

	  	if(stat("./errors.txt", &info) == 0)
			std::remove("./errors.txt");
		
		if(stat(outDir, &info) != 0)
		{
			printf("Error: output directory %s not found!\n", outDir);
		}
		
		Packer packer(workDir, rootDir, outDir);
		packer.readDirectory(workDir);
		packer.processScripts();
		if(!packSwitch.getValue())
		{
			packer.packScripts();
			printf("Wrote changes to the script dll. Hash %u\n\n", GetFileCheckSum("./sc.dll"));
		}
		std::cout << "Finished..." << std::endl;
	} catch (TCLAP::ArgException &err)
	{
		std::cout << "Error: " << err.error() << "for arg " << err.argId() << std::endl;
		exit(-1);
	}

	return 0;
}