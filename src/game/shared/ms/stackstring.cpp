#include "Platform.h"
#include "stackstring.h"
#include "msdebug.h"

#undef msstring

msstring::msstring()
{
	data[0] = 0;
}
msstring::msstring(const msstring_ref a) { operator=(a); }
msstring::msstring(const msstring_ref a, size_t length)
{
	strncpy(data, a, length);
	data[length] = 0;
}
msstring::msstring(const msstring &a) { operator=(a); }
#ifdef _WIN32
msstring::msstring(const string_i &a) { operator=(a); }
#else
msstring::msstring(const string_i& a) { operator=(&a); }
#endif // _WIN32
msstring &msstring::operator=(const msstring_ref a)
{
	if (a == data)
		return *this;
	data[0] = 0;
	append(a);
	return *this;
}
msstring &msstring::operator=(int a)
{
	_snprintf(data, MSSTRING_SIZE, "%i", a);
	return *this;
}
msstring &msstring::operator=(const msstring &a) { return operator=(a.data); }
msstring &msstring::operator+=(const msstring_ref a)
{
	append(a);
	return *this;
}
msstring &msstring::operator+=(int a)
{
	msstring tmp;
	tmp = a;
	return operator+=((const msstring_ref &)(tmp));
}
msstring msstring::operator+(const msstring_ref a) { return msstring(data) += a; }
msstring msstring::operator+(const msstring &a) { return msstring(data) += a.data; }
msstring msstring::operator+(const string_i &a) { return msstring(data) += ((string_i)a).c_str(); }
msstring msstring::operator+(int a) { return msstring(data) += a; }
bool msstring::operator==(char *a) const { return !strcmp(data, a); }
bool msstring::operator==(const char *a) const { return !strcmp(data, a); }
msstring::operator char *() { return data; }
char *msstring::c_str() { return data; }
void msstring::append(const msstring_ref a, size_t length)
{
	size_t my_sz = len();
	size_t capped_sz = V_min(length, MSSTRING_MAXLEN - my_sz);
	if (capped_sz <= 0)
		return;
	strncpy(&data[my_sz], a, capped_sz);
	data[my_sz + capped_sz] = 0;
}
void msstring::append(const msstring_ref a)
{
	size_t len = strlen(a);
	append(a, len);
}
size_t msstring::len() const { return strlen(data); }
size_t msstring::find(const msstring_ref a, size_t start) const
{
	msstring_ref substring = strstr(&data[start], a);
	return substring ? (substring - &data[start]) : msstring_error;
}
msstring_ref msstring::find_str(const msstring_ref a, size_t start) const
{
	size_t ret = find(a, start);
	return (ret != msstring_error) ? &data[ret] : &data[start];
}
size_t msstring::findchar(const msstring_ref a, size_t start) const
{
	for (int i = start; i < (signed)len(); i++)
	{
		char datachar[2] = {data[i], 0};
		if (strstr(a, datachar))
			return i - start;
	}
	return msstring_error;
}
msstring_ref msstring::findchar_str(const msstring_ref a, size_t start) const
{
	size_t ret = findchar(a, start);
	return (ret != msstring_error) ? &data[ret] : &data[start];
}
bool msstring::contains(const msstring_ref a) const { return find(a) != msstring_error; }
bool msstring::starts_with(const msstring_ref a) const { return find(a) == 0; }
bool msstring::ends_with(const msstring_ref a) const
{
	msstring temp = a;
	int loc = len() - temp.len();
	return loc == find(temp);
}
msstring msstring::substr(size_t start, size_t length) { return msstring(&data[start], length); }
msstring msstring::substr(size_t start) { return msstring(&data[start]); }
msstring msstring::thru_substr(const msstring_ref a, size_t start) const
{
	size_t ret = find(a, start);
	return (ret != msstring_error) ? msstring(&data[start], ret) : msstring(&data[start]);
}
msstring msstring::thru_char(const msstring_ref a, size_t start) const
{
	size_t ret = findchar(a, start);
	return (ret != msstring_error) ? msstring(&data[start], ret) : msstring(&data[start]);
}
msstring msstring::skip(const msstring_ref a) const
{
	size_t my_sz = len();
	for (int i = 0; i < my_sz; i++)
	{
		char datachar[2] = {data[i], 0};
		if (!strstr(a, datachar))
			return msstring(&data[i], my_sz - i);
	}
	return &data[my_sz];
}
msstring msstring::tolower(void) const
{
	size_t my_sz = len();
	msstring ret;
	for (int i = 0; i < my_sz; i++)
	{
		char ch = data[i];
		ret += ::tolower(ch);
	}

	return ret;
}
//#define msstring str256

bool TokenizeString(const char *pszString, msstringlist &Tokens, msstring_ref Separator)
{
	char cTemp[MSSTRING_SIZE - 1];
	int i = 0;
	bool AnyFound = false;
	msstring ParseStr = "%[^";
	ParseStr += Separator;
	ParseStr += "]";
	while (sscanf(&pszString[i], ParseStr, cTemp) > 0)
	{
		i += strlen(cTemp);
		Tokens.add(cTemp);
		AnyFound = true;

		if (pszString[i])
			i++; //Hit a semi-colon, continue
	}
	return AnyFound;
}

void ReplaceChar(char *pString, char org, char dest)
{
	int i = 0;
	while (pString[i])
	{
		if (pString[i] == org)
			pString[i] = dest;
		i++;
	}
}

//
//	msvariant
//

msvariant::msvariant() { clrmem(*this); }

void msvariant::SetFromString(msstring_ref a)
{
	m_String = a;
	m_Int = atoi(a);
	m_Float = (float)atof(a);
}

void msvariant::SetFromInt(int a)
{
	m_String = "";
	m_String += a;
	m_Int = a;
	m_Float = (float)a;
}

void msvariant::SetFromFloat(float a)
{
	_snprintf(m_String.c_str(), MSSTRING_SIZE, "%f", a);
	m_Int = (int)a;
	m_Float = a;
}

mslist<std::string> strutil::explode(std::string const &str, char delim)
{
	mslist<std::string> result;
	std::istringstream iss(str);

	for (std::string token; std::getline(iss, token, delim); )
	{	
		if (!token.empty())
			result.push_back(std::move(token));
	}

	return result;
}

std::string& strutil::implode(mslist<std::string> vec, int start)
{
	static std::string result;
	result.clear();

	for (int i = start; i < vec.size(); i++)
		result += vec[i];

	return result;
}

// std::string& strutil::removeWhiteSpace(std::string &str)
// {
// 	str.erase(std::unique(std::begin(str), std::end(str), [](unsigned char a, unsigned char b){
// 		return isSpace(a) && isSpace(b);
// 	}), std::end(str));

// 	return str;
// }

bool strutil::isSpace(const char &ch)
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

void strutil::tolower(char* str)
{
	char* target = str;
	while (*target != '\0')
	{
		(*target) = ::tolower(*target);
		target++;
	}
}

bool strutil::isBadChar(int c)
{
	return (c == '(' || c == ')' || c == '$' || c == '¯');
}

char* strutil::stripBadChars(char* data)
{
	int i = 0, x = 0;
	char c;
	char* cleanData = data;

	while ((c = data[i++]) != '\0')
	{
		if (!isBadChar(c))
			cleanData[x++] = c;
	}

	cleanData[x] = '\0';
	return cleanData;
}

bool strutil::isBadStr(char* str)
{
	char* target = str;
	while (*target != '\0')
	{
		if (isBadChar(*target))
			return true;
		target++;
	}
	return false;
}
