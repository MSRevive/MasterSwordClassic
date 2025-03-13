//
// Verify if a map is eligible for FN play
//

#include "rapidjson/document.h"
#include "ValidateMapReq.h"
#include "FNSharedDefs.h"
#include "msdllheaders.h"
#include "global.h"

ValidateMapRequest::ValidateMapRequest(const char* url) :
	HTTPRequest(HTTPMethod::GET, url)
{
}

void ValidateMapRequest::OnResponse(bool bSuccessful, int iRespCode)
{
	if (bSuccessful == false)
	{
		// MSGlobals::CentralEnabled = false;
		// FNShared::Print("FuzzNet has been disabled!\n");
		return;
	}

	JSONDocument& doc = (*m_JSONResponse);
	if (!doc["data"].GetBool())
	{
		FNShared::Print("Map '%s' is not verified for FN!\n", MSGlobals::MapName.c_str());
	}

	FNShared::Print("Map '%s' verified for FN.\n", MSGlobals::MapName.c_str());
}