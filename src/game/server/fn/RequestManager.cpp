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

void CRequestManager::Think()
{
	if (m_bLoaded)
	{
		for (int i = (m_vRequests.size() - 1); i >= 0; i--)
		{
			HTTPRequest* req = m_vRequests[i];
			
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

void CRequestManager::Shutdown(void)
{
	// we run think here to finish up the requests to prevent dataloss.
	Think();

	m_vRequests.clear();
	m_bLoaded = false;
}

void CRequestManager::QueueRequest(HTTPRequest* req)
{
	m_vRequests.push_back(req);
}