#include "RequestManager.h"
#include "msdllheaders.h"

void CRequestManager::Init()
{
	Shutdown();

	// FN Doesn't work on listen servers.
	if (!IS_DEDICATED_SERVER())
	{
		return;
	}

	if (!m_bLoaded) 
	{
		m_bLoaded = true;
	}
}

void CRequestManager::Think(bool skipCallback)
{
	if (m_bLoaded)
	{
		for (int i = (m_vRequests.size() - 1); i >= 0; i--)
		{
			HTTPRequest* req = m_vRequests[i];
			req->m_bSkipCallback = skipCallback;
			
			switch (req->m_iRequestState)
			{
			case HTTPRequest::RequestState::REQUEST_QUEUED:
				req->AsyncSendRequestDiscard();
				break;

			case HTTPRequest::RequestState::REQUEST_FINISHED:
				delete req;
				m_vRequests.erase(m_vRequests.begin() + i);
				break;
			}
		}
	}
}

extern void wait(unsigned long ms);
void CRequestManager::Shutdown(void)
{
	if (!m_bLoaded)
		return;

	// we run think here to finish up the requests to prevent dataloss.
	do {
		Think(true);
		wait(10);
	}while(m_vRequests.size() != 0);

	m_vRequests.clear();
	m_bLoaded = false;
}

void CRequestManager::QueueRequest(HTTPRequest* req)
{
	if (!m_bLoaded)
		return;
	
	m_vRequests.push_back(req);
}