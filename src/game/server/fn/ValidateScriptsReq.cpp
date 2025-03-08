//
// Validate pak0.pak
//

#include "rapidjson/document.h"
#include "ValidateScriptsReq.h"
#include "FNSharedDefs.h"
#include "msdllheaders.h"
#include "global.h"

ValidateScriptsRequest::ValidateScriptsRequest(const char* url) :
	HTTPRequest(EHTTPMethod::k_EHTTPMethodGET, url)
{
}

void ValidateScriptsRequest::OnResponse(bool bSuccessful, int iRespCode)
{
	if (bSuccessful == false || pJSONData == NULL)
	{
		// MSGlobals::CentralEnabled = false;
		// FNShared::Print("FuzzNet has been disabled!\n");
		return;
	}

	const JSONValue& value = (*pJSONData);
	if (!value["data"].GetBool())
	{
		FNShared::Print("Script file not verified for FN!\n");
		MSGlobals::CentralEnabled = false;
	}

	FNShared::Print("Scripts verified for FN.\n");
}