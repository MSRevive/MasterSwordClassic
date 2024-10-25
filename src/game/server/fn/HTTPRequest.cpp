//
// Steam HTTP Request Handler Class
//

#ifndef _WIN32
#define CURL_PULL_SYS_TYPES_H
#define CURL_PULL_STDINT_H
#define CURL_PULL_INTTYPES_H
#define HAVE_SYS_SOCKET_H
#endif

#include <string>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include "base64/base64.h"
#include "HTTPRequest.h"
#include "FNSharedDefs.h"
#include "msdllheaders.h"
#include "global.h"
#include "player.h"
#include "FNSharedDefs.h"

static char g_szBaseUrl[REQUEST_URL_SIZE];

HTTPRequest::HTTPRequest(HTTPMethod method, const char* url, uint8* body, size_t bodySize, ID64 steamID64, ID64 slot)
{
	m_eHTTPMethod = method;
	m_iRequestState = RequestState::REQUEST_QUEUED;
	_snprintf(m_sPchAPIUrl, REQUEST_URL_SIZE, "http://%s%s", g_szBaseUrl, url);

	m_sRequestBody = nullptr;
	m_iRequestBodySize = 0;
	m_Handle = nullptr;

	m_iSteamID64 = steamID64;
	m_iSlot = slot;

	if ((body != nullptr) && (bodySize > 0))
	{
		m_iRequestBodySize = bodySize;
		m_sRequestBody = new uint8[m_iRequestBodySize];
		memcpy(m_sRequestBody, body, m_iRequestBodySize);
	}

	m_sResponseBody.clear();
}

HTTPRequest::~HTTPRequest()
{
	Cleanup();
}

bool HTTPRequest::SendRequest()
{
	if (m_Handle) 
		return false;

	m_iRequestState = RequestState::REQUEST_EXECUTED;

	m_Handle = curl_easy_init();
	curl_easy_setopt(m_Handle, CURLOPT_URL, m_sPchAPIUrl);
	curl_easy_setopt(m_Handle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(m_Handle, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(m_Handle, CURLOPT_NOSIGNAL, 1L);

	switch (m_eHTTPMethod)
	{
		case HTTPRequest::GET:
			break;
		case HTTPRequest::POST:
			curl_easy_setopt(m_Handle, CURLOPT_POST, 1);
			break;
		case HTTPRequest::DEL:
			curl_easy_setopt(m_Handle, CURLOPT_CUSTOMREQUEST, "DELETE");
			break;
		case HTTPRequest::PUT:
			curl_easy_setopt(m_Handle, CURLOPT_CUSTOMREQUEST, "PUT");
			break;
	}

	// Process request body.
	if (m_sRequestBody != nullptr)
	{
		char steamID64String[REQUEST_URL_SIZE];
		_snprintf(steamID64String, REQUEST_URL_SIZE, "%llu", m_iSteamID64);

		rapidjson::StringBuffer s;
		rapidjson::Writer<rapidjson::StringBuffer> writer(s);

		writer.StartObject();

		writer.Key("steamid");
		writer.String(steamID64String);

		writer.Key("slot");
		writer.Int(m_iSlot);

		writer.Key("size");
		writer.Int(m_iRequestBodySize);

		writer.Key("data");
		writer.String(base64_encode(m_sRequestBody, m_iRequestBodySize).c_str());

		writer.EndObject();

		const std::string buffer = s.GetString();
		curl_easy_setopt(m_Handle, CURLOPT_POSTFIELDS, buffer.c_str());
	}

	return PerformRequest();
}

bool HTTPRequest::AsyncSendRequest()
{
	if (m_Handle)
		return false;

	m_iRequestState = RequestState::REQUEST_EXECUTED;

	m_Handle = curl_easy_init();
	curl_easy_setopt(m_Handle, CURLOPT_URL, m_sPchAPIUrl);
	curl_easy_setopt(m_Handle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(m_Handle, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(m_Handle, CURLOPT_NOSIGNAL, 1L);

	switch (m_eHTTPMethod)
	{
		case HTTPMethod::GET:
			break;
		case HTTPMethod::POST:
			curl_easy_setopt(m_Handle, CURLOPT_POST, 1);
			break;
		case HTTPMethod::DEL:
			curl_easy_setopt(m_Handle, CURLOPT_CUSTOMREQUEST, "DELETE");
			break;
		case HTTPMethod::PUT:
			curl_easy_setopt(m_Handle, CURLOPT_CUSTOMREQUEST, "PUT");
			break;
	}

	// Process request body.
	if (m_sRequestBody != nullptr)
	{
		char steamID64String[REQUEST_URL_SIZE];
		_snprintf(steamID64String, REQUEST_URL_SIZE, "%llu", m_iSteamID64);

		rapidjson::StringBuffer s;
		rapidjson::Writer<rapidjson::StringBuffer> writer(s);

		writer.StartObject();

		writer.Key("steamid");
		writer.String(steamID64String);

		writer.Key("slot");
		writer.Int(m_iSlot);

		writer.Key("size");
		writer.Int(m_iRequestBodySize);

		writer.Key("data");
		writer.String(base64_encode(m_sRequestBody, m_iRequestBodySize).c_str());

		writer.EndObject();

		const std::string buffer = s.GetString();
		curl_easy_setopt(m_Handle, CURLOPT_POSTFIELDS, buffer.c_str());
	}

	m_ResponseFuture = std::async(std::launch::async, &HTTPRequest::PerformRequest, this);
	return m_Promise.get_future().get();
}

void HTTPRequest::AsyncSendRequestDiscard()
{
	if (m_Handle)
		return;

	m_iRequestState = RequestState::REQUEST_EXECUTED;

	m_Handle = curl_easy_init();
	curl_easy_setopt(m_Handle, CURLOPT_URL, m_sPchAPIUrl);
	curl_easy_setopt(m_Handle, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(m_Handle, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(m_Handle, CURLOPT_NOSIGNAL, 1L);

	switch (m_eHTTPMethod)
	{
		case HTTPMethod::GET:
			break;
		case HTTPMethod::POST:
			curl_easy_setopt(m_Handle, CURLOPT_POST, 1);
			break;
		case HTTPMethod::DEL:
			curl_easy_setopt(m_Handle, CURLOPT_CUSTOMREQUEST, "DELETE");
			break;
		case HTTPMethod::PUT:
			curl_easy_setopt(m_Handle, CURLOPT_CUSTOMREQUEST, "PUT");
			break;
	}

	// Process request body.
	if (m_sRequestBody != nullptr)
	{
		char steamID64String[REQUEST_URL_SIZE];
		_snprintf(steamID64String, REQUEST_URL_SIZE, "%llu", m_iSteamID64);

		rapidjson::StringBuffer s;
		rapidjson::Writer<rapidjson::StringBuffer> writer(s);

		writer.StartObject();

		writer.Key("steamid");
		writer.String(steamID64String);

		writer.Key("slot");
		writer.Int(m_iSlot);

		writer.Key("size");
		writer.Int(m_iRequestBodySize);

		writer.Key("data");
		writer.String(base64_encode(m_sRequestBody, m_iRequestBodySize).c_str());

		writer.EndObject();

		const std::string buffer = s.GetString();
		curl_easy_setopt(m_Handle, CURLOPT_POSTFIELDS, buffer.c_str());
	}

	// we use a lambda here because we don't care about the result
	auto future = std::async(std::launch::async, &HTTPRequest::PerformRequest, this);
}

bool HTTPRequest::PerformRequest()
{
	if (!m_Handle) 
		return false;

	bool success = false;
	curl_easy_setopt(m_Handle, CURLOPT_USERAGENT, "MSR Game Server");
	curl_easy_setopt(m_Handle, CURLOPT_WRITEFUNCTION, HTTPRequest::WriteCallbackDispatcher);
	curl_easy_setopt(m_Handle, CURLOPT_WRITEDATA, this);
	CURLcode result = curl_easy_perform(m_Handle);
	if (result == CURLE_OK)
	{
		int httpCode = 200;
		curl_easy_getinfo(m_Handle, CURLINFO_RESPONSE_CODE, &httpCode);
		ResponseCallback(httpCode);
		success = true;
	}else{
		ResponseCallback(0);
		success = false;
	}
	curl_easy_cleanup(m_Handle);
	m_Handle = nullptr;
	m_Promise.set_value(success);
	return success;
}

size_t HTTPRequest::WriteCallbackDispatcher(void* buf, size_t sz, size_t n, void* curlGet)
{
	return static_cast<HTTPRequest*>(curlGet)->WriteCallback(buf, sz, n);
}

size_t HTTPRequest::WriteCallback(void* ptr, size_t size, size_t nmemb)
{
	m_sResponseBody.append((char*)ptr, size * nmemb);
	return size * nmemb;
}

void HTTPRequest::ResponseCallback(int httpCode)
{
	if (httpCode == 0)
	{
		FNShared::Print("Request Failed. %s, '%s'\n", GetName(), g_szBaseUrl);
		m_iRequestState = RequestState::REQUEST_FINISHED;
		return;
	}

	if (httpCode < 200 || httpCode > 299)
	{
		if (httpCode == 401)
		{
			FNShared::Print("FN Authorization failed! %s\n", GetName());
			m_iRequestState = RequestState::REQUEST_FINISHED;
			return;
		}

		FNShared::Print("FN Server Error. %s Code: %d\n", GetName(), httpCode);
		m_iRequestState = RequestState::REQUEST_FINISHED;
		return;
	}

	if (httpCode == 204)
	{
		OnResponse(true, nullptr, httpCode);
		m_iRequestState = RequestState::REQUEST_FINISHED;
		return;
	}

	if (m_sResponseBody.empty())
	{
		FNShared::Print("The data hasn't been received. HTTP code: %d\n", httpCode);
		OnResponse(true, nullptr, httpCode);
		m_iRequestState = RequestState::REQUEST_FINISHED;
		return;
	}
	
	JSONDocument* jsonDoc = ParseJSON(m_sResponseBody.c_str());
	OnResponse(true, jsonDoc);
	delete jsonDoc;
	m_iRequestState = RequestState::REQUEST_FINISHED;
}

JSONDocument* HTTPRequest::ParseJSON(const char* data, size_t length)
{
	if (!(data && data[0]))
		return nullptr;

	JSONDocument* document = new JSONDocument;

	if (length > 0)
		document->Parse(data, length);
	else
		document->Parse(data);

	if (document->HasParseError())
	{
		delete document;
		return nullptr;
	}

	return document;
}

void HTTPRequest::Cleanup()
{
	delete m_sRequestBody;
	m_sResponseBody.clear();

	// just incase it's not cleanuped already.
	if (m_Handle)
	{
		curl_easy_cleanup(m_Handle);
		m_Handle = nullptr;
	}
}

/*
void HTTPRequest::OnHTTPRequestCompleted(HTTPRequestCompleted_t* p, bool bError)
{
	if (suppressResponse || (handle == NULL) || (p == nullptr) || (p->m_hRequest != handle))
	{
		ReleaseHandle();
		return;
	}

	if (bError || p->m_eStatusCode < 200 || p->m_eStatusCode > 299)
	{
		if (p->m_eStatusCode == 401)
		{
			FNShared::Print("FN Authorization failed! %s\n", GetName());
			ReleaseHandle();
			return;
		}

		if (!p->m_bRequestSuccessful)
		{
			FNShared::Print("The data hasn't been received. No response from the server. %s, '%s'\n", GetName(), g_szBaseUrl);
			OnResponse(false, p->m_eStatusCode);
			ReleaseHandle();
			return;
		}

		FNShared::Print("FN Server Error. %s Code: %d\n", GetName(), p->m_eStatusCode);
		ReleaseHandle();
		return;
	}

	size_t unBytes = 0;
	if ((responseBody == nullptr) && g_SteamHTTPContext->GetHTTPResponseBodySize(handle, &unBytes))
	{
		// it should've never gotten to this point but okay.
		if (p->m_eStatusCode == 204)
		{
			OnResponse(true, p->m_eStatusCode);
			ReleaseHandle();
			return;
		}

		if (unBytes <= 0)
		{
			FNShared::Print("The data hasn't been received. HTTP code: %d\n", p->m_eStatusCode);
			ReleaseHandle();
			return;
		}

		responseBodySize = unBytes;
		responseBody = new uint8[responseBodySize];

		if (g_SteamHTTPContext->GetHTTPResponseBodyData(handle, responseBody, unBytes))
			pJSONData = ParseJSON(reinterpret_cast<char*>(responseBody), responseBodySize);
	}

	OnResponse(true, p->m_eStatusCode);
	ReleaseHandle();
}*/

/* static */ void HTTPRequest::SetBaseURL(const char* url)
{
	_snprintf(g_szBaseUrl, REQUEST_URL_SIZE, "%s", url);
}