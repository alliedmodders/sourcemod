#if defined _web_included
 #endinput
#endif
#define _web_included

/**
 * Note: This is auto freed when successfully used by the WebSession's QueueDownload, otherwise, free it yourself.
 */
methodmap WebForm < Handle
{
	/**
	 * Creates a new web form to use for post parameters.
	 *
	 * @return		A new web form to add post params, null if failed.
	 * @error		When form is unable to be created.
	 */
	public native WebForm();
	
	/**
	 * Adds a string parameter to post params list.
	 *
	 * @param paramName		Post param name to add value to.
	 * @param paramValue	Post param value to assign.
	 * @return				True on success, false otherwise.
	 * @error				Invalid handle.
	 */
	public native bool AddString(const char[] paramName, const char[] paramValue);
	
	/**
	 * Adds a string parameter to post params list.
	 *
	 * @param paramName		Post param name to add value to.
	 * @param filePath		Path to file to upload.
	 * @return				True on success, false otherwise.
	 * @error				Invalid handle.
	 */
	public native bool AddFile(const char[] paramName, const char[] filePath);
};

enum DownloadWriteStatus
{
	DownloadWrite_Okay,		/**< Data transfer was successful. */
	DownloadWrite_Error,	/**< Halt the transfer and return an error. */
};

typedef OnWebReturn = function DownloadWriteStatus(WebSession session, const char[] content, any data);

typedef OnWebError = function void(WebSession session);

methodmap WebSession < Handle
{
	/**
	 * Creates a new web session.
	 * Note: Recommended to call only once as you can do all web operations from 1 session.
	 *
	 * @return		A new web session, null if failed.
	 * @error		When session is unable to be created.
	 */
	public native WebSession();
	
	/**
	 * When set, the returned data will include the header.
	 *
	 * @param headerReturn		True to return header, false to not.
	 * @return					True on success, false otherwise.
	 * @error					Invalid handle.
	 */
	public native bool ReturnHeader(bool headerReturn);
	
	/**
	 * When set, the OnError callback will be fired when the remote server returns an http error code.
	 *
	 * @param headerReturn		True to error on http error code, false to not.
	 * @return					True on success, false otherwise.
	 * @error					Invalid handle.
	 */
	public native bool FailOnHttpError(bool fail);
	
	/**
	 * Gets the last error code recieved.
	 *
	 * @return		Last error code recieved(0 = No error).
	 * @error		Invalid handle.
	 */
	public native int LastErrorCode();
	
	/**
	 * Gets the last error message recieved.
	 *
	 * @param buffer	Buffer to write message to.
	 * @param length	Buffer length.
	 * @return			Number of bytes written.
	 * @error			Invalid handle.
	 */
	public native int LastErrorMessage(char[] buffer, int length);
	
	/**
	 * Queues up downloading of a web url.
	 *
	 * @param url			Url to download.
	 * @param callback		Callback to call when string is downloaded.
	 * @param error			Callback when download fails.
	 * @param postParams	POST parameters to pass to server.
	 * @param data			Data to pass to callback.
	 */
	public native void QueueDownload(const char[] url, OnWebReturn callback, OnWebError error, WebForm postParams = null, any data = 0);
};

public Extension __ext_web = 
{
	name = "Webternet",
	file = "webternet.ext",
	autoload = 0,
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_EXTENSIONS
public __ext_web_SetNTVOptional()
{
	MarkNativeAsOptional("WebForm.WebForm");
	MarkNativeAsOptional("WebForm.AddString");
	MarkNativeAsOptional("WebForm.AddFile");
	MarkNativeAsOptional("WebSession.WebSession");
	MarkNativeAsOptional("WebSession.ReturnHeader");
	MarkNativeAsOptional("WebSession.FailOnHttpError");
	MarkNativeAsOptional("WebSession.LastErrorCode");
	MarkNativeAsOptional("WebSession.LastErrorMessage");
	MarkNativeAsOptional("WebSession.QueueDownload");
}
