//The client-side enviroment
//

//Used by various source files

class CEnvMgr
{
public:
	static void Init();
	static void InitNewLevel();
	static void Think_DrawTransparentTriangles();
	static void RenderSky();
	static void Cleanup();

	static void ChangeSkyTexture(msstring_ref NewTexture);
	static void ChangeTint(const Color4F &Color);
	static void SetLightGamma(float Value);

	static float m_LightGamma;
	static float m_MaxViewDistance;

	//Fog
	struct fog_t
	{
		bool Enabled;
		Vector Color;
		float Density, Start, End;
		int Type;
	};
	static fog_t m_Fog;
};

//Mastersword Special rendering
class CRender
{
public:
	static void PushHLStates();
	static void PopHLStates();

	static void Cleanup();

	//Render to texture interface
	static void RT_ClearBuffer(bool ClearColor, bool ClearDepth, bool ClearStencil);
	static void SetRenderTarget(bool ToTexture, bool ClearDepth = true);
	static void SyncOffScreenSurface();

	static bool CheckOpenGL(); //MS only works in openGL
	static void CleanupWGL();
	static void RT_GetNewFrameBufferTexture(uint &TextureID);
	static void RT_BindTexture();
	static void RT_ReleaseTexture();

	static int m_OldHLTexture[10];
	static bool m_OldMultiTextureEnabled;

	static float m_RT_SizeRatio;
	static uint m_RT_Width; //Properties of offscreen buffer
	static uint m_RT_Height;
	static float m_RT_TexU;
	static float m_RT_TexV;
};
