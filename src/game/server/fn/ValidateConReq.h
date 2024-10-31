//
// Validate if FN connectivity could be established!
//

#ifndef HTTP_VALIDATE_CONNECTIVITY_REQUEST_H
#define HTTP_VALIDATE_CONNECTIVITY_REQUEST_H

#include "HTTPRequest.h"

class ValidateConnectivityRequest : public HTTPRequest
{
public:
	ValidateConnectivityRequest(const char* url);
	void OnResponse(bool bSuccessful, int iRespCode);
	const char* GetName() { return "ValidateConnectivityRequest"; }

private:
	ValidateConnectivityRequest(const ValidateConnectivityRequest&);
};

#endif // HTTP_VALIDATE_CONNECTIVITY_REQUEST_H