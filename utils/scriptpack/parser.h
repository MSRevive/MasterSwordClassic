#ifndef PARSER_H
#define PARSER_H

#include <iterator>
#include <sstream>
#include <regex>
#include <ctype.h>
#include <stack>
#include <sys/stat.h>

#include "cbase.h"

class Parser
{
public:
	Parser(char *data, char *file) : m_Data(data) {
		m_FileName = file;
		m_Result = m_Data;
	}

	void stripComments()
	{
		std::istringstream ss(m_Result);
		std::string line;
		std::string newRes;

		//we have to build a new result.
		while(getline(ss, line))
		{
			std::string nLine = "";
			size_t lineLen = line.length();

			for (int i = 0; i < lineLen; i++)
			{
				const char ch = line[i];
				const char nextch = line[i+1]; //get next ch.

				//remove comments.
				if (ch == '/' && nextch == '/')
					break;

				//just remove return carriages here instead of doing erase.
				if (ch == '\r')
					break;

				nLine += ch;
			}

			newRes += nLine;
			newRes += "\n";
		}

		m_Result = newRes;
	}

	/*
	//credits to https://codereview.stackexchange.com/a/215913
	void stripComments()
	{
	State cState = State::NotAComment;
	std::string res;

	for (size_t i = 0; i < m_Result.length(); i++)
	{
		const char ch = m_Result[i];
		switch (cState)
		{
		case State::SlashOC:
			if (ch == '/')
			cState = State::SingleLineComment;
			else
			{
			// ?????
			cState = State::NotAComment;
			res += ch;
			}
			break;
		case State::SingleLineComment:
			if (ch == '\n')
			{
			cState = State::NotAComment;
			res += '\n';
			}
			break;
		case State::NotAComment:
			if (ch == '/')
			cState = State::SlashOC;
			else
			res += ch;

			break;
		}
	}

	m_Result = res;
	}*/

	//strip whitespaces per line.
	void stripWhitespace()
	{
		std::istringstream ss(m_Result);
		std::string line;
		std::string newRes;

		//we have to build a new result.
		while(getline(ss, line))
		{
			//remove tabs at beginning of line.
			line.erase(0, line.find_first_not_of(" \t\v"));

			//skip over empty lines.
			if (line.find_first_not_of(" \r\t") != std::string::npos)
			{
				//remove extra spaces
				//credit to https://stackoverflow.com/questions/35301432/remove-extra-white-spaces-in-c
				line.erase(std::unique(std::begin(line), std::end(line), [](unsigned char a, unsigned char b){
					return isSpace(a) && isSpace(b);
				}), std::end(line));

				newRes += line;
				newRes += "\n";
			}
		}

		m_Result = newRes;
	}

	// void stripDebug()
	// {
	// 	std::istringstream ss(m_Result);
	// 	std::string line;
	// 	std::string newRes;
	// 	std::regex re(R"(dbg (".*?"))");
	// 	std::smatch m;

	// 	//we have to build a new result without the dbg lines.
	// 	while (getline(ss, line))
	// 	{
	// 		if(!std::regex_search(line, m, re))
	// 		{
	// 			newRes += line;
	// 			newRes += "\n";
	// 		}
	// 	}

	// 	m_Result = newRes;
	// }
	void stripDebug()
	{
		std::istringstream ss(m_Result);
		std::string line;
		std::string newRes;

		while(getline(ss, line))
		{
			if (line.substr(0, 3) != "dbg")
			{
				newRes += line;
				newRes += "\n";
			}
		}

		m_Result = newRes;
	}

	void checkQuotes()
	{
		State cState = State::NoQuote;
		std::istringstream ss(m_Result);
		std::string line;
		size_t lineNum = 1;

		while (getline(ss, line)) 
		{	
			//don't process if line is empty
			size_t lineLen = line.length();
			if (!line.empty() && !onlySpace(line))
			{
				for (size_t pos = 0; pos < lineLen; pos++)
				{
					const char ch = line[pos];
					
					switch(cState)
					{
					case State::InDoubleQuote:
						if (ch == '"')
							cState = State::NoQuote;
						else if (pos == (line.length()))
						{
							cState = State::NoQuote;
							quoteError(lineNum, pos);
						}
						break;
					case State::InSingleQuote:
						if (ch == '\'')
							cState = State::NoQuote;
						else if (pos == (line.length()))
						{
							cState = State::NoQuote;
							quoteError(lineNum, pos);
						}
						break;
					case State::InPara:
						if (ch == ')')
							cState = State::NoQuote;
						else if (pos == (line.length()))
						{
							cState = State::NoQuote;
							addError("%s:%u.%u: expected closing parentheses", lineNum, pos);
						}
						break;
					case State::NoQuote:
						if (ch == '"')
							cState = State::InDoubleQuote;
						else if (ch == '\'')
							cState = State::InSingleQuote;
						else if (ch == '(')
							cState = State::InPara;
						break;
					}
				}
			}

			lineNum++;
		}
	}

	void checkBrackets()
	{
		std::istringstream ss(m_Result);
		std::string line;
		std::vector<std::pair<size_t, size_t>> openBrace{};
		size_t lineNum = 1;

		while (getline(ss, line))
		{
			size_t lineLen = line.length();
			for(int pos = 0; pos < lineLen; pos++)
			{
				switch(line[pos])
				{
				case '{':
					openBrace.push_back(std::make_pair(lineNum, pos));
					break;
				case '}':
					if (openBrace.size() > 0) //don't try to pop empty vector, bad things happen...
						openBrace.pop_back();
					break;
				}
			}

			lineNum++;
		}

		for (std::pair<size_t, size_t> ob : openBrace)
			addError("%s:%u.%u: brace opened, but never closed", ob.first, ob.second);
	}

	//false if script passes, true if script fails.
	bool errorCheck()
	{
		if (m_ErrorList.size() > 0)
			return true;
		else
			return false;
	}

	std::string getResult()
	{
		return m_Result;
	}

	std::vector<std::string> getErrorlist()
	{
		return m_ErrorList;
	}

	void printErrors()
	{
		std::cout << m_ErrorList.size() << " errors found" << std::endl;
		for (auto i: m_ErrorList)
			std::cout << '\t' << i << std::endl;
	}

	void saveErrors()
	{
		std::ofstream o;
		o.open("./errors.txt", std::ios_base::app | std::ios_base::out);

		for (auto i: m_ErrorList)
			o << i << std::endl;
			
		o.close();
	}

	void saveResult(char *create)
	{
		char dir[MAX_PATH];
		int ret = _snprintf(dir, MAX_PATH, "%s", getBaseDir(create));
		std::string sdir(dir);

		//terrible method, but only way to make sure it wrote to char array properly?
		if (ret >= 5)
			createDirectoryRecursively(sdir);
		else
			std::cout << "ERROR: MAX PATH limit exceeded" << std::endl;

		std::ofstream o;
		o.open(create, std::ios_base::trunc);
		o << m_Result;
		o.close();
	}

private:
	enum class State : char
	{
		InDoubleQuote,
		InSingleQuote,
		InPara,
		NoQuote,
	};

	//we have to use our own getline because of the mixed line endings.
	//credits to https://gist.github.com/josephwb/df09e3a71679461fc104
	std::istream &getline(std::istream &is, std::string &t) { 
		t.clear();

		// The characters in the stream are read one-by-one using a std::streambuf.
		// That is faster than reading them one-by-one using the std::istream.
		// Code that uses streambuf this way must be guarded by a sentry object.
		// The sentry object performs various tasks,
		// such as thread synchronization and updating the stream state.

		std::istream::sentry se(is, true);
		std::streambuf* sb = is.rdbuf();

		for (;;) {
			int c = sb->sbumpc();
			switch (c) {
				case '\n':
					return is;
				case '\r':
					if (sb->sgetc() == '\n') {
						sb->sbumpc();
					}
					return is;
				case EOF:
					// Also handle the case when the last line has no line ending
					if (t.empty()) {
						is.setstate(std::ios::eofbit);
					}
					return is;
				default:
					t += (char)c;
			}
		}
	}

	bool onlySpace(const std::string &str)
	{
		return std::all_of(str.begin(), str.end(), isspace);
	}

	static bool isSpace(const char &ch)
	{
		switch(ch)
		{
		case ' ':
			return true;
		case '\t':
			return true;
		case '\v':
			return true;
		default:
			return false;
		}
	}

	void addError(const char *fmt, size_t lineNum, size_t pos)
	{
		char eBuffer[256];
		snprintf(eBuffer, 256, fmt, m_FileName, lineNum, pos);
		std::string s(eBuffer);
		m_ErrorList.push_back(s);
	}

	void quoteError(size_t line, size_t pos)
	{
		addError("%s:%u.%u: unclosed quotation", line, pos);
	}

	//create directory recursively for scripts
	//modified from this https://gist.github.com/danzek/d7192d250c951804dec05125f5223a30
	void createDirectoryRecursively(std::string &path)
	{
		static const std::string separators("\\/");

		struct stat info;
		if(stat(path.c_str(), &info) != 0)
		{
			// Recursively do it all again for the parent directory, if any
			size_t slashIndex = path.find_last_of(separators);
			if(slashIndex != std::string::npos) 
			{
				createDirectoryRecursively(path.substr(0, slashIndex));
			}

			// Create the last directory on the path (the recursive calls will have taken
			// care of the parent directories by now)
			bool result = CreateDirectory(path.c_str(), NULL);
			if(!result)
			{
				std::cout << "ERROR: Could not create directory!" << std::endl;
				exit(-1);
			}
		}
	}

	const char *getBaseDir(char *path)
	{
		std::string str(path);
		std::string result = str.substr(0, str.find_last_of("/\\"));
		return result.c_str();
	}

	std::string m_Result{};
	std::string m_Data;
	char *m_FileName;
	std::vector<std::string> m_ErrorList{};
};

#endif