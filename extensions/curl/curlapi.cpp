#include "curlapi.h"

Webternet g_webternet;

WebForm::WebForm() : first(NULL), last(NULL), lastError(CURL_FORMADD_OK)
{
}

bool WebForm::AddString(const char *name, const char *data)
{
	lastError = curl_formadd(&first,
		&last,
		CURLFORM_COPYNAME,
		name,
		CURLFORM_COPYCONTENTS,
		data,
		CURLFORM_END);
	return lastError == CURL_FORMADD_OK;
}

bool WebForm::AddFile(const char *name, const char *path)
{
	lastError = curl_formadd(&first,
		&last,
		CURLFORM_COPYNAME,
		name,
		CURLFORM_FILE,
		path,
		CURLFORM_END);
	return lastError == CURL_FORMADD_OK;
}

curl_httppost *WebForm::GetFormData()
{
	return first;
}

WebForm::~WebForm()
{
	curl_formfree(first);
}

WebTransfer *WebTransfer::CreateWebSession()
{
	CURL *curl;

	curl = curl_easy_init();
	if (curl == NULL)
	{
		return NULL;
	}

	WebTransfer *easy = new WebTransfer(curl);
	
	if (curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, easy->errorBuffer))
	{
		delete easy;
		return NULL;
	}
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

	return easy;
}

WebTransfer::WebTransfer(CURL *_curl)
{
	this->curl = _curl;
}

const char *WebTransfer::LastErrorMessage()
{
	return errorBuffer;
}

int WebTransfer::LastErrorCode()
{
	return lastError;
}

bool WebTransfer::SetHeaderReturn(bool recv_hdr)
{
	lastError = curl_easy_setopt(curl, CURLOPT_HEADER, recv_hdr ? 1 : 0);
	return lastError == 0;
}

bool WebTransfer::SetFailOnHTTPError(bool fail)
{
	lastError = curl_easy_setopt(curl, CURLOPT_FAILONERROR, fail ? 1 : 0);
	return lastError == 0;
}

static size_t curl_write_to_sm(void *ptr, size_t bytes, size_t nmemb, void *stream)
{
	void **userdata = (void **)stream;
	IWebTransfer *session = (IWebTransfer *)userdata[0];
	ITransferHandler *handler = (ITransferHandler *)userdata[1];

	DownloadWriteStatus status;
	status = handler->OnDownloadWrite(session, userdata[2], ptr, bytes, nmemb);
	if (status != DownloadWrite_Okay)
	{
		/* Return a differing amount */
		return (bytes == 0 || nmemb == 0) ? 1 : 0;
	}

	return bytes * nmemb;
}

bool WebTransfer::Download(const char *url, ITransferHandler *handler, void *data)
{
	lastError = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_to_sm);
	if (lastError)
	{
		return false;
	}

	void *userdata[3];
	userdata[0] = this;
	userdata[1] = handler;
	userdata[2] = data;
	lastError = curl_easy_setopt(curl, CURLOPT_WRITEDATA, userdata);
	if (lastError)
	{
		return false;
	}

	lastError = curl_easy_setopt(curl, CURLOPT_HTTPPOST, NULL);
	if (lastError)
	{
		return false;
	}

	lastError = curl_easy_setopt(curl, CURLOPT_URL, url);
	if (lastError)
	{
		return false;
	}

	lastError = curl_easy_perform(curl);
	
	return (lastError == 0);
}

bool WebTransfer::PostAndDownload(const char *url,
								  IWebForm *form,
								  ITransferHandler *handler,
								  void *data)
{
	WebForm *realform = (WebForm*)form;

	lastError = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_to_sm);
	if (lastError)
	{
		return false;
	}

	void *userdata[3];
	userdata[0] = this;
	userdata[1] = handler;
	userdata[2] = data;
	lastError = curl_easy_setopt(curl, CURLOPT_WRITEDATA, userdata);
	if (lastError)
	{
		return false;
	}

	lastError = curl_easy_setopt(curl, CURLOPT_HTTPPOST, realform->GetFormData());
	if (lastError)
	{
		return false;
	}

	lastError = curl_easy_setopt(curl, CURLOPT_URL, url);
	if (lastError)
	{
		return false;
	}

	lastError = curl_easy_perform(curl);

	return (lastError == 0);
}

WebTransfer::~WebTransfer()
{
	curl_easy_cleanup(curl);
}

unsigned int Webternet::GetInterfaceVersion()
{
	return SMINTERFACE_WEBTERNET_VERSION;
}

const char *Webternet::GetInterfaceName()
{
	return SMINTERFACE_WEBTERNET_NAME;
}

IWebTransfer *Webternet::CreateSession()
{
	return WebTransfer::CreateWebSession();
}

IWebForm *Webternet::CreateForm()
{
	return new WebForm();
}
