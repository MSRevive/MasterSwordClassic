//
// Validate sc.dll
//

#include "rapidjson/document.h"
#include "ValidateScriptsReq.h"
#include "FNSharedDefs.h"
#include "msdllheaders.h"
#include "global.h"

ValidateScriptsRequest::ValidateScriptsRequest(const char* url) :
	HTTPRequest(HTTPMethod::GET, url)
{
}

void ValidateScriptsRequest::OnResponse(bool bSuccessful, int iRespCode)
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
		FNShared::Print("Script file not verified for FN!\n");
	}

	FNShared::Print("Scripts verified for FN.\n");
}