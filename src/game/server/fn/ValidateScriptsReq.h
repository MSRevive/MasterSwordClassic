//
// Validate sc.dll
//

#ifndef HTTP_VALIDATE_SCRIPTS_REQUEST_H
#define HTTP_VALIDATE_SCRIPTS_REQUEST_H

#include "HTTPRequest.h"

class ValidateScriptsRequest : public HTTPRequest
{
public:
	ValidateScriptsRequest(const char* url);
	void OnResponse(bool bSuccessful, int iRespCode);
	const char* GetName() { return "ValidateScriptsRequest"; }

private:
	ValidateScriptsRequest(const ValidateScriptsRequest&);
};

#endif // HTTP_VALIDATE_SCRIPTS_REQUEST_H