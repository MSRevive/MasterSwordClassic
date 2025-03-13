// This program is free software; you can redistribute it and/or modify it
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE.
//

#include "stream_safe.h"
#include <string.h>

#include "texturestruct.h"
#include "tgaloader.h"
#include "textureloader.h"

void DbgLog(char *szFmt, ...);

namespace Tartan
{

	//! Loads sFilePath as a texture file into OpenGL, returns OpenGL texture id in iTextureID
	//!
	//! Loads sFilePath as a texture file into OpenGL, returns OpenGL texture id in iTextureID
	//! Returns true on success
	bool LoadTextureFile(const char *sFilePath, loadtex_t &LoadTex)
	{
		if (strlen(sFilePath) <= 4)
		{
			DbgLog("LoadTextureFile: %s has an invalid filename extension", sFilePath);
			//cout << "Tartan::LoadTextureFile() Error: File " << sFilePath << " has an invalid filename extension" << endl;
		}

		const char *filetypestring = sFilePath + strlen(sFilePath) - 4;

		DbgLog("LoadTextureFile - Filetype: %s", filetypestring);

#ifdef _WIN32
		if (stricmp(filetypestring, ".tga") == 0)
#else
		if (strcasecmp(filetypestring, ".tga") == 0)
#endif
		{
			return LoadTextureTGA(sFilePath, LoadTex);
		}
		else
		{
			std::cout << "Tartan::LoadTextureFile() Error: file type for file " << sFilePath << " unknown." << std::endl;
			return false;
		}
	}

	//! Input:
	//!
	//! Returns: True on success, false otherwise
	//!
	//! Description: Loads a TGA image into an OpenGL texture.
	//! This function based on NeHe texture loading tutorial (http://nehe.gamedev.net)
	//!
	//! Thread safety: Unknown
	//!

	bool LoadTextureTGA(const char *sFilepath, loadtex_t &LoadTex)
	{
		Texture Texture;

		//	DEBUG(  "Loading TGA file " << FilePath.GetFullPath().mb_str() << " ... " ); // DEBUG
		DbgLog("Enter LoadTextureTGA");

		int LoadTGAResult = LoadTGA(&Texture, sFilepath);
		//DEBUG(  "Check result of TGA loading..." ); // DEBUG
		if (!LoadTGAResult)
		{
			DbgLog("LoadTextureTGA - Couldn't find or load file: %s", sFilepath);
			return false;
		}
		//DEBUG(  "File found, loading..." ); // DEBUG

		LoadTex.Width = Texture.width;
		LoadTex.Height = Texture.height;

		DbgLog("Call GetCompatibleTextureSize");
		GetCompatibleTextureSize(Texture.width, Texture.height, LoadTex.GLWidth, LoadTex.GLHeight, LoadTex.CoordU, LoadTex.CoordV);

		// Typical Texture Generation Using Data From The TGA ( CHANGE )
		glGenTextures(1, &Texture.texID); // Create The Texture ( CHANGE )
		glBindTexture(GL_TEXTURE_2D, Texture.texID);

		GLint format;
		int BytesPerPixel = Texture.bpp / 8;
		//DEBUG(" *** DESIRED format : " << (Texture.bpp / 8) << " width: " << Texture.width << " height : " << Texture.height);
		DbgLog("Call glTexImage2D (first time)");
		glTexImage2D(GL_PROXY_TEXTURE_2D, 0, BytesPerPixel, LoadTex.GLWidth, LoadTex.GLHeight, 0, Texture.type, GL_UNSIGNED_BYTE, NULL);

		DbgLog("Call glGetTexLevelParameteriv (first time)");
		glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
		//DEBUG(" ***** Load texture: " << format);

		GLubyte *DataPixels = Texture.imageData;
		bool AdjustedTexture = false;

		DbgLog("Change Texture Size");
		//If the texture size was changed to be 2^x, then create a new buffer for the larger texture
		if (LoadTex.GLWidth != LoadTex.Width || LoadTex.GLHeight != LoadTex.Height)
		{
			DbgLog("LoadTex.Width: %u LoadTex.Height: %u", LoadTex.Width, LoadTex.Height);
			DbgLog("LoadTex.GLWidth: %u LoadTex.GLHeight: %u", LoadTex.GLWidth, LoadTex.GLHeight);

			AdjustedTexture = true;
			int OldSize = BytesPerPixel * LoadTex.Width * LoadTex.Height;
			int NewSize = BytesPerPixel * LoadTex.GLWidth * LoadTex.GLHeight;
			DbgLog("OldSize: %i NewSize: %i", OldSize, NewSize);

			DataPixels = new GLubyte[NewSize];
			int OldOfs = 0, NewOfs = 0;
			for (int i = 0; i < (signed)LoadTex.Height; i++)
			{
				int RowSize = BytesPerPixel * LoadTex.Width;
				memcpy(&DataPixels[NewOfs], &Texture.imageData[OldOfs], RowSize);
				OldOfs += RowSize;
				NewOfs += BytesPerPixel * LoadTex.GLWidth;
			}

			memset(&DataPixels[OldSize], 0, NewSize - OldSize);
		}

		DbgLog("Call glTexImage2D (2nd time)");
		glTexImage2D(GL_TEXTURE_2D, 0, BytesPerPixel, LoadTex.GLWidth, LoadTex.GLHeight, 0, Texture.type, GL_UNSIGNED_BYTE, DataPixels);

		DbgLog("Call glTexParameteri (4 times)");
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		GLenum err = glGetError();
		if (err != GL_NO_ERROR)
			DbgLog("glGetError() returned error code: %i", err);

		if (AdjustedTexture)
		{
			DbgLog("delete DataPixels");
			delete[] DataPixels;
		}

		if (Texture.imageData) // If Texture Image Exists ( CHANGE )
		{
			DbgLog("free(Texture.imageData)");
			free(Texture.imageData); // Free The Texture Image Memory ( CHANGE )
		}

		LoadTex.GLTexureID = Texture.texID;

		DbgLog("Exit LoadTextureTGA");
		return true;
	}

}
