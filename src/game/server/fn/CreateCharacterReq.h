//
// Create a new FN character
//

#ifndef HTTP_CREATE_CHARACTER_REQUEST_H
#define HTTP_CREATE_CHARACTER_REQUEST_H

#include "HTTPRequest.h"

class CreateCharacterRequest : public HTTPRequest
{
public:
	CreateCharacterRequest(ID64 steamID, ID64 slot, const char* url, byte* body, size_t bodySize);
	void OnResponse(bool bSuccessful, JSONDocument* jsonDoc, int iRespCode);
	const char* GetName() { return "CreateCharacterRequest"; }

private:
	CreateCharacterRequest(const CreateCharacterRequest&);
};

#endif // HTTP_CREATE_CHARACTER_REQUEST_H