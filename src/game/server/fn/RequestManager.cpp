#include "RequestManager.h"
#include "msdllheaders.h"

void CRequestManager::Init(void)
{
	// FN Doesn't work on listen servers.
	if (!IS_DEDICATED_SERVER())
		return;

	if (!m_bLoaded) 
	{
		//m_SteamGameServerAPIContext = &CSteamGameServerAPIContext.Init();
		m_SteamHTTP = SteamGameServerHTTP();
		m_bLoaded = true;
	}
}

void CRequestManager::Think(void)
{
	if (m_bLoaded)
	{
		if (!m_SteamHTTP)
		{
			m_SteamHTTP = SteamGameServerHTTP();
		}

		for (int i = (m_vRequests.size() - 1); i >= 0; i--)
		{
			HTTPRequest* req = m_vRequests[i];
			switch (req->requestState)
			{
			case HTTPRequest::RequestState::REQUEST_QUEUED:
				req->SendRequest();
				break;

			case  HTTPRequest::RequestState::REQUEST_FINISHED:
				delete req;
				m_vRequests.erase(m_vRequests.begin() + i);
				break;
			}
		}
	}
}

void CRequestManager::Shutdown(void)
{	
	//m_SteamGameServerAPIContext.Shutdown();
	m_SteamHTTP	= nullptr;
	m_vRequests.clear();
	m_bLoaded = false;
}

void CRequestManager::RunCallbacks(void) 
{
	if (SteamGameServerHTTP())
		SteamGameServer_RunCallbacks();
}

extern void wait(unsigned long ms);
void CRequestManager::SendAndWait(void)
{
	if (m_bLoaded)
	{
		//g_bSuppressResponse = true;

		do
		{
			Think();
			RunCallbacks();
			wait(10);
		} while ((m_SteamHTTP != nullptr) && m_vRequests.size());

		Shutdown();
		//g_bSuppressResponse = false;
	}
}

void CRequestManager::QueueRequest(HTTPRequest* req)
{
	req->SetHTTPContext(m_SteamHTTP);
	m_vRequests.push_back(req);
}