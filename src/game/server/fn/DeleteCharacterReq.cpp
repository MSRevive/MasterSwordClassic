//
// Delete FN character
//

#include "rapidjson/document.h"
#include "DeleteCharacterReq.h"
#include "FNSharedDefs.h"
#include "msdllheaders.h"
#include "player.h"
#include "util.h"

DeleteCharacterRequest::DeleteCharacterRequest(ID64 steamID, ID64 slot, const char* url) :
	HTTPRequest(HTTPMethod::DEL, url, NULL, NULL, steamID, slot)
{
}

void DeleteCharacterRequest::OnResponse(bool bSuccessful, int iRespCode)
{
	if (bSuccessful == false)
	{
		FNShared::Print("Unable to delete character for SteamID %llu!\n", m_iSteamID64);
		return;
	}

	CBasePlayer* pPlayer = UTIL_PlayerBySteamID(m_iSteamID64);
	if (pPlayer == NULL)
		return;

	charinfo_t& CharInfo = pPlayer->m_CharInfo[m_iSlot];
	CharInfo.Status = CDS_NOTFOUND;
	CharInfo.m_CachedStatus = CDS_UNLOADED; // force an update!
}