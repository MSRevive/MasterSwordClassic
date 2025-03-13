//
// Validate if FN connectivity could be established!
//

#ifndef HTTP_VALIDATE_CON_REQUEST_H
#define HTTP_VALIDATE_CON_REQUEST_H

#include "HTTPRequest.h"

class ValidateConRequest : public HTTPRequest
{
public:
	ValidateConRequest(const char* url);
	void OnResponse(bool bSuccessful, int iRespCode);
	const char* GetName() { return "ValidateConRequest"; }

private:
	ValidateConRequest(const ValidateConRequest&);
};

#endif // HTTP_VALIDATE_CONNECTIVITY_REQUEST_H