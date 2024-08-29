//
// FuzzNet Data Handler - Load, Save, Update, Create characters
// Generally we want to load all three characters when player X joins the server
// Allow the player to easily switch between and use these characters.
//

#include "rapidjson/document_safe.h"
#include "base64/base64.h"
#include "msdllheaders.h"
#include "player.h"
#include "global.h"

#undef vector
#undef min
#undef max

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>

#include "httprequesthandler.h"
#include "fndatahandler.h"

#define REQUEST_URL_SIZE 256

enum FnPlayerFlags
{
	FN_FLAG_BANNED = 0x01,
	FN_FLAG_ADMIN = 0x02,
};

enum RequestCommand
{
	FN_REQ_LOAD = 0,
	FN_REQ_CREATE,
	FN_REQ_UPDATE,
	FN_REQ_DELETE,
};

enum RequestResult
{
	FN_RES_NA = 0,
	FN_RES_OK,
	FN_RES_ERR,
};

struct FnRequestData
{
public:
	FnRequestData(int command, unsigned long long steamID, int slot, const char* url)
	{
		this->guid[0] = 0;
		this->data = NULL;
		this->result = FN_RES_NA;
		this->steamID = steamID;
		this->command = command;
		this->slot = slot;
		this->size = 0;
		this->flags = 0;
		strncpy(this->url, url, REQUEST_URL_SIZE);
	}

	~FnRequestData()
	{
		delete[] data;
		data = NULL;
	}

	int command;
	int slot;
	int size;
	int result;
	int flags;
	unsigned long long steamID;
	char url[REQUEST_URL_SIZE];
	char guid[MSSTRING_SIZE];
	char* data;

private:
	FnRequestData(const FnRequestData& data);
};

extern void wait(unsigned long ms);
static std::atomic<bool> g_bShouldShutdownFn(false);
static std::atomic<bool> g_bShouldHandleRequests(false);
static std::vector<FnRequestData*> g_vRequestData;
static std::vector<FnRequestData*> g_vIntermediateData;
static std::mutex mutex;
static std::condition_variable cv;
static float g_fThinkTime = 0.0f;

static bool IsSlotValid(int slot) { return ((slot >= 0) && (slot < MAX_CHARSLOTS)); }

static const char* GetFnUrl(char* fmt, ...)
{
	static char requestUrl[REQUEST_URL_SIZE], string[REQUEST_URL_SIZE];

	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(string, sizeof(string), fmt, argptr);
	va_end(argptr);

	_snprintf(requestUrl, REQUEST_URL_SIZE, "%s%s", CVAR_GET_STRING("ms_central_addr"), string);
	return requestUrl;
}

// Load single char details.
static void LoadCharacter(FnRequestData* req, const JSONValue& val)
{
	req->size = val["size"].GetInt();
	strncpy(req->guid, val["id"].GetString(), MSSTRING_SIZE);
	req->data = new char[req->size];
	memcpy(req->data, (char*)base64_decode(val["data"].GetString()).c_str(), req->size);
	req->result = FN_RES_OK;
}

static void GetPlayerFlags(FnRequestData* req, const JSONDocument& doc)
{
	if (req == NULL)
		return;

	if (doc["isBanned"].GetBool())
		req->flags |= FN_FLAG_BANNED;

	if (doc["isAdmin"].GetBool())
		req->flags |= FN_FLAG_ADMIN;
}

// Handle a char request.
static void HandleRequest(FnRequestData* req)
{
	if (req == NULL)
		return;

	req->result = FN_RES_ERR;
	JSONDocument* pDoc = NULL;
	const bool bIsUpdate = (req->command == FN_REQ_UPDATE);

	switch (req->command)
	{

	case FN_REQ_LOAD:
	{
		pDoc = HTTPRequestHandler::GetRequestAsJson(req->url);
		if (pDoc == NULL)
			return;

		const JSONDocument& doc = *pDoc;

		if (doc["code"] == 400)
			return;

		GetPlayerFlags(req, doc);
		LoadCharacter(req, doc["data"]);
		break;
	}

	case FN_REQ_CREATE:
	case FN_REQ_UPDATE:
	{
		char steamID64String[MSSTRING_SIZE];
		_snprintf(steamID64String, MSSTRING_SIZE, "%llu", req->steamID);

		rapidjson::StringBuffer s;
		rapidjson::Writer<rapidjson::StringBuffer> writer(s);

		writer.StartObject();

		writer.Key("steamid");
		writer.String(steamID64String);

		writer.Key("slot");
		writer.Int(req->slot);

		writer.Key("size");
		writer.Int(req->size);

		writer.Key("data");
		writer.String(base64_encode((const unsigned char*)req->data, req->size).c_str());

		writer.EndObject();

		if (bIsUpdate)
		{
			req->result = FN_RES_OK;
			HTTPRequestHandler::PutRequest(req->url, s.GetString());
			break;
		}

		pDoc = HTTPRequestHandler::PostRequestAsJson(req->url, s.GetString());
		if (pDoc == NULL)
			return;

		const JSONDocument& doc = *pDoc;
		GetPlayerFlags(req, doc);
		strncpy(req->guid, doc["data"]["id"].GetString(), MSSTRING_SIZE);
		req->result = FN_RES_OK;
		break;
	}

	case FN_REQ_DELETE:
	{
		HTTPRequestHandler::DeleteRequest(req->url);
		req->result = FN_RES_OK;
		break;
	}

	}

	delete pDoc;
}

static void Worker(void)
{
	while (1)
	{
		std::unique_lock<std::mutex> lck(mutex);
		while ((g_bShouldShutdownFn == false) && (g_bShouldHandleRequests == false))
			cv.wait(lck);

		if (g_bShouldShutdownFn)
			break;

		for (int i = (g_vRequestData.size() - 1); i >= 0; i--)
		{
			FnRequestData* req = g_vRequestData[i];
			if (req->result != FN_RES_NA)
				continue;
			HandleRequest(req);
		}

		g_bShouldHandleRequests = false;
	}
}

void FnDataHandler::Initialize(void)
{
	g_bShouldShutdownFn = false;
	std::thread worker(Worker);
	worker.detach();
}

void FnDataHandler::Destroy(void)
{
	g_bShouldShutdownFn = true;
	cv.notify_all();
}

void FnDataHandler::Reset(void)
{
	cv.notify_all();

	// Wait for any remaining items.
	do
	{
		Think(true);
		wait(100);
	} while (g_vRequestData.size());

	g_fThinkTime = 0.0f;
}

void FnDataHandler::Think(bool bNoCallback)
{
	if (g_bShouldHandleRequests)
		return;

	if (!bNoCallback)
	{
		if (gpGlobals->time <= g_fThinkTime)
			return;
		g_fThinkTime = (gpGlobals->time + 0.15f);
	}

	if (mutex.try_lock())
	{
		for (int i = (g_vRequestData.size() - 1); i >= 0; i--)
		{
			const FnRequestData* req = g_vRequestData[i];

			if (req->result == FN_RES_NA)
			{
				g_bShouldHandleRequests = true;
				continue;
			}

			CBasePlayer* pPlayer = (bNoCallback ? NULL : UTIL_PlayerBySteamID(req->steamID));
			if (pPlayer)
			{
				if ((req->flags & FN_FLAG_BANNED) != 0)
					pPlayer->KickPlayer("You have been banned from FN!");
				else
				{
					charinfo_t& CharInfo = pPlayer->m_CharInfo[req->slot];

					switch (req->command)
					{

					case FN_REQ_LOAD:
					case FN_REQ_CREATE:
					{
						if (req->result == FN_RES_OK)
						{
							CharInfo.AssignChar(req->slot, LOC_CENTRAL, (char*)req->data, req->size, pPlayer);
							strncpy(CharInfo.Guid, req->guid, MSSTRING_SIZE);
						}
						else
						{
							CharInfo.Index = req->slot;
							CharInfo.Location = LOC_CENTRAL;
							CharInfo.Status = CDS_NOTFOUND;
						}

						CharInfo.m_CachedStatus = CDS_UNLOADED; // force an update!
						break;
					}

					case FN_REQ_DELETE:
					{
						CharInfo.Status = CDS_NOTFOUND;
						CharInfo.m_CachedStatus = CDS_UNLOADED; // force an update!
						break;
					}

					}
				}
			}

			g_vRequestData.erase(g_vRequestData.begin() + i);
			delete req;
		}

		// Add new requests
		if (g_vIntermediateData.size())
		{
			for (int i = 0; i < g_vIntermediateData.size(); i++)
				g_vRequestData.push_back(g_vIntermediateData[i]);

			g_vIntermediateData.clear();
			g_bShouldHandleRequests = true;
		}

		mutex.unlock();
	}

	if (g_bShouldHandleRequests)
		cv.notify_all();
}

bool FnDataHandler::IsEnabled(void)
{
	return (MSGlobals::CentralEnabled && !MSGlobals::IsLanGame && MSGlobals::ServerSideChar);
}

bool FnDataHandler::IsValid(const char* url)
{
	if (!url || !url[0] || !IsEnabled())
		return false;

	std::unique_lock<std::mutex> lck(mutex); // Ensure thread safety.

	JSONDocument* pDoc = HTTPRequestHandler::GetRequestAsJson(url);
	if (pDoc == NULL)
		return false;

	const JSONDocument& doc = *pDoc;
	const bool retVal = doc["data"].GetBool();

	delete pDoc;
	return retVal;
}

//Checks if the connection to via FN server is valid.
//needs optimzation!
bool FnDataHandler::IsValidConnection(void) { return IsValid(GetFnUrl("/api/v1/ping")); }

bool FnDataHandler::IsVerifiedMap(const char* name, unsigned int hash) { return IsValid(GetFnUrl("/api/v1/map/%s/%u", name, hash)); }

bool FnDataHandler::IsVerifiedSC(unsigned int hash) { return IsValid(GetFnUrl("/api/v1/sc/%u", hash)); }

// Load all characters!
void FnDataHandler::LoadCharacter(CBasePlayer* pPlayer)
{
	if ((pPlayer == NULL) || (pPlayer->steamID64 == 0ULL))
		return;

	for (int i = 0; i < MAX_CHARSLOTS; i++)
	{
		if (pPlayer->m_CharInfo[i].Status == CDS_LOADING)
			continue;

		pPlayer->m_CharInfo[i].m_CachedStatus = CDS_UNLOADED;
		pPlayer->m_CharInfo[i].Status = CDS_LOADING;
		g_vIntermediateData.push_back(new FnRequestData(FN_REQ_LOAD, pPlayer->steamID64, i, GetFnUrl("/api/v1/character/%llu/%i", pPlayer->steamID64, i)));
	}
}

// Load a specific character!
void FnDataHandler::LoadCharacter(CBasePlayer* pPlayer, int slot)
{
	if ((pPlayer == NULL) || (pPlayer->steamID64 == 0ULL) || !IsSlotValid(slot) || (pPlayer->m_CharInfo[slot].Status == CDS_LOADING))
		return;

	pPlayer->m_CharInfo[slot].m_CachedStatus = CDS_UNLOADED;
	pPlayer->m_CharInfo[slot].Status = CDS_LOADING;
	g_vIntermediateData.push_back(new FnRequestData(FN_REQ_LOAD, pPlayer->steamID64, slot, GetFnUrl("/api/v1/character/%llu/%i", pPlayer->steamID64, slot)));
}

// Create or Update character.
void FnDataHandler::CreateOrUpdateCharacter(CBasePlayer* pPlayer, int slot, const char* data, int size, bool bIsUpdate)
{
	if ((pPlayer == NULL) || (pPlayer->steamID64 == 0ULL) || (data == NULL) || (size <= 0) || !IsSlotValid(slot))
		return; // Quick validation - steamId is vital.

	if (bIsUpdate && (pPlayer->m_CharacterState == CHARSTATE_UNLOADED))
		return; // You cannot update your char (save) if there is no char loaded.

	if (!bIsUpdate && (pPlayer->m_CharInfo[slot].Status == CDS_LOADING))
		return; // Busy, wait for callback!

	FnRequestData* req = new FnRequestData(
		bIsUpdate ? FN_REQ_UPDATE : FN_REQ_CREATE,
		pPlayer->steamID64,
		slot,
		bIsUpdate ? GetFnUrl("/api/v1/character/%s", pPlayer->m_CharInfo[slot].Guid) : GetFnUrl("/api/v1/character/")
	);
	req->data = new char[size];
	req->size = size;
	memcpy(req->data, data, size);

	if (bIsUpdate) // Did we already add an update request? Swap it quickly!
	{
		for (int i = (g_vIntermediateData.size() - 1); i >= 0; i--)
		{
			const FnRequestData* pOtherReq = g_vIntermediateData[i];
			if ((pOtherReq->command != FN_REQ_UPDATE) || (pOtherReq->steamID == 0ULL) || (pOtherReq->steamID != pPlayer->steamID64))
				continue;

			g_vIntermediateData.erase(g_vIntermediateData.begin() + i);
			delete pOtherReq;
		}
	}
	else
	{
		pPlayer->m_CharInfo[slot].m_CachedStatus = CDS_UNLOADED;
		pPlayer->m_CharInfo[slot].Status = CDS_LOADING;
	}

	g_vIntermediateData.push_back(req);
}

void FnDataHandler::DeleteCharacter(CBasePlayer* pPlayer, int slot)
{
	if ((pPlayer == NULL) || (pPlayer->steamID64 == 0ULL) || !IsSlotValid(slot) || (pPlayer->m_CharInfo[slot].Status == CDS_LOADING))
		return;

	pPlayer->m_CharInfo[slot].m_CachedStatus = CDS_UNLOADED;
	pPlayer->m_CharInfo[slot].Status = CDS_LOADING;
	g_vIntermediateData.push_back(new FnRequestData(FN_REQ_DELETE, pPlayer->steamID64, slot, GetFnUrl("/api/v1/character/%s", pPlayer->m_CharInfo[slot].Guid)));
}