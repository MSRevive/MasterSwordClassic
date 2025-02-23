#ifndef STACKSTRING_H
#define STACKSTRING_H

#ifndef _WIN32
extern "C" char *strlwr(char *str);
#endif

#include <cstring>
#include <string.h>
#include <string>
#include <sstream>
//#include <algorithm>

#ifndef STD_MAP
#define STD_MAP
#include <map>
#endif

#ifndef STD_LIST
#define STD_LIST
#include <list>
#endif

#ifndef STD_SET
#define STD_SET
#include <set>
#endif

#include "strhelper.h"

#define clrmem(a) memset(&a, 0, sizeof(a));

//Deuplicated from msdebug.h
#ifdef DEV_BUILD
void *operator new(size_t size, const char *pszSourceFile, int LineNum);
void operator delete(void *ptr, const char *pszSourceFile, int LineNum);
#define msnew new (__FILE__, __LINE__)
#else
#define msnew new
#endif

//mslist - List of items of type itemtype_y
//-Dogg
template <class itemtype_y>
class mslist
{
private:
	itemtype_y *m_First;
	size_t m_Items;
	size_t m_ItemsAllocated;

public:
	mslist()
	{
		m_Items = 0;
		m_ItemsAllocated = 0;
		m_First = (itemtype_y *)0;
	}
	mslist(bool NoInit) {} //Special case - no initialization
	~mslist()
	{
		clear();
	}
	inline itemtype_y &push_back(const itemtype_y &Item) { return add(Item); }
	itemtype_y &add(const itemtype_y &Item)
	{
		reserve(m_Items + 1);

		int idx = m_Items;
		m_First[idx] = Item;

		m_Items++;

		return m_First[idx];
	}
	itemtype_y &add_blank() //Add an entry that's completely zero'd out
	{
		reserve(m_Items + 1);

		int idx = m_Items;
		memset(&m_First[idx], 0, sizeof(itemtype_y));

		m_Items++;

		return m_First[idx];
	}
	itemtype_y &operator[](const int idx) const
	{
		return m_First[idx];
	}
	void erase(const size_t idx)
	{
		if (idx + 1 < m_Items)
			memmove(&m_First[idx], &m_First[idx + 1], (m_Items - (idx + 1)) * sizeof(itemtype_y));

		m_Items--;
	}
	void clear()
	{
		unalloc();
		m_Items = m_ItemsAllocated = 0;
		m_First = (itemtype_y *)0;
	}
	void clearitems() //No slow dealloc.  Just empty out the items
	{
		m_Items = 0;
	}
	size_t size() const
	{
		return m_Items;
	}
	void reserve()
	{
		m_ItemsAllocated++;

		itemtype_y *pNewItems = ::msnew itemtype_y[m_ItemsAllocated];
		for (unsigned int i = 0; i < m_Items; i++)
			pNewItems[i] = m_First[i];
		unalloc();
		m_First = pNewItems;
	}
	void reserve(size_t Items)
	{
		while (m_ItemsAllocated < Items)
			reserve();
	}
	void reserve_once(size_t ReserveItems, size_t Items) //Special case - only use if you know what you're doing
	{
		unalloc();
		m_ItemsAllocated = ReserveItems;
		m_First = msnew itemtype_y[m_ItemsAllocated];
		m_Items = Items;
	}
	mslist &operator=(const mslist &OtherList)
	{
		if (size() != OtherList.size())
		{
			clear();
			for (unsigned int i = 0; i < OtherList.size(); i++)
				add(OtherList[i]);
		}
		else //Don't re-allocate if I don't have to
			for (unsigned int i = 0; i < OtherList.size(); i++)
				m_First[i] = OtherList[i];

		return *this;
	}
private:
	void unalloc()
	{
		if (m_First)
			delete[] m_First;
		m_First = (itemtype_y*)0;
	}
};

//mstring - A fast string with features... no dynamic allocation
//-Dogg
#define MSSTRING_SIZE 256
#define MSSTRING_MAXLEN (MSSTRING_SIZE-1)
#define msstring_error ((size_t)-1)

typedef const char *msstring_ref;
struct string_i;

class msstring
{
public:
	msstring();
	msstring(const msstring_ref a);
	msstring(const msstring_ref a, size_t length);
	msstring(const msstring &a);
	msstring(const string_i &a);
	msstring &operator=(const msstring_ref a);
	msstring &operator=(int a);
	msstring &operator=(const msstring &a);
	msstring &operator+=(const msstring_ref a);
	msstring &operator+=(int a);
	msstring operator+(const msstring_ref a);
	msstring operator+(const msstring &a);
	msstring operator+(const string_i &a);
	msstring operator+(int a);
	bool operator==(char *a) const;
	bool operator==(const char *a) const;
	bool operator!=(char *a) const { return !operator==(a); }
	bool operator!=(const char *a) const { return !operator==(a); }
	bool operator<(char *a) const { return strcmp(data,a) < 0; }
	bool operator<(const char *a) const { return strcmp(data,a) < 0; }
	bool operator<(msstring& a) const { return strcmp(data,a.data) < 0; }
	bool operator<(const msstring& a) const { return strcmp(data,a.data) < 0; }
	operator char *();
	operator void *() { return operator char *(); }
	char *c_str();
	size_t len() const;
	void append(const msstring_ref a);
	void append(const msstring_ref a, size_t length);
	size_t find(const msstring_ref a, size_t start = 0) const;				 //Returns position of the string "a"
	msstring_ref find_str(const msstring_ref a, size_t start = 0) const;	 //Returns a substring starting at "find(a,start)". Returns full string if "a" not found
	size_t findchar(const msstring_ref a, size_t start = 0) const;			 //Returns position of the first char within "a"
	msstring_ref findchar_str(const msstring_ref a, size_t start = 0) const; //Returns a substring starting at "findchar(a,start)". Returns full string if text didn't contain any chars from "a"
	bool contains(const msstring_ref a) const;								 //Reutrns true if substring "a" is contained within the main string
	bool starts_with(const msstring_ref a) const;							 //Reutrns true if the main string starts with "a"
	bool ends_with(const msstring_ref a) const;								 //MIB FEB2008a returns true if last character is "a"
	msstring substr(size_t start, size_t length);
	msstring substr(size_t start);
	msstring thru_substr(const msstring_ref a, size_t start = 0) const; //Returns a substring spanning from "start" to "find(a,start)". Returns full string if "find(a,start)" not found
	msstring thru_char(const msstring_ref a, size_t start = 0) const;	//Returns a substring spanning from "start" to "findchar(a,start)". Returns full string if "findchar(a,start)" not found
	msstring skip(const msstring_ref a) const;							//Returns a substring starting at the first char that isn't within "a"
	msstring tolower(void) const;

protected:
	char data[MSSTRING_SIZE];
};

typedef mslist<msstring> msstringlist;

bool TokenizeString(msstring_ref pszString, msstringlist &Tokens, msstring_ref Separator);
inline bool TokenizeString(msstring_ref pszString, msstringlist &Tokens) { return TokenizeString(pszString, Tokens, ";"); }
void ReplaceChar(char *pString, char org, char dest);

//It's an int, a float, and a string
class msvariant
{
public:
	msstring m_String;
	int m_Int;
	float m_Float;
	enum type
	{
		INT,
		FLOAT,
		STRING
	} m_Type;

	msvariant();
	msvariant(int a) { operator=(a); }
	msvariant(float a) { operator=(a); }
	msvariant(msstring_ref a) { operator=(a); }
	msvariant &operator=(msstring_ref a)
	{
		SetFromString(a);
		m_Type = STRING;
		return *this;
	}
	msvariant &operator=(int a)
	{
		SetFromInt(a);
		m_Type = INT;
		return *this;
	}
	msvariant &operator=(float a)
	{
		SetFromFloat(a);
		m_Type = STRING;
		return *this;
	}

	void SetFromString(msstring_ref a);
	void SetFromInt(int a);
	void SetFromFloat(float a);

	operator int() { return m_Int; }
	operator float() { return m_Float; }
	operator msstring_ref() { return m_String.c_str(); }
};

typedef std::map<msstring,msstring> msstringstringhash;

namespace strutil {
	bool isSpace(const char &ch);
	bool isBadChar(int c);
	char* stripBadChars(char *data);
}

#endif // STACKSTRING_H
