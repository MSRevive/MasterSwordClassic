//
// Create a new FN character
//

#include "rapidjson/document.h"
#include "CreateCharacterReq.h"
#include "FNSharedDefs.h"
#include "msdllheaders.h"
#include "player.h"
#include "util.h"

CreateCharacterRequest::CreateCharacterRequest(ID64 steamID, ID64 slot, const char* url, const char* body, size_t bodySize) :
	HTTPRequest(HTTPMethod::POST, url, body, bodySize, steamID, slot)
{
}

void CreateCharacterRequest::OnResponse(bool bSuccessful, JSONDocument* jsonDoc, int iRespCode)
{
	if (bSuccessful == false)
		FNShared::Print("Unable to create character for SteamID %llu!\n", m_iSteamID64);

	CBasePlayer* pPlayer = UTIL_PlayerBySteamID(m_iSteamID64);
	if (pPlayer == NULL)
		return;

	charinfo_t& CharInfo = pPlayer->m_CharInfo[m_iSlot];

	if (bSuccessful == false)
	{
		CharInfo.Index = m_iSlot;
		CharInfo.Location = LOC_CENTRAL;
		CharInfo.Status = CDS_NOTFOUND;
		CharInfo.m_CachedStatus = CDS_UNLOADED; // force an update!
		return;
	}
	
	JSONDocument& doc = (*jsonDoc);
	const int flags = doc["data"]["flags"].GetInt();

	CharInfo.AssignChar(m_iSlot, LOC_CENTRAL, (char*)m_sRequestBody, m_iRequestBodySize, pPlayer);
	strncpy(CharInfo.Guid, doc["data"]["id"].GetString(), MSSTRING_SIZE);
	CharInfo.Flags = flags;
	CharInfo.Status = CDS_LOADED;
	CharInfo.m_CachedStatus = CDS_UNLOADED; // force an update!
}