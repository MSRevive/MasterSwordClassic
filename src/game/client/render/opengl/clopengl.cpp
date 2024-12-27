//Render mirrors
#include "inc_weapondefs.h"
#include "../hud.h"
#include "../cl_util.h"
#include "com_model.h"
#include "studio.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "com_model.h"
#include "../studio_util.h"
#include "../r_studioint.h"
#include "../studiomodelrenderer.h"
#include "../gamestudiomodelrenderer.h"
#include "pm_movevars.h"
#include "clopengl.h" // OpenGL stuff
#include "SDL2/SDL_messagebox.h"

#ifdef _WIN32
// WGL_ARB_extensions_string
PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = NULL;

// WGL_ARB_pbuffer
PFNWGLCREATEPBUFFERARBPROC wglCreatePbufferARB = NULL;
PFNWGLGETPBUFFERDCARBPROC wglGetPbufferDCARB = NULL;
PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB = NULL;
PFNWGLDESTROYPBUFFERARBPROC wglDestroyPbufferARB = NULL;
PFNWGLQUERYPBUFFERARBPROC wglQueryPbufferARB = NULL;

// WGL_ARB_pixel_format
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = NULL;

// WGL_ARB_render_texture
PFNWGLBINDTEXIMAGEARBPROC wglBindTexImageARB = NULL;
PFNWGLRELEASETEXIMAGEARBPROC wglReleaseTexImageARB = NULL;
PFNWGLSETPBUFFERATTRIBARBPROC wglSetPbufferAttribARB = NULL;
#endif // _WIN32

void VectorAngles(const float *forward, float *angles);
void GenerateInverseMatrix4f(const float inMatrix[4][4], float outInverse[4][4]);

uint id = 0;

struct Vertex
{
	float tu, tv;
	float x, y, z;
};

Vertex g_cubeVertices[] =
	{
		{0.0f, 0.0f, -1.0f, -1.0f, 1.0f},
		{1.0f, 0.0f, 1.0f, -1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
		{0.0f, 1.0f, -1.0f, 1.0f, 1.0f},

		{1.0f, 0.0f, -1.0f, -1.0f, -1.0f},
		{1.0f, 1.0f, -1.0f, 1.0f, -1.0f},
		{0.0f, 1.0f, 1.0f, 1.0f, -1.0f},
		{0.0f, 0.0f, 1.0f, -1.0f, -1.0f},

		{0.0f, 1.0f, -1.0f, 1.0f, -1.0f},
		{0.0f, 0.0f, -1.0f, 1.0f, 1.0f},
		{1.0f, 0.0f, 1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f, 1.0f, -1.0f},

		{1.0f, 1.0f, -1.0f, -1.0f, -1.0f},
		{0.0f, 1.0f, 1.0f, -1.0f, -1.0f},
		{0.0f, 0.0f, 1.0f, -1.0f, 1.0f},
		{1.0f, 0.0f, -1.0f, -1.0f, 1.0f},

		{1.0f, 0.0f, 1.0f, -1.0f, -1.0f},
		{1.0f, 1.0f, 1.0f, 1.0f, -1.0f},
		{0.0f, 1.0f, 1.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 1.0f, -1.0f, 1.0f},

		{0.0f, 0.0f, -1.0f, -1.0f, -1.0f},
		{1.0f, 0.0f, -1.0f, -1.0f, 1.0f},
		{1.0f, 1.0f, -1.0f, 1.0f, 1.0f},
		{0.0f, 1.0f, -1.0f, 1.0f, -1.0f}};

//#include "r_efx.h"
#include "entity_types.h"
#include "clglobal.h"
#include "../clrender.h"

#include "logger.h"

#ifdef _WIN32
typedef struct
{

	HPBUFFERARB hPBuffer;
	HDC hDC;
	HGLRC hRC;
} PBUFFER;

HDC OldhDC;
HGLRC OldhRC;

PBUFFER g_pbuffer;
#endif

void CRender::RT_GetNewFrameBufferTexture(uint &Tex)
{
	//
	// This is our dynamic texture, which will be loaded with new pixel data
	// after we're finshed rendering to the p-buffer.
	//

	SetRenderTarget(true);
	glGenTextures(1, &Tex);
	glBindTexture(GL_TEXTURE_2D, Tex);
	//glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR );
	//glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR );
	SetRenderTarget(false);
}

void CRender::RT_BindTexture()
{
#ifdef _WIN32
	if (!wglBindTexImageARB(g_pbuffer.hPBuffer, WGL_FRONT_LEFT_ARB))
	{
		//MessageBox(NULL, "Could not bind p-buffer to render texture!", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Renderer Error", "Could not bind p-buffer to render texture!", NULL);
		logfile << Logger::LOG_ERROR << "Error: Could not bind p-buffer to render texture!\n";
		Print("Could not bind p-buffer to render texture!");
		//exit(-1);
	}
#endif
}

void CRender::RT_ReleaseTexture()
{
	//we need to make sure that the p-buffer has been
	// released from the dynamic "render-to" texture.
	//

#ifdef _WIN32
	if (!wglReleaseTexImageARB(g_pbuffer.hPBuffer, WGL_FRONT_LEFT_ARB))
	{
		//MessageBox(NULL, "Could not release p-buffer from render texture!", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Renderer Error", "Could not release p-buffer from render texture!", NULL);
		//exit(-1);
	}
#endif
}
struct gllightinfo_t
{
	int Enabled;
	float Pos[4]; //In GL, it's 4 floats.  The last one specifies a sort of attenuation
	float AmbientColor[4], DiffuseColor[4], SpecularColor[4];
	float ConstAttn;
};

// Used to setup the offscreen surface before rendering to it
// Copies the GL values used from HL's framebuffer
void CRender::SyncOffScreenSurface()
{
#ifdef _WIN32
	static bool copied = false;

	//if( !copied )
	{
		//wglCopyContext( OldhRC, g_pbuffer.hRC, /*GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT |*/ GL_TEXTURE_BIT );
		//wglCopyContext( OldhRC, g_pbuffer.hRC, GL_LIGHTING_BIT );
	}

	float mp[16], mmv[16] /*, mt[16]*/;
	glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat *)mp);
	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)mmv);
	//glGetFloatv( GL_TEXTURE_MATRIX, (GLfloat *)mt );

	SetRenderTarget(true, false);

	if (!copied)
	{
		/*glEnable( GL_COLOR_MATERIAL );
 		glColorMaterial( GL_FRONT, GL_AMBIENT_AND_DIFFUSE );
		glColorMaterial( GL_FRONT, GL_EMISSION );
		glColorMaterial( GL_FRONT, GL_SPECULAR );*/
	}

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(mp);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(mmv);

	//glMatrixMode( GL_TEXTURE );
	//glLoadMatrixf( mt );

	//glClear( GL_DEPTH_BUFFER_BIT );
	copied = true;
#endif // _WIN32

	SetRenderTarget(false);
}
void CRender::RT_ClearBuffer(bool ClearColor, bool ClearDepth, bool ClearStencil)
{
	//glClearColor( 0.0f, 0.0f, 1.0f, 0.0f );

	if (ClearColor)
		glClear(GL_COLOR_BUFFER_BIT);
	if (ClearDepth)
		glClear(GL_DEPTH_BUFFER_BIT);
	if (ClearStencil)
		glClear(GL_STENCIL_BUFFER_BIT);
}
void CRender::SetRenderTarget(bool ToTexture, bool ClearDepth)
{
#ifdef _WIN32
	if (ToTexture)
	{
		if (!wglMakeCurrent(g_pbuffer.hDC, g_pbuffer.hRC))
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Renderer Error", "Could not make the p-buffer's context current!", NULL);
			//MessageBox(NULL, "Could not make the p-buffer's context current!", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			//exit(-1);
		}
		if (ClearDepth)
			glClear(GL_DEPTH_BUFFER_BIT);
		/*
		//TEST:  Force the view and draw a cube.  It tests current GL properties such as lighting
		
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		gluPerspective( 45.0, (GLdouble)m_RT_Width / (GLdouble)m_RT_Height, 0.1, 1000.0);

		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();

		LoadGLTexture( "hey.tga", id );		//call load each frame... takes advantage of the texture caching
		glBindTexture( GL_TEXTURE_2D, id );
		glTranslatef( 0.0f, 0.0f, -5.0f );
		glRotatef( -30, 1.0f, 0.0f, 0.0f );
		glRotatef( -30, 0.0f, 1.0f, 0.0f );

		//
		// Now the render the cube to the p-buffer just like you we would have 
		// done with the regular window.
		//

		glInterleavedArrays( GL_T2F_V3F, 0, g_cubeVertices );
		glDrawArrays( GL_QUADS, 0, 24 );*/
	}
	else
	{
		//glMatrixMode( GL_PROJECTION );
		//glPopMatrix( );

		if (!wglMakeCurrent(OldhDC, OldhRC))
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Renderer Error", "Could not make the normal half-life context current!", NULL);
			//MessageBox(NULL, "Could not make the normal half-life context current!", "ERROR", MB_OK | MB_ICONEXCLAMATION);
			//exit(-1);
		}
	}
#endif
}

bool CMirrorMgr::InitMirrors()
{
	//CRender::m_RT_SizeRatio = 0.2f;
	UseMirrors = false;
	if (!IEngineStudio.IsHardware()) //Not using openGL
		return false;

	const char *VenderString = (const char *)glGetString(GL_VENDOR);
	const char *CardString = (const char *)glGetString(GL_RENDERER);
	const char *VersionString = (const char *)glGetString(GL_VERSION);
	const char *ExtensionsString = (const char *)glGetString(GL_EXTENSIONS);
	logfile << Logger::LOG_INFO << "Video Card Vender: " << VenderString << "\n";
	logfile << Logger::LOG_INFO << "Video Card: " << CardString << "\n";
	logfile << Logger::LOG_INFO << "OpenGL Version: " << VersionString << "\n";
	logfile << Logger::LOG_INFO << "OpenGL Extensions: " << ExtensionsString << "\n";

	if (atof(VersionString) < 1.1) //Not high enough OpenGL version
	{
		logfile << Logger::LOG_INFO << "\nOpenGL Version Not High Enough For Mirrors (Needs 1.1)!\n";
		return false;
	}

#ifdef _WIN32
	glMultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARBPROC)wglGetProcAddress("glMultiTexCoord2fARB");
	glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTextureARB");
#endif

	logfile << Logger::LOG_INFO << "OpenGL ActiveTexture Extention: " << (glActiveTextureARB ? "FOUND" : "NOT FOUND") << "\n";

#ifdef _WIN32
	if (!glActiveTextureARB) //Doesn't support Multitexturing
		return false;

	//Get the WGL extensions string
	wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
	const char *wglExtensions = wglGetExtensionsStringARB(wglGetCurrentDC());

	//Set up wgl extensions
	if (!strstr(wglExtensions, "WGL_ARB_pbuffer") || !strstr(wglExtensions, "WGL_ARB_pixel_format") || !strstr(wglExtensions, "WGL_ARB_render_texture"))
	{
		logfile << Logger::LOG_WARN << "\nOpenGL Version Not High Enough For Mirrors (wglExtensions not Present)!\n";
		return false;
	}

	OldhDC = wglGetCurrentDC();
	OldhRC = wglGetCurrentContext();

	//Init the pbuffer
	wglCreatePbufferARB = (PFNWGLCREATEPBUFFERARBPROC)wglGetProcAddress("wglCreatePbufferARB");
	wglGetPbufferDCARB = (PFNWGLGETPBUFFERDCARBPROC)wglGetProcAddress("wglGetPbufferDCARB");
	wglReleasePbufferDCARB = (PFNWGLRELEASEPBUFFERDCARBPROC)wglGetProcAddress("wglReleasePbufferDCARB");
	wglDestroyPbufferARB = (PFNWGLDESTROYPBUFFERARBPROC)wglGetProcAddress("wglDestroyPbufferARB");
	wglQueryPbufferARB = (PFNWGLQUERYPBUFFERARBPROC)wglGetProcAddress("wglQueryPbufferARB");
	wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	wglBindTexImageARB = (PFNWGLBINDTEXIMAGEARBPROC)wglGetProcAddress("wglBindTexImageARB");
	wglReleaseTexImageARB = (PFNWGLRELEASETEXIMAGEARBPROC)wglGetProcAddress("wglReleaseTexImageARB");
	wglSetPbufferAttribARB = (PFNWGLSETPBUFFERATTRIBARBPROC)wglGetProcAddress("wglSetPbufferAttribARB");

	//
	// Define the minimum pixel format requirements we will need for our
	// p-buffer. A p-buffer is just like a frame buffer, it can have a depth
	// buffer associated with it and it can be double buffered.
	//

	int pf_attr[] =
		{
			WGL_SUPPORT_OPENGL_ARB, TRUE,		// P-buffer will be used with OpenGL
			WGL_DRAW_TO_PBUFFER_ARB, TRUE,		// Enable render to p-buffer
			WGL_BIND_TO_TEXTURE_RGBA_ARB, TRUE, // P-buffer will be used as a texture
			WGL_RED_BITS_ARB, 8,				// At least 8 bits for RED channel
			WGL_GREEN_BITS_ARB, 8,				// At least 8 bits for GREEN channel
			WGL_BLUE_BITS_ARB, 8,				// At least 8 bits for BLUE channel
			WGL_ALPHA_BITS_ARB, 8,				// At least 8 bits for ALPHA channel
			WGL_DEPTH_BITS_ARB, 16,				// At least 16 bits for depth buffer
			WGL_DOUBLE_BUFFER_ARB, FALSE,		// We don't require double buffering
			0									// Zero terminates the list
		};

	unsigned int count = 0;
	int pixelFormat = 0;
	wglChoosePixelFormatARB(OldhDC, (const int *)pf_attr, NULL, 1, &pixelFormat, &count);

	if (count == 0)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Renderer Error", "Could not find an acceptable pixel format!", NULL);
		//MessageBox(NULL, "Could not find an acceptable pixel format!", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		//exit(-1);
	}

	//
	// Set some p-buffer attributes so that we can use this p-buffer as a
	// 2D RGBA texture target.
	//

	int pb_attr[] =
		{
			WGL_TEXTURE_FORMAT_ARB, WGL_TEXTURE_RGBA_ARB, // Our p-buffer will have a texture format of RGBA
			WGL_TEXTURE_TARGET_ARB, WGL_TEXTURE_2D_ARB,	  // Of texture target will be GL_TEXTURE_2D
			0											  // Zero terminates the list
		};

	//
	// Create the p-buffer...
	//

	GetCompatibleTextureSize(ScreenWidth * CRender::m_RT_SizeRatio, ScreenHeight * CRender::m_RT_SizeRatio, CRender::m_RT_Width, CRender::m_RT_Height, CRender::m_RT_TexU, CRender::m_RT_TexV);

	g_pbuffer.hPBuffer = wglCreatePbufferARB(OldhDC, pixelFormat, CRender::m_RT_Width, CRender::m_RT_Height, pb_attr);
	g_pbuffer.hDC = wglGetPbufferDCARB(g_pbuffer.hPBuffer);
	g_pbuffer.hRC = wglCreateContext(g_pbuffer.hDC);

	wglShareLists(OldhRC, g_pbuffer.hRC);

	if (!g_pbuffer.hPBuffer)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Renderer Error", "Could not create Offscreen buffer", NULL);
		//MessageBox(NULL, "Could not create Offscreen buffer", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		//exit(-1);
	}

	int h;
	int w;
	wglQueryPbufferARB(g_pbuffer.hPBuffer, WGL_PBUFFER_WIDTH_ARB, &h);
	wglQueryPbufferARB(g_pbuffer.hPBuffer, WGL_PBUFFER_WIDTH_ARB, &w);

	/*if( h != g_pbuffer.nHeight || w != g_pbuffer.nWidth )
	{
		MessageBox(NULL,"The width and height of the created p-buffer don't match the requirements!",
			"ERROR",MB_OK|MB_ICONEXCLAMATION);
		exit(-1);
	}*/

	//
	// We were successful in creating a p-buffer. We can now make its context
	// current and set it up just like we would a regular context
	// attached to a window.
	//

	//glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	//glEnable(GL_TEXTURE_2D);
	//glEnable(GL_DEPTH_TEST);

	//glMatrixMode( GL_PROJECTION );
	//glLoadIdentity();
	//gluPerspective( 45.0f, CRender::m_RT_Width / CRender::m_RT_Height, 0.1f, 10.0f );

	//uint Tex = 0;
	//CRender::RT_GetNewFrameBufferTexture( Tex );

	//CRender::SetRenderTarget( true );

	if (!wglMakeCurrent(g_pbuffer.hDC, g_pbuffer.hRC))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Renderer Error", "Could not make the p-buffer's context current!", NULL);
		//MessageBox(NULL, "Could not make the p-buffer's context current!", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		//exit(-1);
	}

	//glDrawBuffer( GL_FRONT );
	//glReadBuffer( GL_FRONT );

	glViewport(0.0, 0.0, CRender::m_RT_Width * CRender::m_RT_TexU, CRender::m_RT_Height * CRender::m_RT_TexV);
	glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
	//	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, Color4F(1, 1, 1, 1));

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
	glShadeModel(GL_FLAT);

	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glColor4f(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//hints
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	CRender::SetRenderTarget(false);
	//wglCopyContext( OldhRC, g_pbuffer.hRC, GL_LIGHTING_BIT | GL_TEXTURE_BIT );

	UseMirrors = true;
	m_Initialized = false;

	m_WorldMirrors.clearitems();  //Reset world mirrors
	m_WorldSurfaces.clearitems(); //Reset world special surfaces

	return true;
#else
	logfile << Logger::LOG_WARN << "\nOpenGL mirrors are not implemented on this platform\n";
	return false;
#endif
}

void CRender::CleanupWGL()
{
	//glDeleteTextures( 1, &g_testTextureID );
	//glDeleteTextures( 1, &g_dynamicTextureID );

	//
	// Don't forget to clean up after our p-buffer...
	//
#ifdef _WIN32
	if (g_pbuffer.hRC != NULL)
	{
		//wglMakeCurrent( g_pbuffer.hDC, g_pbuffer.hRC );
		wglDeleteContext(g_pbuffer.hRC);
		wglReleasePbufferDCARB(g_pbuffer.hPBuffer, g_pbuffer.hDC);
		wglDestroyPbufferARB(g_pbuffer.hPBuffer);
		g_pbuffer.hRC = NULL;
	}

	if (g_pbuffer.hDC != NULL)
	{
		//ReleaseDC( g_hWnd, g_pbuffer.hDC );
		ReleaseDC(NULL, g_pbuffer.hDC);
		g_pbuffer.hDC = NULL;
	}
#endif
}

bool CRender::CheckOpenGL()
{
	//Thothie attempting to enable D3D (failed)
	if (!IEngineStudio.IsHardware())
	{
		//MessageBox(NULL, "Master Sword uses features only available in OpenGL mode! Attempting other modes may result in instability.", "Invalid Video Mode", MB_OK);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Invalid Video Mode", "We make use of features only in OpenGL!", NULL);
		MSErrorConsoleText("CRender::CheckOpenGL", "User chose non-opengl video mode (semi-fatal)");
		//exit( 0 );
		return false;
	}

	return true;
}
