//
// Load FN character
//

#include "rapidjson/document.h"
#include "base64/base64.h"
#include "LoadCharacterReq.h"
#include "FNSharedDefs.h"
#include "msdllheaders.h"
#include "player.h"
#include "util.h"

LoadCharacterRequest::LoadCharacterRequest(ID64 steamID, ID64 slot, const char* url) :
	HTTPRequest(HTTPMethod::GET, url, NULL, NULL, steamID, slot)
{
}

void LoadCharacterRequest::OnResponse(bool bSuccessful, JSONDocument* jsonDoc, int iRespCode)
{
	if (bSuccessful == false)
	{
		FNShared::Print("Unable to load character %i for SteamID %llu!\n", (m_iSlot + 1), m_iSteamID64);
		return;
	}

	CBasePlayer* pPlayer = UTIL_PlayerBySteamID(m_iSteamID64);
	if (pPlayer == nullptr)
	{
		FNShared::Print("FATALITY: Unable to get player with SteamID64 %llu\n", m_iSteamID64);
		return;
	}

	charinfo_t& CharInfo = pPlayer->m_CharInfo[m_iSlot];

	if (iRespCode == 204)
	{
		CharInfo.Index = m_iSlot;
		CharInfo.Location = LOC_CENTRAL;
		CharInfo.Status = CDS_NOTFOUND;
		CharInfo.m_CachedStatus = CDS_UNLOADED; // force an update!
		return;
	}

	JSONDocument& doc = (*jsonDoc);
	const int flags = doc["data"]["flags"].GetInt();

	if (FNShared::IsBanned(flags) == true)
	{
		FNShared::Print("Account banned from FN! %llu!\n", m_iSteamID64);
		pPlayer->KickPlayer("You have been banned from FN!");
		return;
	}

	m_iRequestBodySize = doc["data"]["size"].GetInt();
	m_sRequestBody = new char[m_iRequestBodySize];
	memcpy(m_sRequestBody, (char*)base64_decode(doc["data"]["data"].GetString()).c_str(), m_iRequestBodySize);

	CharInfo.AssignChar(m_iSlot, LOC_CENTRAL, (char*)m_sRequestBody, m_iRequestBodySize, pPlayer);
	strncpy(CharInfo.Guid, doc["data"]["id"].GetString(), MSSTRING_SIZE);
	CharInfo.Flags = flags;
	CharInfo.Status = CDS_LOADED;
	CharInfo.m_CachedStatus = CDS_UNLOADED; // force an update!
}