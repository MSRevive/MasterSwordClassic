// This program is free software; you can redistribute it and/or modify it
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE.
//

typedef unsigned int uint;
struct loadtex_t
{
	uint GLTexureID;
	uint Width, Height;
	uint GLWidth, GLHeight; //the width and hieght, after conversion to 2^x
	float CoordU, CoordV;	//The texture coordinates, after conversion to 2^x
};

namespace Tartan
{
	bool LoadTextureFile(const char *sFilePath, loadtex_t &LoadTex); //!< Load texture file in sFilePath, returns OpenGL textureid in textureid
	bool LoadTextureTGA(const char *sFilepath, loadtex_t &LoadTex); //!< Load tga texture file in sFilePath, returns OpenGL textureid in textureid
}

void GetCompatibleTextureSize(uint SizeW, uint SizeH, uint &outNewSizeW, uint &outNewSizeH, float &outTexCoordU, float &outTexCoordV);
bool LoadGLTexture(const char *FileName, uint &TextureID);
bool LoadGLTexture(const char *FileName, loadtex_t &LoadTga);
