//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:
//
// $Workfile:     $
// $Date: 2004/09/08 03:20:54 $
//
//-----------------------------------------------------------------------------
// $Log: vgui_SchemeManager.cpp,v $
// Revision 1.2  2004/09/08 03:20:54  dogg
// no message
//
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"
#include "vgui_schememanager.h"
#include "cvardef.h"
#include "filesystem_shared.h"

#include <string.h>
#include <vector>

cvar_t *g_CV_BitmapFonts;

void Scheme_Init()
{
	g_CV_BitmapFonts = gEngfuncs.pfnRegisterVariable("bitmapfonts", "1", 0);
}

//-----------------------------------------------------------------------------
// Purpose: Scheme managers data container
//-----------------------------------------------------------------------------
class CSchemeManager::CScheme
{
public:
	enum
	{
		SCHEME_NAME_LENGTH = 32,
		FONT_NAME_LENGTH = 48,
		FONT_FILENAME_LENGTH = 64,
	};

	// name
	char schemeName[SCHEME_NAME_LENGTH];

	// font
	char fontName[FONT_NAME_LENGTH];

	int fontSize;
	int fontWeight;

	vgui::Font *font;
	int ownFontPointer; // true if the font is ours to delete

	// scheme
	byte fgColor[4];
	byte bgColor[4];
	byte armedFgColor[4];
	byte armedBgColor[4];
	byte mousedownFgColor[4];
	byte mousedownBgColor[4];
	byte borderColor[4];

	// MIB FEB2019_26 [LOCAL_PANEL_FONT]
	bool bItalic;
	bool bUnderline;
	bool bStrike;

	// construction/destruction
	CScheme();
	~CScheme();
};

CSchemeManager::CScheme::CScheme()
{
	schemeName[0] = 0;
	fontName[0] = 0;
	fontSize = 0;
	fontWeight = 0;
	font = NULL;
	ownFontPointer = false;

	// MIB FEB2019_26 [LOCAL_PANEL_FONT]
	bItalic = false;
	bUnderline = false;
	bStrike = false;
}

CSchemeManager::CScheme::~CScheme()
{
	// only delete our font pointer if we own it
	if (ownFontPointer)
	{
		delete font;
	}
}

//-----------------------------------------------------------------------------
// Purpose: resolution information
//			!! needs to be shared out
//-----------------------------------------------------------------------------
static int g_ResArray[] =
{
	640,
	800,
	720,
	1024,
	1152,
	1176,
	1280,
	1360,
	1366,
	1440,
	1600,
	1680,
	1768,
	1920
};
static int g_NumReses = sizeof(g_ResArray);

static std::vector<byte> LoadFileByResolution(const char *filePrefix, int xRes, const char *filePostfix)
{
	// find our resolution in the res array
	int resNum = ARRAYSIZE(g_ResArray) - 1;
	while (g_ResArray[resNum] > xRes)
	{
		resNum--;

		if (resNum < 0)
			return {};
	}

	for (; resNum >= 0; --resNum)
	{
		// try load
		char fname[256];
		sprintf(fname, "%s%d%s", filePrefix, g_ResArray[resNum], filePostfix);
		auto fileContents = FileSystem_LoadFileIntoBuffer(fname, FileContentFormat::Text);

		if (!fileContents.empty())
			return fileContents;
	}

	return {};
}

static void ParseRGBAFromString(byte colorArray[4], const char *colorVector)
{
	int r, g, b, a;
	sscanf(colorVector, "%d %d %d %d", &r, &g, &b, &a);
	colorArray[0] = r;
	colorArray[1] = g;
	colorArray[2] = b;
	colorArray[3] = a;
}

//-----------------------------------------------------------------------------
// Purpose: initializes the scheme manager
//			loading the scheme files for the current resolution
// Input  : xRes -
//			yRes - dimensions of output window
//-----------------------------------------------------------------------------
CSchemeManager::CSchemeManager(int xRes, int yRes)
{
	// basic setup
	m_pSchemeList = NULL;
	m_iNumSchemes = 0;

	// find the closest matching scheme file to our resolution
	char token[1024];
	const auto fileContents = LoadFileByResolution("", xRes, "_textscheme.txt");
	m_xRes = xRes;

	char* pFileStart = (char*)fileContents.data();
	char* pFile = pFileStart;

	char fontFilename[512];

	//
	// Read the scheme descriptions from the text file, into a temporary array
	// format is simply:
	// <paramName name> = <paramValue>
	//
	// a <paramName name> of "SchemeName" signals a new scheme is being described
	//

	const static int numTmpSchemes = 192;
	static CScheme tmpSchemes[numTmpSchemes];
	memset(tmpSchemes, 0, sizeof(tmpSchemes));
	int currentScheme = -1;
	CScheme *pScheme = nullptr;
	// record what has been entered so we can create defaults from the different values
	bool hasFgColor, hasBgColor, hasArmedFgColor, hasArmedBgColor, hasMouseDownFgColor, hasMouseDownBgColor = false;

	if (fileContents.empty())
	{
		gEngfuncs.Con_DPrintf("Unable to find *_textscheme.txt\n");
		goto buildDefaultFont;
	}

	pFile = gEngfuncs.COM_ParseFile(pFile, token);
	while (strlen(token) > 0 && (currentScheme < numTmpSchemes))
	{
		// get the paramName name
		static const int tokenSize = 64;
		char paramName[tokenSize], paramValue[tokenSize];

		strncpy(paramName, token, tokenSize);
		paramName[tokenSize - 1] = 0; // ensure null termination

		// get the '=' character
		pFile = gEngfuncs.COM_ParseFile(pFile, token);
		if (_stricmp(token, "="))
		{
			if (currentScheme < 0)
			{
				gEngfuncs.Con_Printf("error parsing font scheme text file at file start - expected '=', found '%s''\n", token);
			}
			else
			{
				gEngfuncs.Con_Printf("error parsing font scheme text file at scheme '%s' - expected '=', found '%s''\n", tmpSchemes[currentScheme].schemeName, token);
			}
			break;
		}

		// get paramValue
		pFile = gEngfuncs.COM_ParseFile(pFile, token);
		strncpy(paramValue, token, tokenSize);
		paramValue[tokenSize - 1] = 0; // ensure null termination

		// is this a new scheme?
		if (!_stricmp(paramName, "SchemeName"))
		{
			// setup the defaults for the current scheme
			if (pScheme)
			{
				// foreground color defaults (normal -> armed -> mouse down)
				if (!hasFgColor)
				{
					pScheme->fgColor[0] = pScheme->fgColor[1] = pScheme->fgColor[2] = pScheme->fgColor[3] = 255;
				}
				if (!hasArmedFgColor)
				{
					memcpy(pScheme->armedFgColor, pScheme->fgColor, sizeof(pScheme->armedFgColor));
				}
				if (!hasMouseDownFgColor)
				{
					memcpy(pScheme->mousedownFgColor, pScheme->armedFgColor, sizeof(pScheme->mousedownFgColor));
				}

				// background color (normal -> armed -> mouse down)
				if (!hasBgColor)
				{
					pScheme->bgColor[0] = pScheme->bgColor[1] = pScheme->bgColor[2] = pScheme->bgColor[3] = 0;
				}
				if (!hasArmedBgColor)
				{
					memcpy(pScheme->armedBgColor, pScheme->bgColor, sizeof(pScheme->armedBgColor));
				}
				if (!hasMouseDownBgColor)
				{
					memcpy(pScheme->mousedownBgColor, pScheme->armedBgColor, sizeof(pScheme->mousedownBgColor));
				}

				// font size
				if (!pScheme->fontSize)
				{
					pScheme->fontSize = 17;
				}
				if (!pScheme->fontName[0])
				{
					strncpy(pScheme->fontName,  "Courier", sizeof(pScheme->fontName) );
				}
			}

			// create the new scheme
			currentScheme++;
			pScheme = &tmpSchemes[currentScheme];
			hasFgColor = hasBgColor = hasArmedFgColor = hasArmedBgColor = hasMouseDownFgColor = hasMouseDownBgColor = false;

			strncpy(pScheme->schemeName, paramValue, CScheme::SCHEME_NAME_LENGTH);
			pScheme->schemeName[CScheme::SCHEME_NAME_LENGTH - 1] = '\0'; // ensure null termination of string
		}

		if (!pScheme)
		{
			gEngfuncs.Con_Printf("font scheme text file MUST start with a 'SchemeName'\n");
			break;
		}

		// pull the data out into the scheme
		if (!_stricmp(paramName, "FontName"))
		{
			strncpy(pScheme->fontName, paramValue, CScheme::FONT_NAME_LENGTH);
			pScheme->fontName[CScheme::FONT_NAME_LENGTH - 1] = 0;
		}
		else if (!_stricmp(paramName, "FontSize"))
		{
			pScheme->fontSize = atoi(paramValue);
		}
		else if (!_stricmp(paramName, "FontWeight"))
		{
			pScheme->fontWeight = atoi(paramValue);
		}
		else if (!_stricmp(paramName, "FgColor"))
		{
			ParseRGBAFromString(pScheme->fgColor, paramValue);
			hasFgColor = true;
		}
		else if (!_stricmp(paramName, "BgColor"))
		{
			ParseRGBAFromString(pScheme->bgColor, paramValue);
			hasBgColor = true;
		}
		else if (!_stricmp(paramName, "FgColorArmed"))
		{
			ParseRGBAFromString(pScheme->armedFgColor, paramValue);
			hasArmedFgColor = true;
		}
		else if (!_stricmp(paramName, "BgColorArmed"))
		{
			ParseRGBAFromString(pScheme->armedBgColor, paramValue);
			hasArmedBgColor = true;
		}
		else if (!_stricmp(paramName, "FgColorMousedown"))
		{
			ParseRGBAFromString(pScheme->mousedownFgColor, paramValue);
			hasMouseDownFgColor = true;
		}
		else if (!_stricmp(paramName, "BgColorMousedown"))
		{
			ParseRGBAFromString(pScheme->mousedownBgColor, paramValue);
			hasMouseDownBgColor = true;
		}
		else if (!_stricmp(paramName, "BorderColor"))
		{
			ParseRGBAFromString(pScheme->borderColor, paramValue);
			hasMouseDownBgColor = true;
		}
		// MIB FEB2019_26 [LOCAL_PANEL_FONT]
		else if (!_stricmp(paramName, "Italic"))
			pScheme->bItalic = atoi(paramValue) >= 1;
		else if (!_stricmp(paramName, "Underline"))
			pScheme->bUnderline = atoi(paramValue) >= 1;
		else if (!_stricmp(paramName, "StrikeThrough"))
			pScheme->bStrike = atoi(paramValue) >= 1;

		// get the new token last, so we now if the loop needs to be continued or not
		pFile = gEngfuncs.COM_ParseFile(pFile, token);
	}

buildDefaultFont:

	// make sure we have at least 1 valid font
	if (currentScheme < 0)
	{
		currentScheme = 0;
		strncpy(tmpSchemes[0].schemeName, "Default Scheme", sizeof(tmpSchemes[0].schemeName));
		strncpy(tmpSchemes[0].fontName, "Arial", sizeof(tmpSchemes[0].fontName));
		tmpSchemes[0].fontSize = 0;
		tmpSchemes[0].fgColor[0] = tmpSchemes[0].fgColor[1] = tmpSchemes[0].fgColor[2] = tmpSchemes[0].fgColor[3] = 255;
		tmpSchemes[0].armedFgColor[0] = tmpSchemes[0].armedFgColor[1] = tmpSchemes[0].armedFgColor[2] = tmpSchemes[0].armedFgColor[3] = 255;
		tmpSchemes[0].mousedownFgColor[0] = tmpSchemes[0].mousedownFgColor[1] = tmpSchemes[0].mousedownFgColor[2] = tmpSchemes[0].mousedownFgColor[3] = 255;
	}

	// we have the full list of schemes in the tmpSchemes array
	// now allocate the correct sized list
	m_iNumSchemes = currentScheme + 1; // 0-based index
	m_pSchemeList = new CScheme[m_iNumSchemes];

	// copy in the data
	memcpy(m_pSchemeList, tmpSchemes, sizeof(CScheme) * m_iNumSchemes);

	// create the fonts
	for (int i = 0; i < m_iNumSchemes; i++)
	{
		m_pSchemeList[i].font = NULL;

		// see if the current font values exist in a previously loaded font
		for (int j = 0; j < i; j++)
		{
			// check if the font name, size, and weight are the same
			if (
				!_stricmp(m_pSchemeList[i].fontName, m_pSchemeList[j].fontName)
				&& m_pSchemeList[i].fontSize == m_pSchemeList[j].fontSize
				&& m_pSchemeList[i].fontWeight == m_pSchemeList[j].fontWeight
				&& m_pSchemeList[i].bItalic == m_pSchemeList[j].bItalic // MIB FEB2019_26 [LOCAL_PANEL_FONT]
				&& m_pSchemeList[i].bUnderline == m_pSchemeList[j].bUnderline
				&& m_pSchemeList[i].bStrike == m_pSchemeList[j].bStrike
				)
			{
				// copy the pointer, but mark i as not owning it
				m_pSchemeList[i].font = m_pSchemeList[j].font;
				m_pSchemeList[i].ownFontPointer = false;
			}
		}

		// if we haven't found the font already, load it ourselves
		if (!m_pSchemeList[i].font)
		{
			int fontFileLength = -1;
			byte *pFontData = nullptr;
			std::vector<byte> fontFileContents;

			if (g_CV_BitmapFonts && g_CV_BitmapFonts->value)
			{
				//custom because msc has weird font sizes
				int fontRes = 640;
				if (m_xRes >= 1600)
					fontRes = 1440;
				else if (m_xRes >= 1280)
					fontRes = 1400;
				else if (m_xRes >= 800)
					fontRes = 960;

				// if (m_xRes >= 1600)
				// 	fontRes = 1600;
				// else if (m_xRes >= 1280)
				// 	fontRes = 1280;
				// else if (m_xRes >= 1152)
				// 	fontRes = 1152;
				// else if (m_xRes >= 1024)
				// 	fontRes = 1024;
				// else if (m_xRes >= 800)
				// 	fontRes = 800;

				_snprintf(fontFilename, sizeof(fontFilename), "gfx\\vgui\\fonts\\%d_%s.tga", fontRes, m_pSchemeList[i].schemeName);
				fontFileContents = FileSystem_LoadFileIntoBuffer(fontFilename, FileContentFormat::Binary);
				pFontData = reinterpret_cast<byte*>(fontFileContents.data());
				fontFileLength = static_cast<int>(fontFileContents.size());
				if (fontFileContents.empty())
					gEngfuncs.Con_Printf("Missing bitmap font: %s\n", fontFilename);
			}

			m_pSchemeList[i].font = new vgui::Font(
				m_pSchemeList[i].fontName,
				pFontData,
				fontFileLength,
				m_pSchemeList[i].fontSize,
				0,
				0,
				m_pSchemeList[i].fontWeight,
				m_pSchemeList[i].bItalic, // MIB FEB2019_26 [LOCAL_PANEL_FONT]
				m_pSchemeList[i].bUnderline,
				m_pSchemeList[i].bStrike,
				false);
			
			m_pSchemeList[i].ownFontPointer = true;
		}

		// fix up alpha values; VGUI uses 1-A (A=0 being solid, A=255 transparent)
		m_pSchemeList[i].fgColor[3] = 255 - m_pSchemeList[i].fgColor[3];
		m_pSchemeList[i].bgColor[3] = 255 - m_pSchemeList[i].bgColor[3];
		m_pSchemeList[i].armedFgColor[3] = 255 - m_pSchemeList[i].armedFgColor[3];
		m_pSchemeList[i].armedBgColor[3] = 255 - m_pSchemeList[i].armedBgColor[3];
		m_pSchemeList[i].mousedownFgColor[3] = 255 - m_pSchemeList[i].mousedownFgColor[3];
		m_pSchemeList[i].mousedownBgColor[3] = 255 - m_pSchemeList[i].mousedownBgColor[3];
	}
}

//-----------------------------------------------------------------------------
// Purpose: frees all the memory used by the scheme manager
//-----------------------------------------------------------------------------
CSchemeManager::~CSchemeManager()
{
	delete[] m_pSchemeList;
	m_iNumSchemes = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Finds a scheme in the list, by name
// Input  : char *schemeName - string name of the scheme
// Output : SchemeHandle_t handle to the scheme
//-----------------------------------------------------------------------------
SchemeHandle_t CSchemeManager::getSchemeHandle(const char *schemeName)
{
	// iterate through the list
	for (int i = 0; i < m_iNumSchemes; i++)
	{
		if (!_stricmp(schemeName, m_pSchemeList[i].schemeName))
			return i;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: always returns a valid scheme handle
// Input  : schemeHandle -
// Output : CScheme
//-----------------------------------------------------------------------------
CSchemeManager::CScheme *CSchemeManager::getSafeScheme(SchemeHandle_t schemeHandle)
{
	if (schemeHandle < m_iNumSchemes)
		return m_pSchemeList + schemeHandle;

	return m_pSchemeList;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the schemes pointer to a font
// Input  : schemeHandle -
// Output : vgui::Font
//-----------------------------------------------------------------------------
vgui::Font *CSchemeManager::getFont(SchemeHandle_t schemeHandle)
{
	return getSafeScheme(schemeHandle)->font;
}

void CSchemeManager::getFgColor(SchemeHandle_t schemeHandle, int &r, int &g, int &b, int &a)
{
	CScheme *pScheme = getSafeScheme(schemeHandle);
	r = pScheme->fgColor[0];
	g = pScheme->fgColor[1];
	b = pScheme->fgColor[2];
	a = pScheme->fgColor[3];
}

void CSchemeManager::getBgColor(SchemeHandle_t schemeHandle, int &r, int &g, int &b, int &a)
{
	CScheme *pScheme = getSafeScheme(schemeHandle);
	r = pScheme->bgColor[0];
	g = pScheme->bgColor[1];
	b = pScheme->bgColor[2];
	a = pScheme->bgColor[3];
}

void CSchemeManager::getFgArmedColor(SchemeHandle_t schemeHandle, int &r, int &g, int &b, int &a)
{
	CScheme *pScheme = getSafeScheme(schemeHandle);
	r = pScheme->armedFgColor[0];
	g = pScheme->armedFgColor[1];
	b = pScheme->armedFgColor[2];
	a = pScheme->armedFgColor[3];
}

void CSchemeManager::getBgArmedColor(SchemeHandle_t schemeHandle, int &r, int &g, int &b, int &a)
{
	CScheme *pScheme = getSafeScheme(schemeHandle);
	r = pScheme->armedBgColor[0];
	g = pScheme->armedBgColor[1];
	b = pScheme->armedBgColor[2];
	a = pScheme->armedBgColor[3];
}

void CSchemeManager::getFgMousedownColor(SchemeHandle_t schemeHandle, int &r, int &g, int &b, int &a)
{
	CScheme *pScheme = getSafeScheme(schemeHandle);
	r = pScheme->mousedownFgColor[0];
	g = pScheme->mousedownFgColor[1];
	b = pScheme->mousedownFgColor[2];
	a = pScheme->mousedownFgColor[3];
}

void CSchemeManager::getBgMousedownColor(SchemeHandle_t schemeHandle, int &r, int &g, int &b, int &a)
{
	CScheme *pScheme = getSafeScheme(schemeHandle);
	r = pScheme->mousedownBgColor[0];
	g = pScheme->mousedownBgColor[1];
	b = pScheme->mousedownBgColor[2];
	a = pScheme->mousedownBgColor[3];
}

void CSchemeManager::getBorderColor(SchemeHandle_t schemeHandle, int &r, int &g, int &b, int &a)
{
	CScheme *pScheme = getSafeScheme(schemeHandle);
	r = pScheme->borderColor[0];
	g = pScheme->borderColor[1];
	b = pScheme->borderColor[2];
	a = pScheme->borderColor[3];
}
