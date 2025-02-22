/********************************************************************************
/Name:		TGA.cpp																*
/Header:	tga.h																*
/Purpose:	Load Compressed and Uncompressed TGA files							*
/Functions:	LoadTGA(Texture * texture, char * filename)							*
/			LoadCompressedTGA(Texture * texture, char * filename, FILE * fTGA)	*
/			LoadUncompressedTGA(Texture * texture, char * filename, FILE * fTGA)*	
/*******************************************************************************/

//#include "stream_safe.h"
//using namespace std;

#include "tgastruct.h"

#include <GL/gl.h>	  // Header File For The OpenGL32 Library

#include "msfileio.h"
#include "textureloader.h"

using namespace Tartan;

namespace Tartan
{

	/********************************************************************************
/name :		LoadTGA(Texture * texture, char * filename)							*
/function:  Open and test the file to make sure it is a valid TGA file			*	
/parems:	texture, pointer to a Texture structure								*
/			filename, string pointing to file to open							*
/********************************************************************************/

	CMemFile File;

	bool LoadTGA(Texture *texture, const char *filename) // Load a TGA file
	{
		//FILE * fTGA;												// File pointer to texture file
		//fTGA = fopen(filename, "rb");								// Open file for reading

		if (!File.ReadFromGameFile(filename)) // If it didn't open....
			return false;					  // Exit function

		if (!File.Read(&tgaheader, sizeof(TGAHeader))) // Attempt to read 12 byte header from file
		{
			File.Close(); // If it is, close it
			return false; // Exit function
		}

		if (tgaheader.imagetype == uTGAcompare[2] /*memcmp(uTGAcompare, &tgaheader, sizeof(tgaheader)) == 0*/) // See if header matches the predefined header of
		{																									   // an Uncompressed TGA image
			return LoadUncompressedTGA(texture);															   // If so, jump to Uncompressed TGA loading code
		}
		else if (tgaheader.imagetype == cTGAcompare[2] /*memcmp(cTGAcompare, &tgaheader, sizeof(tgaheader)) == 0*/) // See if header matches the predefined header of
		{																											// an RLE compressed TGA image
			return LoadCompressedTGA(texture);																		// If so, jump to Compressed TGA loading code
		}
		else // If header matches neither type
		{
			File.Close();
			return false; // Exit function
		}
		return true; // All went well, continue on
	}

	bool LoadUncompressedTGA(Texture *texture) // Load an uncompressed TGA (note, much of this code is based on NeHe's
	{										   // TGA Loading code nehe.gamedev.net)
		//if(fread(tga.header, sizeof(tga.header), 1, fTGA) == 0)					// Read TGA header
		if (!File.Read(tga.header, sizeof(tga.header)))
		{
			File.Close();
			return false; // Return failular
		}

		texture->width = tga.header[1] * 256 + tga.header[0];  // Determine The TGA Width	(highbyte*256+lowbyte)
		texture->height = tga.header[3] * 256 + tga.header[2]; // Determine The TGA Height	(highbyte*256+lowbyte)
		texture->bpp = tga.header[4];						   // Determine the bits per pixel
		tga.Width = texture->width;							   // Copy width into local structure
		tga.Height = texture->height;						   // Copy height into local structure
		tga.Bpp = texture->bpp;								   // Copy BPP into local structure

		if ((texture->width <= 0) || (texture->height <= 0) || ((texture->bpp != 24) && (texture->bpp != 32))) // Make sure all information is valid
		{
			File.Close();
			return false; // Return failed
		}

		if (texture->bpp == 24)		 // If the BPP of the image is 24...
			texture->type = GL_RGB;	 // Set Image type to GL_RGB
		else						 // Else if its 32 BPP
			texture->type = GL_RGBA; // Set image type to GL_RGBA

		tga.bytesPerPixel = (tga.Bpp / 8);							  // Compute the number of BYTES per pixel
		tga.imageSize = (tga.bytesPerPixel * tga.Width * tga.Height); // Compute the total amout ofmemory needed to store data
		texture->imageData = new GLubyte[tga.imageSize]; // Allocate that much memory

		if (texture->imageData == NULL) // If no space was allocated
		{
			File.Close();
			return false; // Return failed
		}

		//Dogg - Read Identity Data
		//So far only Paint Shop Pro 8+ uses this... but that one byte PSP uses must be read here.
		GLubyte IDData[256];
		if (tgaheader.identsize)
		{
#ifdef min
			if (!File.Read(IDData, min((int)tgaheader.identsize, 256)))
#else
			if (!File.Read(IDData, std::min((int)tgaheader.identsize, 256)))
#endif
			{
				File.Close();
				return false;
			}
		}

		//if(fread(texture->imageData, 1, tga.imageSize, fTGA) != tga.imageSize)	// Attempt to read image data
		if (!File.Read(texture->imageData, tga.imageSize))
		{
			if (texture->imageData != NULL) // If imagedata has data in it
				free(texture->imageData);	// Delete data from memory

			File.Close();
			return false; // Return failed
		}

		// Byte Swapping Optimized By Steve Thomas
		for (GLuint cswap = 0; cswap < (int)tga.imageSize; cswap += tga.bytesPerPixel)
		{
			texture->imageData[cswap] ^= texture->imageData[cswap + 2] ^=
				texture->imageData[cswap] ^= texture->imageData[cswap + 2];
		}

		// Dogg - PSP8 saves with top-to-bottom row ordering, so swap the rows vertically if the flag is set
		if (tga.header[5] & (1 << 5))
		{
			GLuint RowSize = tga.Width * tga.bytesPerPixel;
			GLubyte *NewData = new GLubyte[tga.imageSize];
			if (!NewData)
			{
				File.Close();
				return false; // Return failed
			}

			for (GLuint row = 0; row < (int)tga.Height; row++)
			{
				int NewRowStart = row * RowSize;
				int OldRowStart = ((tga.Height - row) - 1) * RowSize;
				memcpy(&NewData[NewRowStart], &texture->imageData[OldRowStart], RowSize);
			}
			delete[] texture->imageData;
			texture->imageData = NewData;
		}

		File.Close();
		return true; // Return success
	}

	bool LoadCompressedTGA(Texture *texture) // Load COMPRESSED TGAs
	{
		if (!File.Read(tga.header, sizeof(tga.header)))
		{
			File.Close(); // Close it
			return false; // Return failed
		}

		texture->width = tga.header[1] * 256 + tga.header[0];  // Determine The TGA Width	(highbyte*256+lowbyte)
		texture->height = tga.header[3] * 256 + tga.header[2]; // Determine The TGA Height	(highbyte*256+lowbyte)
		texture->bpp = tga.header[4];						   // Determine Bits Per Pixel
		tga.Width = texture->width;							   // Copy width to local structure
		tga.Height = texture->height;						   // Copy width to local structure
		tga.Bpp = texture->bpp;								   // Copy width to local structure

		if ((texture->width <= 0) || (texture->height <= 0) || ((texture->bpp != 24) && (texture->bpp != 32))) //Make sure all texture info is ok
		{
			//DEBUG("Invalid texture information" << filename);
			//if(fTGA != NULL)													// Check if file is open
			{
				File.Close(); // Ifit is, close it
			}
			return false; // Return failed
		}

		if (texture->bpp == 24)		 // If the BPP of the image is 24...
			texture->type = GL_RGB;	 // Set Image type to GL_RGB
		else						 // Else if its 32 BPP
			texture->type = GL_RGBA; // Set image type to GL_RGBA

		tga.bytesPerPixel = (tga.Bpp / 8);							  // Compute BYTES per pixel
		tga.imageSize = (tga.bytesPerPixel * tga.Width * tga.Height); // Compute amout of memory needed to store image
		texture->imageData = (GLubyte *)malloc(tga.imageSize);		  // Allocate that much memory

		if (texture->imageData == NULL) // If it wasnt allocated correctly..
		{
			//DEBUG("Could not allocate memory for image " << filename << " size " << tga.imageSize);
			File.Close(); // Close file
			return false; // Return failed
		}

		GLuint pixelcount = tga.Height * tga.Width;					 // Nuber of pixels in the image
		GLuint currentpixel = 0;									 // Current pixel being read
		GLuint currentbyte = 0;										 // Current byte
		GLubyte *colorbuffer = (GLubyte *)malloc(tga.bytesPerPixel); // Storage for 1 pixel

		do
		{
			GLubyte chunkheader = 0; // Storage for "chunk" header

			if (!File.Read(&chunkheader, sizeof(GLubyte))) // Read in the 1 byte header
			{
				File.Close();					// Close file
				if (texture->imageData != NULL) // If there is stored image data
					free(texture->imageData);	// Delete image data

				return false; // Return failed
			}

			if (chunkheader < 128)										  // If the header is < 128, it means the that is the number of RAW color packets minus 1
			{															  // that follow the header
				chunkheader++;											  // add 1 to get number of following color values
				for (short counter = 0; counter < chunkheader; counter++) // Read RAW color values
				{
					//if(fread(colorbuffer, 1, tga.bytesPerPixel, fTGA) != tga.bytesPerPixel) // Try to read 1 pixel
					if (!File.Read(colorbuffer, tga.bytesPerPixel))
					{
						//DEBUG("Could not read image1 data " << filename);

						//if(fTGA != NULL)													// See if file is open
						{
							File.Close(); // If so, close file
						}

						if (colorbuffer != NULL) // See if colorbuffer has data in it
						{
							free(colorbuffer); // If so, delete it
						}

						if (texture->imageData != NULL) // See if there is stored Image data
						{
							free(texture->imageData); // If so, delete it too
						}

						return false; // Return failed
					}
					// write to memory
					texture->imageData[currentbyte] = colorbuffer[2]; // Flip R and B vcolor values around in the process
					texture->imageData[currentbyte + 1] = colorbuffer[1];
					texture->imageData[currentbyte + 2] = colorbuffer[0];

					if (tga.bytesPerPixel == 4) // if its a 32 bpp image
					{
						texture->imageData[currentbyte + 3] = colorbuffer[3]; // copy the 4th byte
					}

					currentbyte += tga.bytesPerPixel; // Increase thecurrent byte by the number of bytes per pixel
					currentpixel++;					  // Increase current pixel by 1

					if (currentpixel > pixelcount) // Make sure we havent read too many pixels
					{
						//DEBUG("Too many pixels read " << pixelcount << " fn: " << filename);

						//if(fTGA != NULL)													// If there is a file open
						{
							File.Close(); // Close file
						}

						if (colorbuffer != NULL) // If there is data in colorbuffer
						{
							free(colorbuffer); // Delete it
						}

						if (texture->imageData != NULL) // If there is Image data
						{
							free(texture->imageData); // delete it
						}

						return false; // Return failed
					}
				}
			}
			else // chunkheader > 128 RLE data, next color reapeated chunkheader - 127 times
			{
				chunkheader -= 127; // Subteact 127 to get rid of the ID bit
				//if(fread(colorbuffer, 1, tga.bytesPerPixel, fTGA) != tga.bytesPerPixel)		// Attempt to read following color values
				if (!File.Read(colorbuffer, tga.bytesPerPixel))
				{
					//DEBUG("Could not read from file " << filename);

					//if(fTGA != NULL)														// If thereis a file open
					{
						File.Close(); // Close it
					}

					if (colorbuffer != NULL) // If there is data in the colorbuffer
					{
						free(colorbuffer); // delete it
					}

					if (texture->imageData != NULL) // If thereis image data
					{
						free(texture->imageData); // delete it
					}

					return false; // return failed
				}

				for (short counter = 0; counter < chunkheader; counter++) // copy the color into the image data as many times as dictated
				{														  // by the header
					texture->imageData[currentbyte] = colorbuffer[2];	  // switch R and B bytes areound while copying
					texture->imageData[currentbyte + 1] = colorbuffer[1];
					texture->imageData[currentbyte + 2] = colorbuffer[0];

					if (tga.bytesPerPixel == 4) // If TGA images is 32 bpp
					{
						texture->imageData[currentbyte + 3] = colorbuffer[3]; // Copy 4th byte
					}

					currentbyte += tga.bytesPerPixel; // Increase current byte by the number of bytes per pixel
					currentpixel++;					  // Increase pixel count by 1

					if (currentpixel > pixelcount) // Make sure we havent written too many pixels
					{
						//DEBUG("Too many pixels read "  << filename << " mx: " << pixelcount);

						//if(fTGA != NULL)													// If there is a file open
						{
							File.Close(); // Close file
						}

						if (colorbuffer != NULL) // If there is data in colorbuffer
						{
							free(colorbuffer); // Delete it
						}

						if (texture->imageData != NULL) // If there is Image data
						{
							free(texture->imageData); // delete it
						}

						return false; // Return failed
					}
				}
			}
		}

		while (currentpixel < pixelcount); // Loop while there are still pixels left
		File.Close();					   // Close the file
		return true;					   // return success
	}

} // namespace Tartan
