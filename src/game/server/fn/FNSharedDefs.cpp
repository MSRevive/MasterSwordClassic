//
// FN Shared Definitions
//

#include "rapidjson/document.h"
#include "FNSharedDefs.h"
#include "msdllheaders.h"
#include "player.h"
#include "logger.h"
#include "global.h"
#include "crc/crchash.h"
#include "RequestManager.h"

// Requests
#include "ValidateConReq.h"
#include "ValidateScriptsReq.h"
#include "ValidateMapReq.h"
#include "CreateCharacterReq.h"
#include "UpdateCharacterReq.h"
#include "LoadCharacterReq.h"
#include "DeleteCharacterReq.h"

#define STRING_BUFFER 1024

void FNShared::Print(const char* fmt, ...)
{
	static char string[STRING_BUFFER];

	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	g_engfuncs.pfnServerPrint(string);
	logfile << Logger::LOG_INFO << string;
}

bool FNShared::IsSlotValid(int slot)
{
	return ((slot >= 0) && (slot < MAX_CHARSLOTS));
}

bool FNShared::IsEnabled(void)
{
	return (MSGlobals::CentralEnabled && !MSGlobals::IsLanGame && MSGlobals::ServerSideChar);
}

// Send validation requests to the FN backend.
void FNShared::Validate(void)
{
	if (IsEnabled() == false)
		return;

	char scFile[MAX_PATH];
	_snprintf(scFile, sizeof(scFile), "%s/dlls/sc.dll", MSGlobals::AbsGamePath.c_str());
	unsigned int scFileHash = GetFileCheckSum(scFile);

	char mapFile[MAX_PATH];
	_snprintf(mapFile, sizeof(mapFile), "%s/maps/%s.bsp", MSGlobals::AbsGamePath.c_str(), MSGlobals::MapName.c_str());
	unsigned int mapFileHash = GetFileCheckSum(mapFile);

	g_FNRequestManager.QueueRequest(new ValidateScriptsRequest(UTIL_VarArgs("/api/v2/internal/sc/%u", scFileHash)));
	g_FNRequestManager.QueueRequest(new ValidateMapRequest(UTIL_VarArgs("/api/v2/internal/map/%s/%u", MSGlobals::MapName.c_str(), mapFileHash)));
}

bool FNShared::ValidateFN(void)
{
	if (IsEnabled() == false)
		return false;
	
	std::unique_ptr<HTTPRequest> pReq(new ValidateConnectivityRequest("/api/v2/internal/ping"));
	if (pReq.get()->AsyncSendRequest())
	{
		JSONDocument& doc = (*pReq.get()->m_JSONResponse);
		return doc["data"].GetBool();
	}

	return false;
}

// Check if player has BANNED flag.
bool FNShared::IsBanned(int flags)
{
	return (flags & FN_FLAG_BANNED) == FN_FLAG_BANNED;
}

// Check if player has DONOR flag.
bool FNShared::IsDonor(int flags)
{
	return (flags & FN_FLAG_DONOR) == FN_FLAG_DONOR;
}

// Check if player has ADMIN flag.
bool FNShared::IsAdmin(int flags)
{
	return (flags & FN_FLAG_ADMIN) == FN_FLAG_ADMIN;
}

// Load all characters!
void FNShared::LoadCharacter(CBasePlayer* pPlayer)
{
	if ((pPlayer == NULL) || (pPlayer->steamID64 == 0ULL))
		return;

	for (int i = 0; i < MAX_CHARSLOTS; i++)
	{
		if (pPlayer->m_CharInfo[i].Status == CDS_LOADING)
			continue;

		pPlayer->m_CharInfo[i].m_CachedStatus = CDS_UNLOADED;
		pPlayer->m_CharInfo[i].Status = CDS_LOADING;

		g_FNRequestManager.QueueRequest(new LoadCharacterRequest(pPlayer->steamID64, i, UTIL_VarArgs("/api/v2/internal/character/%llu/%i", pPlayer->steamID64, i)));
	}
}

// Load a specific character!
void FNShared::LoadCharacter(CBasePlayer* pPlayer, int slot)
{
	if ((pPlayer == NULL) || (pPlayer->steamID64 == 0ULL) || !IsSlotValid(slot) || (pPlayer->m_CharInfo[slot].Status == CDS_LOADING))
		return;

	pPlayer->m_CharInfo[slot].m_CachedStatus = CDS_UNLOADED;
	pPlayer->m_CharInfo[slot].Status = CDS_LOADING;

	g_FNRequestManager.QueueRequest(new LoadCharacterRequest(pPlayer->steamID64, slot, UTIL_VarArgs("/api/v2/internal/character/%llu/%i", pPlayer->steamID64, slot)));
}

// Create or Update FN character!
void FNShared::CreateOrUpdateCharacter(CBasePlayer* pPlayer, int slot, const char* data, size_t size, bool bIsUpdate)
{
	if ((pPlayer == NULL) || (pPlayer->steamID64 == 0ULL) || (data == NULL) || (size <= 0) || !IsSlotValid(slot))
		return; // Quick validation - steamId is vital.

	if (bIsUpdate && (pPlayer->m_CharacterState == CHARSTATE_UNLOADED))
		return; // You cannot update your char (save) if there is no char loaded.

	if (!bIsUpdate && (pPlayer->m_CharInfo[slot].Status == CDS_LOADING))
		return; // Busy, wait for callback!

	char pchApiUrl[REQUEST_URL_SIZE];

	if (bIsUpdate)
	{
		_snprintf(pchApiUrl, REQUEST_URL_SIZE, "/api/v2/internal/character/%s", pPlayer->m_CharInfo[slot].Guid);
		
		g_FNRequestManager.QueueRequest(new UpdateCharacterRequest(pPlayer->steamID64, slot, pchApiUrl, data, size));
	}
	else
	{
		_snprintf(pchApiUrl, REQUEST_URL_SIZE, "/api/v2/internal/character/");
		pPlayer->m_CharInfo[slot].m_CachedStatus = CDS_UNLOADED;
		pPlayer->m_CharInfo[slot].Status = CDS_LOADING;
		
		g_FNRequestManager.QueueRequest(new CreateCharacterRequest(pPlayer->steamID64, slot, pchApiUrl, data, size));
	}
}

void FNShared::DeleteCharacter(CBasePlayer* pPlayer, int slot)
{
	if ((pPlayer == NULL) || (pPlayer->steamID64 == 0ULL) || !IsSlotValid(slot) || (pPlayer->m_CharInfo[slot].Status == CDS_LOADING))
		return;

	pPlayer->m_CharInfo[slot].m_CachedStatus = CDS_UNLOADED;
	pPlayer->m_CharInfo[slot].Status = CDS_LOADING;

	g_FNRequestManager.QueueRequest(new DeleteCharacterRequest(pPlayer->steamID64, slot, UTIL_VarArgs("/api/v2/internal/character/%s", pPlayer->m_CharInfo[slot].Guid)));
}