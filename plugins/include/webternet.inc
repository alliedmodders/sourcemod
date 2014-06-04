#if defined _webternet_included
	#endinput
#endif

#define _webternet_included

#define HTTP_OK		1
#define HTTP_ERROR	0

funcenum HTTP_SessionCallback {
    public(Handle:session, result),
	public(Handle:session, result, Handle:downloader),
	public(Handle:session, result, Handle:downloader, any:data),
};

native Handle:HTTP_CreateSession();

native HTTP_SetFailOnHTTPError(Handle:session, bool:fail);

native HTTP_GetLastError(Handle:session, String:error[], maxlen);

native Handle:HTTP_CreateFileDownloader(const String:file[]);

native Handle:HTTP_CreateMemoryDownloader();

native HTTP_Download(Handle:session, Handle:downloader, const String:url[], HTTP_SessionCallback:callback, any:data = INVALID_HANDLE);

native HTTP_PostAndDownload(Handle:session, Handle:downloader, Handle:form, const String:url[], HTTP_SessionCallback:callback, any:data = INVALID_HANDLE);

native Handle:HTTP_CreateWebForm();

native HTTP_AddStringToWebForm(Handle:form, const String:name[], const String:data[]);

native HTTP_AddFileToWebForm(Handle:form, const String:name[], const String:path[]);

native HTTP_GetBodySize(Handle:downloader);

native HTTP_GetBodyContent(Handle:downloader, String:body[], maxlen);


/**
 * Do not edit below this line!
 */
public Extension:__ext_webternet = 
{
	name = "webternet",
	file = "webternet.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};
