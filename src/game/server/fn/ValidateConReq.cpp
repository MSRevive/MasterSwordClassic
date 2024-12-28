//
// Validate if FN connectivity could be established!
//

#include "rapidjson/document.h"
#include "ValidateConReq.h"
#include "FNSharedDefs.h"
#include "msdllheaders.h"
#include "global.h"

ValidateConRequest::ValidateConRequest(const char* url) :
	HTTPRequest(HTTPMethod::GET, url)
{
}

void ValidateConRequest::OnResponse(bool bSuccessful, int iRespCode)
{
	return;
}