
//Render mirrors
#include "inc_weapondefs.h"
#include "../hud.h"
#include "../cl_util.h"
#include "com_model.h"
#include "studio.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "dlight.h"
#include "triangleapi.h"
#include "com_model.h"
#include "studio_util.h"
#include "../r_studioint.h"
#include "studiomodelrenderer.h"
#include "gamestudiomodelrenderer.h"
#include "pm_movevars.h"
#include "opengl/clopengl.h" // OpenGL stuff

void VectorAngles(const float *forward, float *angles);
void GenerateInverseMatrix4f(const float inMatrix[4][4], float outInverse[4][4]);

//#include "r_efx.h"
#include "entity_types.h"
#include "clglobal.h"
#include "clrender.h"

#include "hudmisc.h"
#include "logger.h"
extern CGameStudioModelRenderer g_StudioRenderer;
extern float ClFOV;

mleaf_t *FindLeaf(Vector &Origin, mnode_t *pNode);
bool CheckSurface(TraverseInfo_t &Info, msurface_t *pSurface);
bool IsLeafVisible(cl_entity_t *pEntity, mleaf_t *pStartLeaf, mleaf_t *pSearchLeaf);
bool IsLeafVisible(cl_entity_t *pEntity, mnode_t *pNode, mleaf_t *pSearchLeaf);
bool IsBoundsVisible(cl_entity_t *pEntity, mleaf_t *pStartLeaf, Vector Bounds[2]);
bool Mirror_DrawSurface(TraverseInfo_t &Info, msurface_t *pSurface);
void MirrorPoint(Vector &OrigPoint, Plane &Plane, Vector &MirroredPoint)
{
	float Dist = Plane.GetDist(OrigPoint);
	MirroredPoint = OrigPoint + ((-Plane.m_Normal) * (Dist * 2));
}

mslist<uint> CMirrorMgr::m_MirrorTextures;			  //List of mirror textures in level
mslist<CMirror> CMirrorMgr::m_Mirrors;				  //List of mirrors in the level
mslist<CMirror *> CMirrorMgr::m_RdrMirrors;			  //List of mirrors to render this frame
mslist<CMirror> CMirrorMgr::m_WorldMirrors;			  //List of mirrors attached to the world
mslist<rendersurface_t> CMirrorMgr::m_RenderSurfaces; //List of special surfaces
mslist<rendersurface_t> CMirrorMgr::m_WorldSurfaces;  //List of special surfaces attached to world (cached for speed)
mirrorprops_t CMirrorMgr::m_CurrentMirror;
mslist<cl_entity_t *> CMirrorMgr::m_BrushEnts; //List of brush entites found to be mirrored
mslist<cl_entity_t *> CMirrorMgr::m_FrameEnts; //List of entites found to be mirrored
bool CMirrorMgr::UseMirrors = false;
bool CMirrorMgr::m_Initialized = false;
struct mleaf_s *CMirrorMgr::m_pStartLeaf; //The viewer's current leaf
extern int OldVisFrame, OldContents;

//#define DEBUG_MIRRORS		//Enable viewing mirrors at world origin
//#define MIRROR_STENCIL_MASK 0x3
#define MIRROR_STENCIL_MASK 0xFFFFFFFF

class CFrustum
{
public:
	Vector m_Forward, m_Right, m_Up;
	Plane Planes[6];

	void Update()
	{
		float FOV = gHUD.m_iFOV ? ((float)gHUD.m_iFOV / 2.0f) : 45;
		float RightPlaneYaw = FOV;
		float UpPlanePitch = FOV;
		for (int i = 0; i < 6; i++)
		{
			Plane &plane = Planes[i];
			switch (i)
			{
			case 0: //Near plane
				plane.m_Normal = ViewMgr.Params->forward;
				plane.m_Dist = DotProduct(ViewMgr.Origin, plane.m_Normal);
				break;
			case 1: //Far plane
			{
				plane.m_Normal = -ViewMgr.Params->forward;
				Vector Spot = ViewMgr.Origin + ViewMgr.Params->forward * ViewMgr.Params->movevars->zmax;
				plane.m_Dist = DotProduct(Spot, plane.m_Normal);
				break;
			}
			case 2: //Right plane
			{
				Vector Forward;
				Vector NewAng = ViewMgr.Angles + Vector(0, RightPlaneYaw, 0);
				EngineFunc::MakeVectors(NewAng, Forward);
				plane.m_Normal = Forward;
				plane.m_Dist = DotProduct(ViewMgr.Origin, plane.m_Normal);
				break;
			}
			case 3: //Left plane
			{
				Vector Forward;
				Vector NewAng = ViewMgr.Angles + Vector(0, -RightPlaneYaw, 0);
				EngineFunc::MakeVectors(NewAng, Forward);
				plane.m_Normal = Forward;
				plane.m_Dist = DotProduct(ViewMgr.Origin, plane.m_Normal);
				break;
			}
			case 4: //Top plane
			{
				Vector Forward;
				Vector NewAng = ViewMgr.Angles + Vector(-UpPlanePitch, 0, 0);
				EngineFunc::MakeVectors(NewAng, Forward);
				plane.m_Normal = Forward;
				plane.m_Dist = DotProduct(ViewMgr.Origin, plane.m_Normal);
				break;
			}
			case 5: //Bottom plane
			{
				Vector Forward;
				Vector NewAng = ViewMgr.Angles + Vector(UpPlanePitch, 0, 0);
				EngineFunc::MakeVectors(NewAng, Forward);
				plane.m_Normal = Forward;
				plane.m_Dist = DotProduct(ViewMgr.Origin, plane.m_Normal);
				break;
			}
			}
		}
	}

	bool IsBBoxVisible(Vector Bounds[2])
	{
		for (int i = 0; i < 6; i++)
		{
			Plane &plane = Planes[i];
			if (!plane.BBoxIsInFront(Bounds))
				return false;
		}

		return true;
	}
} Frustum;

bool CheckBBox(Vector Bounds[2])
{
	return Frustum.IsBBoxVisible(Bounds);
}

void CMirrorMgr::Cleanup()
{
	//Clear out old mirror data
	for (int m = 0; m < CMirrorMgr::m_MirrorTextures.size(); m++)
	{
		//CMirror &Mirror = CMirrorMgr::m_LevelMirrors[m];
		glDeleteTextures(1, &m_MirrorTextures[m]);
	}

	CMirrorMgr::m_MirrorTextures.clearitems();
	CMirrorMgr::m_Mirrors.clearitems();
	CMirrorMgr::m_RdrMirrors.clearitems();
	CMirrorMgr::m_CurrentMirror.Enabled = false;

	MSCLGlobals::Textures.clear();
}

bool CheckSurface(TraverseInfo_t &Info, msurface_t *pSurface)
{
	//if( !FBitSet(pSurface->flags,(1<<8)) )
	//	return;

	cl_entity_t *pEntity = Info.pEntity;

	//Frontside Visiblity check
	msurface_t &Surface = *pSurface;
	float PlaneMutliplier = (FBitSet(Surface.flags, SURF_PLANEBACK) ? -1 : 1);

	Vector SurfaceNormal = Surface.plane->normal * PlaneMutliplier;
	float SurfaceDist = Surface.plane->dist * PlaneMutliplier;

	//!!! TEST TEST TEST !!!!
	/*if( SurfaceNormal.z != 1 )
		return false;
	if( Surface.plane->dist != -64 )
		return false;*/

	mstexture_t *pCustomTexture = NULL;
	for (int t = 0; t < MSCLGlobals::Textures.size(); t++)
	{
		mstexture_t &MSTexture = MSCLGlobals::Textures[t];

		// if( Surface.texinfo->texture->name[0] == '!' )

		if (!_stricmp(MSTexture.Name, Surface.texinfo->texture->name)) //Case in-sensitive compare for the texture name
		{
			pCustomTexture = &MSCLGlobals::Textures[t];
			break;
		}
	}

	if (!pCustomTexture)
		return false; //This surface is not a custom texture

	mstexture_t &CustomTexture = *pCustomTexture;

	if (CustomTexture.IsReflective)
	{
		CMirror *pUseMirror = NULL;

		for (int m = 0; m < CMirrorMgr::m_Mirrors.size(); m++)
		{
			CMirror &Mirror = CMirrorMgr::m_Mirrors[m];
			if (Mirror.Normal == SurfaceNormal &&
				Mirror.Dist == SurfaceDist)
			{
				pUseMirror = &CMirrorMgr::m_Mirrors[m];
				break;
			}
		}

		surfaceinfo_t SurfInfo;
		Vector Bounds[2];
		Bounds[0] = Vector(9999999.9f, 9999999.9f, 9999999.9f);
		Bounds[1] = Vector(-9999999.9f, -9999999.9f, -9999999.9f);
		float *Vertex = Surface.polys->verts[0];
		for (int t = 0; t < Surface.polys->numverts; t++, Vertex += VERTEXSIZE)
		{
			Vector Vert = Vector(Vertex);
			for (int i = 0; i < 3; i++)
			{
				if (Vert[i] < Bounds[0][i])
					Bounds[0][i] = Vert[i];
				if (Vert[i] > Bounds[1][i])
					Bounds[1][i] = Vert[i];
			}
		}

		SurfInfo.Origin = (Bounds[0] + Bounds[1]) * 0.5f;
		SurfInfo.Radius = (Bounds[1] - Bounds[0]).Length();
		//float Dist = (SurfInfo.Origin - ViewMgr.Origin).Length( );
		//if( Dist > (SurfInfo.Radius + pCustomTexture->Mirror.Range) )
		//	return false;

		if (!pUseMirror)
		{
			//Create new mirror
			//A mirror can extend across multiple faces - even faces across the level,
			//as long as they have the same plane and dist
			CMirror TmpMirror;
			pUseMirror = &CMirrorMgr::m_Mirrors.add(TmpMirror);
			memset(pUseMirror, 0, sizeof(CMirror));

			int MirrorNum = CMirrorMgr::m_Mirrors.size() - 1;
			if (MirrorNum < (signed)CMirrorMgr::m_MirrorTextures.size())
				pUseMirror->GLBufferTexture = CMirrorMgr::m_MirrorTextures[MirrorNum]; //Re-use old Buffer texture
			else
			{
				glGenTextures(1, (GLuint *)&pUseMirror->GLBufferTexture); // Create a new Buffer Texture
				glBindTexture(GL_TEXTURE_2D, pUseMirror->GLBufferTexture);
				//		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, CRender::m_RT_Width, CRender::m_RT_Height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0 );
				//		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, CRender::m_RT_Width, CRender::m_RT_Height, 0, GL_RGBA, GL_FLOAT, 0 );
				//glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, CRender::m_RT_Width, CRender::m_RT_Height, 0, GL_RGB, GL_FLOAT, 0 );
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

				CMirrorMgr::m_MirrorTextures.add(pUseMirror->GLBufferTexture);
			}

			pUseMirror->m_Texture = pCustomTexture;
			pUseMirror->IsWater = false;
			pUseMirror->Normal = SurfaceNormal;
			pUseMirror->Dist = SurfaceDist;
			pUseMirror->GLIgnoreTexture = Surface.texinfo->texture->gl_texturenum;
			pUseMirror->m_OnWorld = !Info.pEntity->index ? true : false;
			//Surface.texinfo->texture->gl_texturenum = 0;
			//The texture must have dimensions 2^X
			float Power = logf(ScreenWidth) / logf(2); //Use ScreenHeight because it's smaller than width
			int IntPower = (int)Power;
			if (Power > IntPower)
				IntPower++; //Screen Width is in-between standardized texure sizes.  Use the next highest size
			float TexSize = pow(2, (int)IntPower);

			//Cap texure size at GL_MAX_TEXTURE_SIZE
			int TexSizeMax = 0;
			glGetIntegerv(GL_MAX_TEXTURE_SIZE, &TexSizeMax);
			if (TexSizeMax < TexSize)
				TexSize = (float)TexSizeMax;

			pUseMirror->TexSize.x = pUseMirror->TexSize.y = TexSize;

			//logfile	<< "Mirror #" << CMirrorMgr::m_LevelMirrors.size() << " - Screen: " << ScreenWidth << "x" << ScreenHeight << " Power: " << Power << " (" << IntPower << ")" << endl;
			//logfile	<< "Mirror #" << CMirrorMgr::m_LevelMirrors.size() << " - Size: " << pUseMirror->TexSize.x << "x" << pUseMirror->TexSize.y << " Max: " << TexSizeMax << "x" << TexSizeMax << endl;

			pUseMirror->CreateMatrix();
		}

		//Add this surface to the mirror
		CMirror &Mirror = *pUseMirror;

		Mirror.Entity = pEntity;
		SetBits(Surface.flags, (1 << 8));

		CSurface NewSurface(pSurface);
		NewSurface.Bounds[0] = Bounds[0];
		NewSurface.Bounds[1] = Bounds[1];
		NewSurface.m_Origin = SurfInfo.Origin;
		//NewSurface.ParentNode = Info.pNode->parent;
		NewSurface.m_Radius = SurfInfo.Radius;

		Mirror.m_Surfaces.add(NewSurface);

		//surfaceinfo_t &SurfInfo = Mirror.SurfaceInfo.add( surfaceinfo_t() );
	}

	return true;
}

bool CMirrorMgr::Enabled()
{
	return UseMirrors && EngineFunc::CVAR_GetFloat("ms_reflect");
}

void CMirrorMgr::MarkCustomTextures()
{
	m_RdrMirrors.clearitems(); //Reset visble mirrors
	m_FrameEnts.clearitems();  //Reset entites marked for rendering last frame

	static float TimeLastUpdate = 0;
	if (gpGlobals->time > TimeLastUpdate && (gpGlobals->time - TimeLastUpdate) < 0.5f) //OPTIMIZATION: Only check for new brushes and mirros every now and then
		return;

	TimeLastUpdate = gpGlobals->time;

	//Find and mark any mirrors in the level for later rendering
	m_RenderSurfaces.clearitems(); //Reset surfaces
	m_Mirrors.clearitems();		   //Reset mirrors
	m_BrushEnts.clearitems();	   //Reset mirrored brush entities
	//m_FrameEnts.clearitems( );	//Reset mirrored entities

	bool SearchWorld = !m_Initialized;

	if (!gEngfuncs.GetEntityByIndex(0))
		return; //No world yet... keep trying until there is one

	if (!UseMirrors)
		return;

	//Search through all visible faces looking for custom textures
	cl_entity_t &WorldEntity = *gEngfuncs.GetEntityByIndex(0);
	m_pStartLeaf = FindLeaf(ViewMgr.Origin, WorldEntity.model->nodes);
	TraverseInfo_t Info;
	//Info.CheckFrustum = true;
	Info.CheckFrustum = false;
	Info.CheckClipplane = false;
	Info.Func = CheckSurface;

	if (!m_pStartLeaf || m_pStartLeaf->contents == CONTENTS_SOLID)
		return;

	for (int e = 0; e < ViewMgr.Params->max_entities; e++) //MAX_MAP_MODELS
	{
		cl_entity_t *pEntity = gEngfuncs.GetEntityByIndex(e);
		if (!pEntity || !pEntity->model)
			continue;

		model_t &Model = *pEntity->model;
		if (Model.type != mod_brush)
			continue;

		cl_entity_t &Ent = *pEntity;
		Info.pEntity = pEntity;

		if (!Model.firstmodelsurface)
		{
			if (SearchWorld)
			{
				//This is the main world model.  Traverse the BSP tree to mark textures
				ParseVisibleSurfaces(Info, m_pStartLeaf);
			}
			else if (!e)
			{
				//This is being called on a frame basis.  Don't search the world, just check cached level mirrors

				//Find any cached mirrors with at least one surface close enough to the camera
				for (int wm = 0; wm < m_WorldMirrors.size(); wm++)
				{
					CMirror &Mirror = m_WorldMirrors[wm];

					if (!Mirror.Vis_Surface())
						continue;

					m_Mirrors.add(m_WorldMirrors[wm]);
				}

				//m_Mirrors = m_WorldMirrors;
			}

			m_BrushEnts.add(pEntity);
		}
		else if (!SearchWorld)
		{
			//if( (pEntity->curstate.rendermode == kRenderTransAlpha
			//	||pEntity->curstate.rendermode == kRenderTransTexture )
			//	&& !pEntity->curstate.renderamt )
			//		continue;

			if (FBitSet(pEntity->curstate.effects, EF_NODRAW))
				continue;

			//Brush only has to be potientially visible to be drawn in the mirrored world
			m_BrushEnts.add(pEntity);

			//Now check the view frustum to see if any mirrors on this brush should be drawn
			Vector Bounds[2] = {pEntity->origin + pEntity->model->mins, pEntity->origin + pEntity->model->maxs};
			if (!CheckBBox(Bounds))
				continue;

			//Success = false;
			for (int s = 0; s < Model.nummodelsurfaces; s++)
				CheckSurface(Info, &Model.surfaces[Model.firstmodelsurface + s]);
		}
	}

	//Store a lookup table.
	//Surface --> mirror, So I can find child mirrors by the surface
	for (int i = 0; i < m_Mirrors.size(); i++)
	{
		CMirror &Mirror = m_Mirrors[i];
		for (int s = 0; s < Mirror.m_Surfaces.size(); s++)
		{
			rendersurface_t RendSurface;
			RendSurface.Mirror = &m_Mirrors[i];
			RendSurface.Surface = Mirror.m_Surfaces[i].m_Surface;
			m_RenderSurfaces.add(RendSurface);
		}
	}

	if (SearchWorld)
	{
		m_WorldMirrors = m_Mirrors;			//This is the first call since a level change.  Save the mirrors
		m_WorldSurfaces = m_RenderSurfaces; //and surfaces found in the level geometry.  Just cache them for runtime
	}

	m_Initialized = true;
}

void CMirrorMgr::Render_SetupViewReflection()
{
	//UNDONE - NICEST is too slow for a simple mirror reflection.  Use it for drawing the main world
	//glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );	// Really Nice Perspective Calculations

	if (!ViewMgr.Passes) //It's the first view... rendering the normal world
	{

		//Find visible mirrors
		//The first time this is called in a level, it finds world mirrors only and caches them
		//After that (m_Initialized), it starts with the cached list and adds on dynamic mirrors (found in visible brush ents)
		if (Enabled())
		{
			Frustum.Update();
		}

		MarkCustomTextures();

		//Reset mirrors to be drawn this frame

		//Do vis checks on mirrors
		if (Enabled() && m_Mirrors.size())
		{
			// cl_entity_t &WorldEntity = *gEngfuncs.GetEntityByIndex( 0 );

			for (int i = 0; i < m_Mirrors.size(); i++)
			{
				CMirror &Mirror = m_Mirrors[i];

				Mirror.Frame_Plane = Plane(Mirror.Normal, Mirror.Dist);

				if (!Mirror.Vis_Eye())
					continue;

				/*if( Mirror.Entity )
				{
					Vector Bounds[2] = { Mirror.Entity->model->mins, Mirror.Entity->model->maxs };
					bool Success = IsBoundsVisible( &WorldEntity, m_pStartLeaf, Bounds );
					if( !Success )
						continue;
				}*/

				m_RdrMirrors.add(&m_Mirrors[i]);
			}
		}

		static bool dbg_on = false;
		if (CVAR_GET_FLOAT("ms_reflect_dbg"))
		{
			dbg_on = true;
			dbgtxt(msstring("Mirrors Rendered: ") + (int)m_RdrMirrors.size());
		}
		else if (dbg_on)
		{
			dbg_on = false;
			dbgtxt("");
		}

		m_CurrentMirror.Index = 0;
		SetupNormalView(); //Done rendering mirrors, next, render the normal view
	}
	else if (ViewMgr.Passes == 1)
	{
		//Finished normal view.  Start rendering mirrors
		SetupMirrorView(0);
	}
	else
	{
		//Finished rendering mirror.  Save screen into a texture for rendering later

		//		CMirror &Mirror = m_RdrMirrors[CMirrorMgr::m_CurrentMirror.Index];

		/*if( !Mirror.Frame_NoRender )
		{
			CRender::PushHLStates( );

			Mirror.BindTexture( );				//Sets the texture with the copy of the mirrored screen

			///NOTE!!!! - glCopyTexImage2D must use texture sizes 2^x.  And older cards will clamp negative start coords

			glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, Mirror.TexSize.x, Mirror.TexSize.y, 0 );
			//glClear( GL_DEPTH_BUFFER_BIT );
				
			//glClear( GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT|GL_ACCUM_BUFFER_BIT );
		
			//Render next mirror
			CRender::PopHLStates( );
		}*/

		/*Mirror.Frame_NoRender = false;

		if( Mirror.Frame_IsCopy )
		{
			//This mirror was overriden by the world (a client changed leafs during this frame)
			//It is being drawn a second time to make up for that.  Now get rid of the second copy
			m_RdrMirrors.erase( CMirrorMgr::m_CurrentMirror.Index-- );
		}*/

		m_CurrentMirror.Index++;

		if (m_CurrentMirror.Index >= (signed)CMirrorMgr::m_RdrMirrors.size())
			//[Note: Should never happen, because a check is made the previous pass if there is another mirror or not]
			SetupNormalView(); //Done rendering mirrors, next, render the normal view
		else
			SetupMirrorView(m_CurrentMirror.Index); //Continue rendering mirrors
	}

	SetupNextView();
}

void CMirrorMgr::SetupNormalView()
{
	//Params.vieworg = OldView;
	//Params.viewangles = OldAng;

	ref_params_t &Params = *ViewMgr.Params;

	/*#ifdef DEBUG_MIRRORS
		//Only use a portion of the screen, so I can see what the mirror sees in the top left
		Params.viewport[0] = MIRRORTEX_W;
		Params.viewport[1] = MIRRORTEX_H;
		Params.viewport[2] = ScreenWidth - MIRRORTEX_W;
 		Params.viewport[3] = ScreenHeight - MIRRORTEX_H;
	#else*/
	Params.viewport[0] = 0;
	Params.viewport[1] = 0;
	Params.viewport[2] = ScreenWidth;
	Params.viewport[3] = ScreenHeight;
	//#endif
	m_CurrentMirror.Enabled = false;
}
void CMirrorMgr::SetupMirrorView(int MirrorNum)
{
	//CMirror &Mirror = *CMirrorMgr::m_RdrMirrors[MirrorNum];

	m_CurrentMirror.Enabled = true;
	m_CurrentMirror.Index = MirrorNum;
	m_CurrentMirror.Mirror = CMirrorMgr::m_RdrMirrors[MirrorNum];

	/*if( !Mirror.Texture->Mirror.NoWorld )
	{
		cl_entity_t *clWorldEnt = gEngfuncs.GetEntityByIndex( 0 );
		model_t *pModel = clWorldEnt->model;
		OldVisFrame = clWorldEnt->model->nodes[0].visframe;
		clWorldEnt->model->nodes[0].visframe = 0;
	}*/

	//if( Mirror.Texture->Mirror.NoWorld )
	//	glClear( GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT/*|GL_ACCUM_BUFFER_BIT*/ );
}
void CMirrorMgr::SetupNextView()
{
	ref_params_t &Params = *ViewMgr.Params;

	Params.nextView = 0;

	/*if( !m_CurrentMirror.Enabled )
	{
		if( m_RdrMirrors.size() )
			Params.nextView = 1;		//Render the first mirror next frame
		else
		{
			Params.nextView = 0;		//No mirrors visible
		}
	}
	else 
	{
		if( m_CurrentMirror.Index+1 < (signed)CMirrorMgr::m_RdrMirrors.size() )
			Params.nextView = 1;		//Render the next mirror next frame
		else
		{
			Params.nextView = 0;		//All done rendering mirrors
		}
	}*/
}

#define PLANE_X 0
#define PLANE_Y 1
#define PLANE_Z 2
//extern vec3_t v_origin, v_angles, v_cl_angles, v_sim_org, v_lastAngles;
bool Mirror_TraverseVis(TraverseInfo_t &Info);
void DrawVisLeafs(model_t *pModel, mleaf_t *pStartLeaf, TraverseInfo_t &Info);
void rdrsky();

void CMirrorMgr::HUD_DrawTransparentTriangles()
{
	if (!UseMirrors)
		return;

	CRender::PushHLStates();

	if (CMirrorMgr::m_RdrMirrors.size())
	{
		CRender::SyncOffScreenSurface();

		for (int i = 0; i < CMirrorMgr::m_RdrMirrors.size(); i++)
		{
			CMirrorMgr::SetupMirrorView(i);
			CMirrorMgr::m_RdrMirrors[i]->RenderMirroredWorld(0);

//
//This allows me to test the texture that the mirrored world is rendered onto
//
#ifdef DEBUG_MIRRORS
			if (!i)
			{
				CParticle Particle;
				//Particle.BillBoard( );
				CMirror &Mirror = *CMirrorMgr::m_RdrMirrors[i];
				int basex = 100 + i * 280;
				int basey = 380;
				int basez = 200;
				Particle.SetAngles(Vector(0, -90, 0));
				Particle.m_Origin = Vector(basex, basey, basez);
				Mirror.BindTexture();
				Particle.m_GLTex = Mirror.GLBufferTexture;
				Particle.m_Width = 256; //Mirror.TexSize.x;
				Particle.m_ContinuedParticle = false;
				Particle.Render();
				Mirror.ReleaseTexture();
			}
#endif
		}
		CMirrorMgr::SetupNormalView();
	}

	CRender::PopHLStates();
}
void CMirror::SetupLighting()
{
	/*glEnable( GL_COLOR_MATERIAL );
	glColor4f( 1, 1, 1, 1 );
 	//glDisable( GL_COLOR_MATERIAL );
	glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
	glColorMaterial( GL_FRONT_AND_BACK, GL_EMISSION );
	glColorMaterial( GL_FRONT_AND_BACK, GL_SPECULAR );
	glLightModelfv( GL_LIGHT_MODEL_AMBIENT, Color4F(1,1,1,1) );

	glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE, Color4F(1,1,1,1) );
	glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, Color4F(1,1,1,1) );
	glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, Color4F(1,1,1,1) );
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

	
	//glDisable( GL_COLOR_MATERIAL );
	glDisable(GL_ALPHA_TEST );
	//glDisable( GL_BLEND );
	glDisable( GL_LOGIC_OP );
	glDisable( GL_DITHER );
	//glBlendFunc( GL_ONE, GL_ONE );
	//glColorMask( 1, 1, 1, 1 );
	glIndexMask( 0xFFFFFFFF );*/
	//glDrawBuffer( GL_FRONT );
}
bool CMirror::Vis_Eye() //Check if the camera is facing the mirror and not behind it
{
	float Dist = DotProduct(ViewMgr.Origin, Frame_Plane.m_Normal) - Frame_Plane.m_Dist;
	if (Dist < 0)
		return false; //Player is behind this surface

	Dist = DotProduct(Frame_Plane.m_Normal, ViewMgr.Params->forward);
	if (Dist >= ClFOV)
		return false; //The player is looking the same direction as my normal

	return true;
}
bool CMirror::Vis_Surface() //Check if the camera is close enough to the mirror
{
	for (int s = 0; s < m_Surfaces.size(); s++)
	{
		CSurface &Surface = m_Surfaces[s];

		float Dist = (Surface.m_Origin - ViewMgr.Origin).Length();
		if (Dist > (Surface.m_Radius + m_Texture->Mirror.Range))
			continue;

		return true;
	}

	return false;
}

void CMirror::RenderMirroredWorld(int RecurseCall)
{
	startdbg;

	CMirror &Mirror = *this;
	if (RecurseCall >= 2)
		return;

	if (m_Parent)
	{
		//Change the mirror's orientation
		Vector Point1 = Vector(Normal.x * Dist, Normal.y * Dist, Normal.z * Dist);
		Vector Point2 = Point1 + Normal;

		float flDist = DotProduct(m_Parent->Normal, Point1) - m_Parent->Dist;
		Vector ReflectedPt1 = Point1 + -m_Parent->Normal * (flDist * 2);
		flDist = DotProduct(m_Parent->Normal, Point2) - m_Parent->Dist;
		Vector ReflectedPt2 = Point2 + -m_Parent->Normal * (flDist * 2);

		Frame_Plane.m_Normal = (ReflectedPt2 - ReflectedPt1).Normalize();
		Frame_Plane.m_Dist = DotProduct(Frame_Plane.m_Normal, ReflectedPt1);

		if (!Vis_Eye())
			return; //New orientation is not visible

		CreateMatrix(); //If this is a child, re-create the reflection matrix, based on my new orientation
	}

	//Mirror all visible surfaces

	mirrorprops_t OldCurrentMirror;

	//If child mirror, take over the spot as current mirror
	if (m_Parent)
	{
		OldCurrentMirror = CMirrorMgr::m_CurrentMirror;
		CMirrorMgr::m_CurrentMirror.Mirror = this;
	}
	Frame_ChildLevel = RecurseCall;

	CRender::SetRenderTarget(true, false); //Start rendering to offscreen buffer

	SetupLighting();

	if (!m_Texture->Mirror.NoWorld) //Drawing world.  Clear the Z and the Stencil for the mirror's offscreen surface area
	{
		if (!m_Parent)
			Draw(RDR_CLEARZ | RDR_CLRSTENCIL); //Parent mirror.  Clear the Z buffer for the mirror's full area.
		else
			Draw(RDR_CLEARZ | RDR_CHKSTENCIL); //It's a child mirror.  Clear the Z buffer only within my stencil section.

		SetStencil(); //Limit rendering to the cleared area.  For parent, that's the full mirror. For child, just a section
	}
	else
	{
		Draw(RDR_CLEARZ | RDR_CLRALPHA); //If not rendering world, clear the color alpha instead of stencil
										 //CRender::RT_ClearBuffer( true, false );	//The entites are drawn with alpha = 1,
										 //so the area in-between the entites is masked out
	}

	TraverseInfo_t Info;
	Info.Func = Mirror_DrawSurface;
	Info.Mirror = this;
	Info.RecurseCall = RecurseCall;
	Info.CheckClipplane = true;
	//Info.CheckFrustum = true;
	//Info.CheckClipplane = false;
	Info.CheckFrustum = false;
	EnableClippingPlane(true);

	//Render world
	if (m_Parent)
		SetCustomTextureSettings(true);

	if (!m_Texture->Mirror.NoWorld)
	{
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

		ApplyTransformation();
		CEnvMgr::RenderSky();

		gEngfuncs.pTriAPI->CullFace(TRI_NONE);

		for (int i = 0; i < CMirrorMgr::m_BrushEnts.size(); i++)
		{
			cl_entity_t *pEntity = CMirrorMgr::m_BrushEnts[i];
			if (!pEntity || !pEntity->model)
				continue;

			model_t &Model = *pEntity->model;
			if (Model.type == mod_brush)
			{
				Info.pEntity = pEntity;
				Info.Origin = ViewMgr.Origin;
				Info.pModel = pEntity->model;
				Info.pNode = Info.pModel->nodes;
				Info.TraverseAmt = 0;

				glPushMatrix();

				Info.ClipPlane = Plane(Normal, Dist); //Always use the original normal for this.  Skips objects behind the mirror

				//Move/Rotate moving brushes

				cl_entity_t &Ent = *pEntity;

				AngleMatrix(Vector(-Ent.angles[0], Ent.angles[1], -Ent.angles[2]), Info.MoveMatrix);
				Info.MoveMatrix[3][0] = Ent.origin[0];
				Info.MoveMatrix[3][1] = Ent.origin[1];
				Info.MoveMatrix[3][2] = Ent.origin[2];
				Info.MoveMatrix[3][3] = 1;
				glMultMatrixf((float *)Info.MoveMatrix);

				if (!Model.firstmodelsurface)
				{
					//This is the main world model.  Traverse the BSP tree to draw it
					ParseVisibleSurfaces(Info, CMirrorMgr::m_pStartLeaf);
				}
				else
				{
					if (Ent.curstate.rendermode = kRenderTransTexture)
					{
						glEnable(GL_BLEND); //This is just so that transparent brushes get drawn transparent.  Nothing to do with mirror in mirrot
						glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					}

					for (int s = 0; s < Model.nummodelsurfaces; s++)
						Mirror_DrawSurface(Info, &Model.surfaces[Model.firstmodelsurface + s]);
				}

				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();
			}
		}

		glPopMatrix();
	}

	if (!Mirror.m_Texture->Mirror.NoEnts)
	{
		//Alpha - If rendering world always use 1.  It's belended later.  If not, set alpha now
		float Alpha = !m_Texture->Mirror.NoWorld ? 1 : Mirror.m_Texture->Mirror.Color.a;
		gEngfuncs.pTriAPI->Color4f(Mirror.m_Texture->Mirror.Color.r, Mirror.m_Texture->Mirror.Color.g, Mirror.m_Texture->Mirror.Color.b, Alpha);
	}

	//gEngfuncs.pTriAPI->Brightness( 1 );

	//Render entities
	//Warning: Drawing entites causes half-life to kill the culling order, so restore it afterwards

	if (!Mirror.m_Texture->Mirror.NoEnts)
		for (int i = 0; i < CMirrorMgr::m_FrameEnts.size(); i++)
			RenderModel(CMirrorMgr::m_FrameEnts[i]);

	gEngfuncs.pTriAPI->CullFace(TRI_NONE); //Warning: This must be reset after calling RenderModel.  HL changes the mode

	//Reset states

	if (m_Parent)
		SetCustomTextureSettings(false);

	EnableClippingPlane(false);

	//glDisable( GL_ALPHA_TEST );
	glDisable(GL_STENCIL_TEST);

	//Set render target back to HL framebuffer
	CRender::SetRenderTarget(false);

	dbg("Render Mirror within Mirror");

	int ChildMirrors = 0;
	if (!m_Parent && !m_Texture->Mirror.NoWorld)
		for (int m = 0; m < m_ChildMirrors.size(); m++)
		{
			CMirror &ChildMirror = *m_ChildMirrors[m];
			if (m_ChildMirrors[m] == this)
				continue;

			ChildMirror.m_Parent = this;
			ChildMirror.RenderMirroredWorld(RecurseCall + 1);
			ChildMirror.m_Parent = NULL;
			ChildMirrors++;
		}
	m_ChildMirrors.clearitems();

	//Draw the mirror onscreen
	if (!m_Parent)
	{
		SetupLighting();
		if (!Mirror.m_Texture->Mirror.NoWorld)
			Draw(RDR_TEXTURE | RDR_PROJECTIVE);
		else
			Draw(RDR_TEXTURE | RDR_PROJECTIVE /*|RDR_ALPHATEST*/);
	}

	//If child, Reset current mirror to parent
	if (m_Parent)
		CMirrorMgr::m_CurrentMirror = OldCurrentMirror;

	enddbg;
}

mleaf_t *FindLeaf(Vector &Origin, mnode_t *pNode)
{
	if (!pNode)
		return nullptr;

	mnode_t &Node = *pNode;

	if (Node.contents == CONTENTS_SOLID)
		return nullptr;

	if (Node.contents < 0)
	{
		//Found the resident Leaf
		return (mleaf_t *)pNode;
	}

	mplane_t &Plane = *Node.plane;

	float Dist = 0;

	switch (Plane.type)
	{
	case PLANE_X:
		Dist = Origin[0] - Plane.dist;
		break;
	case PLANE_Y:
		Dist = Origin[1] - Plane.dist;
		break;
	case PLANE_Z:
		Dist = Origin[2] - Plane.dist;
		break;
	default:
		Dist = DotProduct(Origin, Plane.normal) - Plane.dist;
		break;
	}

	int Side = (Dist >= 0) ? 0 : 1;

	for (int i = 0; i < 2; i++)
	{
		mleaf_t *pLeaf = FindLeaf(Origin, Node.children[(i == 0) ? Side : !Side]);
		if (pLeaf)
			return pLeaf;
	}

	return nullptr;
}

inline bool ParseLeaf(TraverseInfo_t &Info, mleaf_t &Leaf)
{
	bool Success = false;
	Vector Bounds[2] = {&Leaf.minmaxs[0], &Leaf.minmaxs[3]};
	bool CallFunc = Info.CheckFrustum ? CheckBBox(Bounds) : true;

	if (Info.CheckClipplane)
		CallFunc = Info.ClipPlane.BBoxIsInFront(Bounds);

	if (CallFunc)
		for (int s = 0; s < Leaf.nummarksurfaces; s++)
			if ((*(ParseSurfaceFunc *)Info.Func)(Info, Leaf.firstmarksurface[s]))
				Success = true;

	return Success;
}

bool ParseVisibleSurfaces(TraverseInfo_t &Info, mleaf_t *pStartLeaf)
{
	bool FoundOne = false;
	model_t &Model = *Info.pEntity->model;

	mleaf_t &StartLeaf = *pStartLeaf;
	int num_leaves = Model.numleafs;
	int visofs = 0; //Leaf.visofs;
	if (!StartLeaf.compressed_vis)
	{
		FoundOne = true;
		for (int c = 1; c <= num_leaves; c++)
		{
			mleaf_t &DrawLeaf = Model.leafs[c]; //c+1
			ParseLeaf(Info, DrawLeaf);
		}
		return true;
	}

	for (int c = 1; c <= num_leaves; visofs++)
		if (StartLeaf.compressed_vis[visofs] == 0)
			c += 8 * StartLeaf.compressed_vis[++visofs];
		else
			for (byte bit = 1; bit != 0; bit = (bit * 2) & (0xFF), c++)
				if (StartLeaf.compressed_vis[visofs] & bit)
				{
					int leafnum = c /*+1*/;
					mleaf_t &DrawLeaf = Model.leafs[leafnum];
					FoundOne = ParseLeaf(Info, DrawLeaf);
				}

	return FoundOne;
}

//Starts at a headnode, checks all leaves found in child nodes to see if any
//of those leaves can see the desired leaf
bool IsLeafVisible(cl_entity_t *pEntity, mnode_t *pNode, mleaf_t *pSearchLeaf)
{
	if (!pNode)
		return false;

	mnode_t &Node = *pNode;

	if (Node.contents == CONTENTS_SOLID)
		return false;

	if (Node.contents < 0)
	{
		//Search Leaf
		return IsLeafVisible(pEntity, (mleaf_t *)pNode, pSearchLeaf);
	}

	//Search child nodes
	for (int i = 0; i < 2; i++)
	{
		bool Found = IsLeafVisible(pEntity, Node.children[i], pSearchLeaf);
		if (Found)
			return Found;
	}

	return false;
}

bool Intersects(Vector Bounds[2], Vector OtherBounds[2])
{
	if (OtherBounds[0][0] > Bounds[1][0] ||
		OtherBounds[0][1] > Bounds[1][1] ||
		OtherBounds[0][2] > Bounds[1][2] ||
		OtherBounds[1][0] < Bounds[0][0] ||
		OtherBounds[1][1] < Bounds[0][1] ||
		OtherBounds[1][2] < Bounds[0][2])
		return false;
	return true;
}

bool IsLeafVisible(cl_entity_t *pEntity, mleaf_t *pStartLeaf, mleaf_t *pSearchLeaf)
{
	/*model_t &Model = *pEntity->model;
	mleaf_t &StartLeaf = *pStartLeaf;
	int num_leaves = Model.numleafs;
	int visofs = 0;//Leaf.visofs;
	if( !StartLeaf.compressed_vis )
		return true;

	Vector CheckBounds[2] = { &pSearchLeaf->minmaxs[0], &pSearchLeaf->minmaxs[3] };

	for (int c = 1; c <= num_leaves; visofs++)
		if(StartLeaf.compressed_vis[visofs] == 0)
			c += 8 * StartLeaf.compressed_vis[++visofs];
		else
			for (byte bit = 1; bit != 0; bit = (bit * 2) & (0xFF), c++)
			 if( StartLeaf.compressed_vis[visofs] & bit )
			 {
				int leafnum = c+1;
				mleaf_t *pVisLeaf = &Model.leafs[leafnum];
				Vector VisBounds[2] = { &pVisLeaf->minmaxs[0], &pVisLeaf->minmaxs[3] };
				if( Intersects( VisBounds, CheckBounds) )
					return true;
			 }*/

	return false;
}

bool IsBoundsVisible(cl_entity_t *pWorldEntity, mleaf_t *pStartLeaf, Vector Bounds[2])
{
	model_t &Model = *pWorldEntity->model;
	mleaf_t &StartLeaf = *pStartLeaf;
	int num_leaves = Model.numleafs;
	int visofs = 0; //Leaf.visofs;
	if (!StartLeaf.compressed_vis)
		return true;

	for (int c = 1; c <= num_leaves; visofs++)
		if (StartLeaf.compressed_vis[visofs] == 0)
			c += 8 * StartLeaf.compressed_vis[++visofs];
		else
			for (byte bit = 1; bit != 0; bit = (bit * 2) & (0xFF), c++)
				if (StartLeaf.compressed_vis[visofs] & bit)
				{
					int leafnum = c; //c+1;
					mleaf_t *pVisLeaf = &Model.leafs[leafnum];
					Vector VisBounds[2] = {&pVisLeaf->minmaxs[0], &pVisLeaf->minmaxs[3]};
					if (Intersects(VisBounds, Bounds))
						return true;
				}

	return false;
}

bool Mirror_DrawSurface(TraverseInfo_t &Info, msurface_t *pSurface)
{
	startdbg;

	//If it's the original glass texture, don't draw
	msurface_t &Surface = *pSurface;
	//if( Surface.texinfo->texture->gl_texturenum == Info.Mirror->GLIgnoreTexture )
	//	return false;

	//Don't draw polys on the same normal
	float PlaneMutliplier = (FBitSet(Surface.flags, SURF_PLANEBACK) ? -1 : 1);
	Vector SurfaceNormal = Surface.plane->normal * PlaneMutliplier;
	float SurfaceDist = Surface.plane->dist * PlaneMutliplier;

	if (SurfaceNormal == Info.Mirror->Normal && SurfaceDist == Info.Mirror->Dist)
		return false;

	if (FBitSet(Surface.flags, SURF_DRAWSKY | SURF_DRAWTURB | SURF_UNDERWATER))
		return false;

	CMirror &Mirror = *Info.Mirror;

	//Check if the polygon is part of a mirror.  If so, mark it as a child mirror for later

	glColor4f(1, 1, 1, 1);
	bool IsMirrorSurface = false;

	if (FBitSet(Surface.flags, (1 << 8)) && !Mirror.m_Parent)
	{
		//glPushAttrib( GL_STENCIL_BUFFER_BIT );
		glStencilFunc(GL_GEQUAL, Mirror.Frame_ChildLevel + 1, MIRROR_STENCIL_MASK);
		glStencilOp(GL_REPLACE, GL_KEEP, GL_REPLACE);
		IsMirrorSurface = true;

		for (int r = 0; r < CMirrorMgr::m_RenderSurfaces.size(); r++)
		{
			rendersurface_t &Rend = CMirrorMgr::m_RenderSurfaces[r];
			if (Rend.Surface != pSurface || Rend.Mirror == Info.Mirror)
				continue;

			bool MirrorAlreadyFound = false;
			for (int m = 0; m < Mirror.m_ChildMirrors.size(); m++)
				if (Mirror.m_ChildMirrors[m] == Rend.Mirror)
				{
					MirrorAlreadyFound = true;
					break;
				}

			if (MirrorAlreadyFound)
				break;

			Mirror.m_ChildMirrors.add(Rend.Mirror);
			break;
		}
	}

	//Draw surface
	glBindTexture(GL_TEXTURE_2D, Surface.texinfo->texture->gl_texturenum);

	CSurface DrawSurface(&Surface);
	DrawSurface.DrawNormal();

	if (IsMirrorSurface)
	{
		Mirror.SetStencil();
		//glPopAttrib( );
	}

	/*glBindTexture( GL_TEXTURE_2D, surf->lightmaptexturenum );
	glEnable( GL_BLEND );
	glBegin( GL_POLYGON );
	Vertex = Poly.verts[0];
	for( int t = 0; t < Poly.numverts; t++, Vertex += VERTEXSIZE )
	{
		glTexCoord2f( Vertex[5], Vertex[6] );
		glVertex3fv( Vertex );
	}
	glEnd ();*/
	enddbg;
	return true;
}

bool CMirrorMgr::Render_StudioModel(cl_entity_t *pEnt)
{
	if (m_CurrentMirror.Enabled)
	{
		CMirror &Mirror = *m_CurrentMirror.Mirror;
		if (Mirror.m_Texture->Mirror.NoEnts)
			return false; //Don't reflect any entities

		//if( FBitSet( pEnt->curstate.oldbuttons, MSRDR_NOREFLECT ) )
		//	return false;		//Don't reflect this entity

		if (Mirror.Frame_NoRender)
			return false; //Render got canceled for this frame
	}
	else
	{
		if (FBitSet(pEnt->curstate.oldbuttons, MSRDR_NOREFLECT))
			return true; //Don't reflect this entity, but still draw it (return true)

		m_FrameEnts.add(pEnt); //List of ents to reflect
	}

	return true;
}

void CMirror::EnableClippingPlane(bool Enable)
{
	if (Enable)
	{
		Vector &vNormal = !m_Parent ? Normal : Frame_Plane.m_Normal;
		Frame_ClipPlane.m_Normal = Vector(-vNormal.x, -vNormal.y, -vNormal.z);
		Frame_ClipPlane.m_Dist = !m_Parent ? Dist : Frame_Plane.m_Dist;

		//Clip
		double glplane[4] = {Frame_ClipPlane.m_Normal.x, Frame_ClipPlane.m_Normal.y, Frame_ClipPlane.m_Normal.z, Frame_ClipPlane.m_Dist};
		glClipPlane(GL_CLIP_PLANE0, glplane);
		glEnable(GL_CLIP_PLANE0);
	}
	else
	{
		glDisable(GL_CLIP_PLANE0);
	}
}

void CMirror::CreateMatrix()
{
	//Creates a matrix that reflects through an arbitrary plane

	//Find the angles for that plane
	//Find the delta required to align the angles to the Y axis (0,90,0)
	//Rotate the world by the delta, so that what's directly in front of the mirror is now straight down the Y axis
	//Flip the world across the Y axis
	//Rotate the world back, inverse of the delta

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	float mm[4][4], mi[4][4];

	//New mirror calculation

	Vector ang;
	VectorAngles(!m_Parent ? Normal : Frame_Plane.m_Normal, ang);
	if (Entity)
		ang += Entity->angles;

	//This matrix translates the world so that the mirror would be facing down the Y axis.
	AngleMatrix(Vector(/*-ang.z*/ 0, -(90 - ang.y), ang.x), mm);
	mm[3][0] = 0;
	mm[3][1] = 0;
	mm[3][2] = 0;
	mm[3][3] = 1;

	GenerateInverseMatrix4f(mm, mi);

	//Reflect along z axis
	static float my[4][4] =
		{
			1,
			0,
			0,
			0,
			0,
			-1,
			0,
			0,
			0,
			0,
			1,
			0,
			0,
			/*Dynamic Dist Value*/ 0,
			0,
			1,
		};

	my[3][1] = (!m_Parent ? Dist : Frame_Plane.m_Dist) * 2;

	glLoadIdentity();
	glMultMatrixf((float *)mi); //Rotate back to normal
	glMultMatrixf((float *)my); //Flip across Y axis
	glMultMatrixf((float *)mm); //Align World to Y axis

	glGetFloatv(GL_MODELVIEW_MATRIX, !m_Parent ? (float *)MirrorMatrix : (float *)Frame_ReflectMatrix);

	glPopMatrix();
}

void CMirror::SetStencil()
{
	//All world surfaces and entities to update the stencil buffer
	//Any child mirrors conform to the parent's stencil areas
	//Remember: Passes if ( ref & mask) OPERATOR ( stencil & mask)
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_LEQUAL, Frame_ChildLevel, MIRROR_STENCIL_MASK);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
}

void CMirror::ApplyTransformation()
{
	if (!m_Parent)
		glMultMatrixf((float *)MirrorMatrix);
	else
	{
		glMultMatrixf((float *)Frame_ReflectMatrix);
		glMultMatrixf((float *)m_Parent->MirrorMatrix);
	}
}

void CMirror::BindTexture()
{
	glBindTexture(GL_TEXTURE_2D, GLBufferTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	CRender::RT_BindTexture();
}
void CMirror::ReleaseTexture()
{
	CRender::RT_ReleaseTexture();
}

void CMirror::SetCustomTextureSettings(bool Enable)
{
	if (Enable)
	{
		mstexture_t &Tex = *m_Texture;
		glColor4fv(Tex.Mirror.Color);

		if (Tex.Mirror.Blending)
		{
			//glEnable( GL_ALPHA_TEST );
			//glAlphaFunc( GL_EQUAL, 1 );
			glDisable(GL_DEPTH);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
	}
	else
	{
		glDisable(GL_BLEND);
	}
}

void CMirror::Draw(int Flags)
{
	float OldProjMatrix[16];
	int oldDepthFunc = GL_LEQUAL;

	if (FBitSet(Flags, RDR_PROJECTIVE))
	{
		float mp[16], mm[16];
		glGetFloatv(GL_PROJECTION_MATRIX, mp);
		glGetFloatv(GL_MODELVIEW_MATRIX, mm);

		// Select and save the texture matrix
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();

		// Generate the perspective projection and range remapping matrix
		glLoadIdentity();

		//~~Important~~
		//The basic calculation for Projective Texture Mapping is Scale 0.5 then add 0.5 as shown:
		//glTranslatef( 0.5f, 0.5f, 0.5f );			//Bulids a matrix which will scale by 0.5, then add
		//glScalef( 0.5f, 0.5f , 0.5f );			//0.5.  Yes, the translatef comes first - otherwise the translation gets scaled too

		//I had to modify this equation to take into acount that my texures are generally going to
		//have extra space (For compatibility, I force them to be 2^x, but I dont use all the space).
		//So below is the modified equation

		float RatioW = (float)ScreenWidth / TexSize.x;
		float RatioH = (float)ScreenHeight / TexSize.y;
		//glTranslatef( 0.5f * RatioW, 0.5f * (1-(1-RatioH)), 0.0f );
		glTranslatef(0.5f * RatioW, 0.5f * RatioH, 0.0f);
		glScalef(0.5f * RatioW, 0.5f * RatioH, 1);

		glMultMatrixf(mp);
		glMultMatrixf(mm);

		SetBits(Flags, RDR_TEXTURE);
	}

	if (FBitSet(Flags, RDR_TEXTURE))
	{
		SetCustomTextureSettings(true);
		BindTexture(); //Texture containing the mirrored screen
	}
	else
	{
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
	}

	if (FBitSet(Flags, RDR_ALPHATEST))
	{
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_EQUAL, 1);
	}

	if (FBitSet(Flags, RDR_CLEARZ)) //Clear the Z buffer only within the surface area
	{
		glGetFloatv(GL_PROJECTION_MATRIX, OldProjMatrix);

		static float ClearZMatrix[4][4] =
			{
				1,
				0,
				0,
				0,
				0,
				1,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				0,
				1,
				1,
			};

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadMatrixf((GLfloat *)ClearZMatrix);
		glMultMatrixf((GLfloat *)OldProjMatrix);

		glGetIntegerv(GL_DEPTH_FUNC, &oldDepthFunc);
		glDepthFunc(GL_ALWAYS);
	}

	if (FBitSet(Flags, RDR_CLRALPHA)) //Clear the alpha buffer within the surface area
	{
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
		glColor4f(0, 0, 1, 0);
	}

	if (FBitSet(Flags, RDR_STENCIL))
	{
		//-- unused
		/*glEnable( GL_STENCIL_TEST );
        //glStencilFunc( GL_NEVER, 2, 2 );
        glStencilFunc( GL_ALWAYS, 1, 1 );
        glStencilOp( GL_REPLACE, GL_REPLACE, GL_REPLACE );*/
	}

	if (FBitSet(Flags, RDR_CLRSTENCIL)) //Clear the stencil buffer within the surface area
	{
		glEnable(GL_STENCIL_TEST);
		glStencilFunc(GL_ALWAYS, 0, MIRROR_STENCIL_MASK);
		glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
	}

	if (FBitSet(Flags, RDR_CHKSTENCIL)) //Only clear the Z if within a certain stencil'd area
	{
		glEnable(GL_STENCIL_TEST);
		glStencilFunc(GL_LEQUAL, Frame_ChildLevel, MIRROR_STENCIL_MASK);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	}

	if (m_Parent)
	{
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glMultMatrixf((float *)m_Parent->MirrorMatrix);
	}

	for (int s = 0; s < m_Surfaces.size(); s++)
	{
		//if( !Mirror.Surfaces[s]->visframe )//!= Mirror.Entity->model->nodes[0].visframe )
		//	continue;
		/*surfaceinfo_t &SurfInfo = Mirror.SurfaceInfo[s];
		if( !CheckBBox( SurfInfo.Origin, SurfInfo.Bounds ) )
			continue;*/

		//gEngfuncs.pTriAPI->CullFace( TRI_NONE );
		//glDisable( GL_DEPTH_TEST );
		CSurface &Surface = m_Surfaces[s];
		Surface.m_SurfaceNormal = Normal;
		if (FBitSet(Flags, RDR_PROJECTIVE))
			Surface.DrawProjective();
		else
			Surface.DrawNormal();

		//glEnable( GL_DEPTH_TEST );
	}

	//Restore
	//=======

	if (m_Parent)
	{
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}

	if (FBitSet(Flags, RDR_TEXTURE))
	{
		SetCustomTextureSettings(false);
		ReleaseTexture();
	}
	else
	{
		glEnable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}

	if (FBitSet(Flags, RDR_PROJECTIVE))
	{
		glMatrixMode(GL_TEXTURE); //restore
		glPopMatrix();
	}

	if (FBitSet(Flags, RDR_ALPHATEST))
	{
		glDisable(GL_ALPHA_TEST);
	}

	if (FBitSet(Flags, RDR_CLEARZ))
	{
		glMatrixMode(GL_PROJECTION); //restore
		glPopMatrix();
		glDepthFunc(oldDepthFunc);
	}

	if (FBitSet(Flags, RDR_CLRALPHA))
	{
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}

	if (FBitSet(Flags, RDR_STENCIL) || FBitSet(Flags, RDR_CLRSTENCIL) || FBitSet(Flags, RDR_CHKSTENCIL))
	{
		glDisable(GL_STENCIL_TEST);
	}
}

//
// CSurface - Adds funtionality the bsp struct
//
void CSurface::Draw()
{
	glpoly_t &Poly = *m_Surface->polys;
	glBegin(GL_POLYGON);

	//this loops backwards, for backfaces
	//float *Vertex = Poly.verts[0] + (Poly.numverts-1) * VERTEXSIZE;
	//for( int t = Poly.numverts-1; t >= 0 ; t--, Vertex -= VERTEXSIZE )

	float *Vertex = Poly.verts[0];
	for (int t = 0; t < Poly.numverts; t++, Vertex += VERTEXSIZE)
	{
		//Vector Vec = Vector(Vertex) + m_SurfaceNormal * 5;
		//Set Texture coordinates
		if (m_ModeTexCoord == TEXCOORD_3D)
			glTexCoord3fv(Vertex); //3D coords - Projective texturing
		else
			glTexCoord2f(Vertex[3], Vertex[4]); //2D coords - Normal mapped texureing

		//Set Vertex
		glVertex3fv(Vertex);
	}
	glEnd();
}
void CSurface::DrawProjective()
{
	m_ModeTexCoord = TEXCOORD_3D;
	Draw();
}
void CSurface::DrawNormal()
{
	m_ModeTexCoord = TEXCOORD_2D;
	Draw();
}

float Determinant4f(const float m[16])
{
	return m[12] * m[9] * m[6] * m[3] -
		   m[8] * m[13] * m[6] * m[3] -
		   m[12] * m[5] * m[10] * m[3] +
		   m[4] * m[13] * m[10] * m[3] +
		   m[8] * m[5] * m[14] * m[3] -
		   m[4] * m[9] * m[14] * m[3] -
		   m[12] * m[9] * m[2] * m[7] +
		   m[8] * m[13] * m[2] * m[7] +
		   m[12] * m[1] * m[10] * m[7] -
		   m[0] * m[13] * m[10] * m[7] -
		   m[8] * m[1] * m[14] * m[7] +
		   m[0] * m[9] * m[14] * m[7] +
		   m[12] * m[5] * m[2] * m[11] -
		   m[4] * m[13] * m[2] * m[11] -
		   m[12] * m[1] * m[6] * m[11] +
		   m[0] * m[13] * m[6] * m[11] +
		   m[4] * m[1] * m[14] * m[11] -
		   m[0] * m[5] * m[14] * m[11] -
		   m[8] * m[5] * m[2] * m[15] +
		   m[4] * m[9] * m[2] * m[15] +
		   m[8] * m[1] * m[6] * m[15] -
		   m[0] * m[9] * m[6] * m[15] -
		   m[4] * m[1] * m[10] * m[15] +
		   m[0] * m[5] * m[10] * m[15];
}

void GenerateInverseMatrix4f(const float inMatrix[4][4], float outInverse[4][4])
{
	float x = Determinant4f(&inMatrix[0][0]);
	const float *m = &inMatrix[0][0];
	float *i = &outInverse[0][0];

	i[0] = (-m[13] * m[10] * m[7] + m[9] * m[14] * m[7] + m[13] * m[6] * m[11] - m[5] * m[14] * m[11] - m[9] * m[6] * m[15] + m[5] * m[10] * m[15]) / x;
	i[4] = (m[12] * m[10] * m[7] - m[8] * m[14] * m[7] - m[12] * m[6] * m[11] + m[4] * m[14] * m[11] + m[8] * m[6] * m[15] - m[4] * m[10] * m[15]) / x;
	i[8] = (-m[12] * m[9] * m[7] + m[8] * m[13] * m[7] + m[12] * m[5] * m[11] - m[4] * m[13] * m[11] - m[8] * m[5] * m[15] + m[4] * m[9] * m[15]) / x;
	i[12] = (m[12] * m[9] * m[6] - m[8] * m[13] * m[6] - m[12] * m[5] * m[10] + m[4] * m[13] * m[10] + m[8] * m[5] * m[14] - m[4] * m[9] * m[14]) / x;
	i[1] = (m[13] * m[10] * m[3] - m[9] * m[14] * m[3] - m[13] * m[2] * m[11] + m[1] * m[14] * m[11] + m[9] * m[2] * m[15] - m[1] * m[10] * m[15]) / x;
	i[5] = (-m[12] * m[10] * m[3] + m[8] * m[14] * m[3] + m[12] * m[2] * m[11] - m[0] * m[14] * m[11] - m[8] * m[2] * m[15] + m[0] * m[10] * m[15]) / x;
	i[9] = (m[12] * m[9] * m[3] - m[8] * m[13] * m[3] - m[12] * m[1] * m[11] + m[0] * m[13] * m[11] + m[8] * m[1] * m[15] - m[0] * m[9] * m[15]) / x;
	i[13] = (-m[12] * m[9] * m[2] + m[8] * m[13] * m[2] + m[12] * m[1] * m[10] - m[0] * m[13] * m[10] - m[8] * m[1] * m[14] + m[0] * m[9] * m[14]) / x;
	i[2] = (-m[13] * m[6] * m[3] + m[5] * m[14] * m[3] + m[13] * m[2] * m[7] - m[1] * m[14] * m[7] - m[5] * m[2] * m[15] + m[1] * m[6] * m[15]) / x;
	i[6] = (m[12] * m[6] * m[3] - m[4] * m[14] * m[3] - m[12] * m[2] * m[7] + m[0] * m[14] * m[7] + m[4] * m[2] * m[15] - m[0] * m[6] * m[15]) / x;
	i[10] = (-m[12] * m[5] * m[3] + m[4] * m[13] * m[3] + m[12] * m[1] * m[7] - m[0] * m[13] * m[7] - m[4] * m[1] * m[15] + m[0] * m[5] * m[15]) / x;
	i[14] = (m[12] * m[5] * m[2] - m[4] * m[13] * m[2] - m[12] * m[1] * m[6] + m[0] * m[13] * m[6] + m[4] * m[1] * m[14] - m[0] * m[5] * m[14]) / x;
	i[3] = (m[9] * m[6] * m[3] - m[5] * m[10] * m[3] - m[9] * m[2] * m[7] + m[1] * m[10] * m[7] + m[5] * m[2] * m[11] - m[1] * m[6] * m[11]) / x;
	i[7] = (-m[8] * m[6] * m[3] + m[4] * m[10] * m[3] + m[8] * m[2] * m[7] - m[0] * m[10] * m[7] - m[4] * m[2] * m[11] + m[0] * m[6] * m[11]) / x;
	i[11] = (m[8] * m[5] * m[3] - m[4] * m[9] * m[3] - m[8] * m[1] * m[7] + m[0] * m[9] * m[7] + m[4] * m[1] * m[11] - m[0] * m[5] * m[11]) / x;
	i[15] = (-m[8] * m[5] * m[2] + m[4] * m[9] * m[2] + m[8] * m[1] * m[6] - m[0] * m[9] * m[6] - m[4] * m[1] * m[10] + m[0] * m[5] * m[10]) / x;
}
