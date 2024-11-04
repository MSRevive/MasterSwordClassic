//! \file
//! \brief tga file data structure header

#ifndef __TGA_H__
#define __TGA_H__

#ifdef _WIN32
#include <windows.h> // Standard windows header
#else
typedef unsigned char byte;
#endif
#include <stdio.h> // Standard I/O header
#include <GL/gl.h> // Header for OpenGL32 library

#include "texturestruct.h"

#pragma pack(push, 1)
namespace Tartan
{

    typedef struct
    {
        //GLubyte Header[12];									//!< TGA File Header
        byte identsize;     // size of ID field that follows 18 byte header (0 usually)
        byte colourmaptype; // type of colour map 0=none, 1=has palette
        byte imagetype;     // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

        short colourmapstart;  // first colour map entry in palette
        short colourmaplength; // number of colours in palette
        byte colourmapbits;    // number of bits per palette entry 15,16,24,32

        short xstart; // image x origin
        short ystart; // image y origin
        /*short width;              // image width in pixels
    short height;             // image height in pixels
    byte  bits;               // image bits per pixel 8,16,24,32
    byte  descriptor;         // image descriptor bits (vh flip bits)*/

    } TGAHeader;

    typedef struct
    {
        GLubyte header[6];    //!< First 6 Useful Bytes From The Header
        GLuint bytesPerPixel; //!< Holds Number Of Bytes Per Pixel Used In The TGA File
        GLuint imageSize;     //!< Used To Store The Image Size When Setting Aside Ram
        GLuint temp;          //!< Temporary Variable
        GLuint type;
        GLuint Height; //!< Height of Image
        GLuint Width;  //!< Width ofImage
        GLuint Bpp;    //!< Bits Per Pixel
    } TGA;

    TGAHeader tgaheader; //!< TGA header
    TGA tga;             //!< TGA image data

    GLubyte uTGAcompare[12] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};  //!< Uncompressed TGA Header
    GLubyte cTGAcompare[12] = {0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //!< Compressed TGA Header
    bool LoadUncompressedTGA(Texture *);                             //!< Load an Uncompressed file
    bool LoadCompressedTGA(Texture *);                               //!< Load a Compressed file

} // namespace Tartan
#pragma pack(pop)

#endif
