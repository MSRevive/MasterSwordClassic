#include "findentities.h"
#ifdef VALVE_DLL
#include "cbase.h"
#define MAX_SEARCH_ENTITIES gpGlobals->maxEntities
#else
#include "inc_huditem.h"
#include "movement/pm_defs.h"
#define MAX_SEARCH_ENTITIES 1500 // TODO
#endif

int UtilFindEntities(mslist<CFindEntity>& rFound, const mslist<CEntityFilter *>& vFilters, CFindEntity::YWrapType * pIgnoreEntity, int vMax)
{
	rFound.clear();
	int vNumFound = 0;
	bool bValid;
	bool bFilterFail;
	bool bCheckMax = vMax > 0;
	for (int i = 0; i < MAX_SEARCH_ENTITIES; i++)
	{
		CFindEntity vEntity = CFindEntity::GetEntity(i, bValid);
		if (!bValid || vEntity.GetWrapped() == pIgnoreEntity) continue;

		bFilterFail = false;
		for (int j = 0; j < vFilters.size(); j++)
		{
			if (!vFilters[j]->Allow(vEntity))
			{
				bFilterFail = true;
				break;
			}
		}

		if (bFilterFail) continue;

		rFound.push_back(vEntity);
		++vNumFound;
		if (bCheckMax)
		{
			if (vNumFound >= vMax)
			{
				break;
			}
		}
	}
	return vNumFound;
}

CFindEntity::CFindEntity(CFindEntity::YWrapType * pEntity)
{
	mpEntity = pEntity;
}

CFindEntity::YWrapType *CFindEntity::GetWrapped()
{
	return mpEntity;
}

CScriptFilter::CScriptFilter(IScripted * pScripted, const msstring & vsEventName)
{
	mpScripted = pScripted;
	msEventName = vsEventName;
}

bool CScriptFilter::Allow(CFindEntity & vEntity)
{
	msstringlist                        vParams;
	vParams.add(vEntity.AsString());
	mpScripted->CallScriptEvent(msEventName, &vParams);
	return atoi(mpScripted->m_ReturnData) == 1;
}

bool CEntityTypeFilter::Allow(CFindEntity & vEntity) const
{
	if (vEntity.IsPlayer())
	{
		return mbAllowPlayer;
	}
	else
#ifdef VALVE_DLL
	if (vEntity.IsMSItem())
	{
		return mbAllowItem;
	}
	else
	if ( vEntity.IsMSMonster() )
	{
		return mbAllowNpc;
	}
#else
		return !mbAllowPlayer;
#endif

	return false;
}

CTraceLineFilter::CTraceLineFilter(const Vector& vOrigin)
{
#ifdef VALVE_DLL
	mpIgnoreEntity = NULL;
#endif
	mOrigin = vOrigin;
}

#ifdef VALVE_DLL
CTraceLineFilter::CTraceLineFilter(CFindEntity::YWrapType * pEntity, const Vector& vOrigin)
{
	mpIgnoreEntity = pEntity;
	mOrigin = vOrigin;
}
#endif

bool CTraceLineFilter::Allow(CFindEntity &vEntity) const
{
#ifdef VALVE_DLL
	TraceResult tr;
	UTIL_TraceLine(mOrigin, vEntity.Center(), ignore_monsters, mpIgnoreEntity ? mpIgnoreEntity->edict() : NULL, &tr);
	return tr.flFraction == 1;
#else
	pmtrace_s pTrace = *gEngfuncs.PM_TraceLine((float *)&mOrigin, (float *)&vEntity.Center(), PM_TRACELINE_PHYSENTSONLY, 2, ignore_monsters);
	return pTrace.fraction == 1;
#endif
}

CShapeFilter::CShapeFilter()
{
	mpTraceLine = NULL;
	ResetStringCache();
}

CShapeFilter::~CShapeFilter()
{
	if (mpTraceLine) 
		delete mpTraceLine;
}

void CShapeFilter::ResetStringCache()
{
	msAsString = "";
}

const msstring& CShapeFilter::AsString()
{
	if (!msAsString.len())
	{
		CreateStringCache();
	}
	return msAsString;
}

const Vector& CShapeFilter::GetOrigin()
{
	return mOrigin;
}

bool CShapeFilter::Allow(CFindEntity & vEntity) const
{
	Vector vEntityCenter = vEntity.Center();
	bool bAllow = Allow(vEntityCenter);
	if (mpTraceLine)
	{
		bAllow = mpTraceLine->Allow(vEntity);
	}
	return bAllow;
}

CShapeFilter *CShapeFilter::CreateFromString(const msstring & vsShape, bool  bTraceLine, CBaseEntity * pIgnoreEntity)
{
	CShapeFilter * pShape = NULL;
	if (vsShape.starts_with("Rec"))
	{
		pShape = new CRectangleFilter();
	}
	else
	if (vsShape.starts_with("Sphere"))
	{
		pShape = new CSphereFilter();
	}
	else
	if (vsShape.starts_with("Cyl"))
	{
		pShape = new CCylinderFilter();
	}

	try
	{
		if (pShape)
		{
			pShape->SetFromString(vsShape);
			pShape->SetTraceLine(new CTraceLineFilter(
#ifdef VALVE_DLL
				pIgnoreEntity,
#endif
				pShape->GetOrigin()
			));
		}
	}
	catch (...)
	{
		delete pShape;
		throw;
	}
	return pShape;
}

void CShapeFilter::SetTraceLine(CTraceLineFilter * pTraceLineFilter)
{
	if (mpTraceLine) 
		delete mpTraceLine;
		
	mpTraceLine = pTraceLineFilter;
}

CRectangleFilter::CRectangleFilter()
{
	mOrigin = Vector(0, 0, 0);
	mXSize = 0;
	mYSize = 0;
	mZSize = 0;
	mbCalculatedPoints = false;
	mbCheckZ = false;
}

CRectangleFilter::CRectangleFilter(const Vector& vOrigin, float vXSize, float vYSize, float vZSize)
{
	mOrigin = vOrigin;
	mXSize = vXSize;
	mYSize = vYSize;
	mZSize = vZSize;
	mbCalculatedPoints = false;
	mbCheckZ = false;
}

bool CRectangleFilter::Allow(const Vector& vOrigin) const
{
	CalcPoints();
	return vOrigin.x > mMinPoint.x
		&& vOrigin.x < mMaxPoint.x
		&& vOrigin.y > mMinPoint.y
		&& vOrigin.y < mMaxPoint.y
		&& (!mbCheckZ || (vOrigin.z > mMinPoint.z && vOrigin.z < mMaxPoint.z));
}

void CRectangleFilter::CalcPoints() const
{
	if (!mbCalculatedPoints)
	{
		mbCalculatedPoints = true;
		Vector vDelta(mXSize, mYSize, mZSize);
		mMinPoint = mOrigin - vDelta;
		mMaxPoint = mOrigin + vDelta;
		mbCheckZ = mZSize != 0;
	}
}

void CRectangleFilter::CreateStringCache() const
{
	msAsString = UTIL_VarArgs("Rec(%s,%.2f,%.2f,%.2f)", VecToString(mOrigin), mXSize, mYSize, mZSize);
}

bool CRectangleFilter::SetFromString(msstring vsString)
{
	bool bOk = false;
	Vector vOrigin;
	float vXSize;
	float vYSize;
	float vZSize;
	bOk = sscanf(vsString.c_str(), "Rec3D((%f,%f,%f),%f,%f,%f)", &vOrigin.x, &vOrigin.y, &vOrigin.z, &vXSize, &vYSize, &vZSize) == 6;

	if (bOk)
	{
		mOrigin = vOrigin;
		mXSize = vXSize;
		mYSize = vYSize;
		mZSize = vZSize;
		mbCalculatedPoints = false;
		mbCheckZ = false;
	}

	return bOk;
}

CSphereFilter::CSphereFilter()
{
	mRadius = 0;
	mRadiusSquared = 0;
}

CSphereFilter::CSphereFilter(const Vector& vOrigin, float vRadius, float vRadiusSquared)
{
	mOrigin = vOrigin;
	mRadius = vRadius;
	if (vRadiusSquared == -1)
	{
		vRadiusSquared = vRadius * vRadius;
	}
	mRadiusSquared = vRadiusSquared;
}


bool CSphereFilter::Allow(const Vector& vPoint) const
{
	float x = vPoint.x - mOrigin.x;
	float y = vPoint.y - mOrigin.y;
	float z = vPoint.z - mOrigin.z;

	float vDistSquared = (x*x) + (y*y) + (z*z);
	return vDistSquared <= mRadiusSquared;
}

void CSphereFilter::CreateStringCache() const
{
	msAsString = UTIL_VarArgs("Sphere(%s,%.2f,%.2f)", VecToString(mOrigin), mRadius, mRadiusSquared);
}

bool CSphereFilter::SetFromString(msstring vsString)
{
	Vector vOrigin;
	float vRadius;
	float vRadiusSquared;
	bool bOk = sscanf(vsString.c_str(), "Sphere((%f,%f,%f),%f,%f)", &vOrigin.x, &vOrigin.y, &vOrigin.z, &vRadius, &vRadiusSquared) == 5;
	if (bOk)
	{
		mOrigin = vOrigin;
		mRadius = vRadius;
		mRadiusSquared = vRadiusSquared;
	}

	return bOk;
}

CCylinderFilter::CCylinderFilter()
{
	mOrigin = Vector(0, 0, 0);
	mRadius = 0;
	mRadiusSquared = 0;
	mPosZ = 0;
	mNegZ = 0;
	mbCheckHeight = false;
}

CCylinderFilter::CCylinderFilter(Vector vOrigin, float vRadius, float vPosZ, float vNegZ)
{
	mOrigin = vOrigin;
	mRadius = vRadius;
	mPosZ = vPosZ;
	mNegZ = vNegZ;
	mbCheckHeight = vPosZ || vNegZ;
}

bool CCylinderFilter::Allow(const Vector& vPoint) const
{
	float x = vPoint.x - mOrigin.x;
	float y = vPoint.y - mOrigin.y;
	float vDistSquared = (x*x) + (y*y);

	if (vDistSquared >= mRadiusSquared) return false;
	if (mbCheckHeight && ((vPoint.z > (mOrigin.z + mPosZ)) || (vPoint.z < (mOrigin.z - mNegZ))))
	{
		return false;
	}
	return true;
}

void CCylinderFilter::CreateStringCache() const
{
	msAsString = UTIL_VarArgs("Cyl(%s,%.2f,%.2f,%.2f,%.2f)", VecToString(mOrigin), mRadius, mRadiusSquared, mPosZ, mNegZ);
}

bool CCylinderFilter::SetFromString(msstring vsString)
{
	Vector vOrigin;
	float vRadius;
	float vRadiusSquared;
	float vPosZ;
	float vNegZ;
	bool bOk = sscanf(vsString.c_str(), "Cyl((%f,%f,%f),%f,%f,%f,%f)", &vOrigin, &vRadius, &vRadiusSquared, &vPosZ, &vNegZ) == 7;
	if (bOk)
	{
		mOrigin = vOrigin;
		mRadius = vRadius;
		mRadiusSquared = vRadiusSquared;
		mPosZ = vPosZ;
		mNegZ = vNegZ;
	}

	return bOk;
}

Vector CFindEntity::Center()
{
	return (GetAbsMax() + GetAbsMin()) * 0.5;
}
