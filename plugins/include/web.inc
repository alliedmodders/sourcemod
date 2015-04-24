#if defined _web_included
 #endinput
#endif
#define _web_included

methodmap WebForm < Handle
{
	/**
	 * Creates a new webform for POST request parameters.
	 * NOTE: Automatically freed when used by WebSession.DownloadString(), otherwise free it yourself.
	 *
	 * @return Handle to new webform, null otherwise.
	 * @error Unable to create form or new handle internally.
	 */
	public native WebForm();
	
	/*
	 * Adds a string value to form.
	 *
	 * @param paramName		Name of POST param to put a value to.
	 * @param paramValue	Value of param.
	 * @return				True if successful, false otherwise.
	 * @error				Invalid handle.
	 */
	public native bool AddString(const char[] paramName, const char[] paramValue);
	
	/*
	 * Adds a file(that is uploaded) to the post param list.
	 *
	 * @param paramName		Name of POST param to put a value to.
	 * @param path			Path of file.
	 * @return				True if successful, false otherwise.
	 * @error				Invalid handle.
	 */
	public native bool AddFile(const char[] paramName, const char[] path);
};

enum DownloadWriteStatus
{
	DownloadWrite_Ok = 0,
	DownloadWrite_Error,
};

/**
 * Callback for DownloadString()
 *
 * @param session				Handle to session that did the download.
 * @param downloadedString		String returned from url.
 * @param data					Data passed to DownloadString().
 * @return						DownloadWrite_Ok on success, DownloadWrite_Error otherwise.
 */
typedef WebCallback = function DownloadWriteStatus(WebSession session, const char[] downloadedString, any data);

/**
 * Called on http error.
 *
 * @param session				Session that errored.
 */
typedef WebError = function void(WebSession session);

methodmap WebSession < Handle
{
	/**
	 * Creates a new websession handle.
	 * Note: Only really need to call this once. All web sessions can be done with 1 handle.
	 *
	 * @return		Handle to new web session, null otherwise.
	 * @error		Unable to create session or handle.
	 */
	public native WebSession();
	
	/**
	 * Adds a form to the session.
	 * Note: Call this once and only once before DownloadString().
	 *
	 * @param form		Handle to web form.
	 * @return			True on success, false if a form is already set.
	 * @error			Invalid web session handle.
	 */
	public native bool SetPostParam(WebForm form);
	
	/*
	 * Should DownloadString() return false when an http error code is returned?
	 *
	 * @param error		True to fail on http error code, false to not.
	 * @return			True on success, false otherwise.
	 * @error			Invalid handle.
	 */
	public native bool SetErrorOnHttpError(bool error = true);
	
	/*
	 * Should DownloadString() return the http header also?
	 *
	 * @param error		True to return header, false to not.
	 * @return			True on success, false otherwise.
	 * @error			Invalid handle.
	 */
	public native bool SetHeaderReturn(bool returnHeader = true);
	
	/*
	 * Gets the last error code returned.
	 *
	 * @return			Last error code returned.
	 * @error			Invalid handle.
	 */
	public native int GetLastErrorCode();
	
	/*
	 * Gets the last error message returned.
	 *
	 * @param buffer	Buffer to store error message.
	 * @param size		Size of buffer.
	 * @return			Number of bytes written.
	 * @error			Invalid handle.
	 */
	public native int GetLastErrorMessage(char[] buffer, int size);
	
	/*
	 * Downloads the contents of url.
	 *
	 * @param url		Url to get string of.
	 * @param callback	Callback to call when complete.
	 * @param data		Optional data to pass to callback.
	 * @return			True on success, false otherwise.
	 */
	public native bool QueueDownloadString(const char[] url, WebCallback callback, WebError error, any data = 0);
};

public Extension __ext_web = 
{
	name = "Webternet",
	file = "webternet.ext",
	autoload = 1,
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_EXTENSIONS
public __ext_web_SetNTVOptional()
{
	MarkNativeAsOptional("WebSession.WebSession");
	MarkNativeAsOptional("WebSession.SetPostParam");
	MarkNativeAsOptional("WebSession.SetErrorOnHttpError");
	MarkNativeAsOptional("WebSession.SetHeaderReturn");
	MarkNativeAsOptional("WebSession.GetLastErrorCode");
	MarkNativeAsOptional("WebSession.GetLastErrorMessage");
	MarkNativeAsOptional("WebSession.DownloadString");
	MarkNativeAsOptional("WebForm.WebForm");
	MarkNativeAsOptional("WebForm.AddString");
	MarkNativeAsOptional("WebForm.AddFile");
}
#endif