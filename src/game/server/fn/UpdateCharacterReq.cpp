//
// Update FN character
//

#include "rapidjson/document.h"
#include "UpdateCharacterReq.h"
#include "FNSharedDefs.h"
#include "msdllheaders.h"
#include "player.h"
#include "util.h"

UpdateCharacterRequest::UpdateCharacterRequest(ID64 steamID, ID64 slot, const char* url, byte* body, size_t bodySize) :
	HTTPRequest(HTTPMethod::PUT, url, body, bodySize, steamID, slot)
{
}

void UpdateCharacterRequest::OnResponse(bool bSuccessful, JSONDocument* jsonDoc, int iRespCode)
{
}