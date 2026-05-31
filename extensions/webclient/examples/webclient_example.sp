/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Web Client Extension
 * Copyright (C) 2024 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

/**
 * WebClient extension example plugin.
 *
 * Demonstrates an async HTTP GET and POST, and a WebSocket echo round-trip.
 * Build with spcomp against plugins/include/webclient.inc.
 *
 *   sm_webclient_get          - GET https://example.com and log status + length
 *   sm_webclient_post         - POST a small JSON body to httpbin and log the reply
 *   sm_webclient_download     - stream a response body to a file in addons/sourcemod/data/webclient
 *   sm_webclient_ws           - connect to an echo server, send a message, log the echo
 */

#include <sourcemod>
#include <webclient>

#pragma semicolon 1
#pragma newdecls required

public Plugin myinfo =
{
	name = "WebClient Example",
	author = "AlliedModders LLC",
	description = "Demonstrates the WebClient HTTP/WebSocket natives",
	version = "1.0.0",
	url = "https://www.sourcemod.net/"
};

public void OnPluginStart()
{
	RegConsoleCmd("sm_webclient_get", Cmd_Get);
	RegConsoleCmd("sm_webclient_post", Cmd_Post);
	RegConsoleCmd("sm_webclient_download", Cmd_Download);
	RegConsoleCmd("sm_webclient_ws", Cmd_Ws);
}

/* ----- HTTP GET ----- */

Action Cmd_Get(int client, int args)
{
	HTTPRequest request = new HTTPRequest("https://example.com/");
	request.SetHeader("User-Agent", "SourceMod-WebClient/1.0");
	request.Timeout = 10;
	// Append a percent-encoded query parameter (no manual URL building). The
	// value is a format string, so pass dynamic/user data via "%s".
	request.AppendQueryParam("q", "%s", "hello world & friends");

	// The returned token can cancel the request via WebClient_CancelRequest().
	int token = request.Get(OnGetComplete, client);
	ReplyToCommand(client, "[WebClient] GET dispatched (token %d)...", token);
	return Plugin_Handled;
}

void OnGetComplete(HTTPResponse response, any data, bool failure, const char[] error)
{
	if (failure)
	{
		PrintToServer("[WebClient] GET failed: %s", error);
		return;
	}

	char url[256];
	response.GetURL(url, sizeof(url));
	char contentType[128];
	response.GetHeader("Content-Type", contentType, sizeof(contentType));
	PrintToServer("[WebClient] GET ok=%d status=%d length=%d url=%s content-type=%s",
		response.IsSuccess(), response.Status, response.Length, url, contentType);

	// Status is tagged HTTPStatus: compare against the named constants.
	if (response.Status == HTTPStatus_OK)
		PrintToServer("[WebClient] server returned 200 OK");
	else if (response.IsServerError())
		PrintToServer("[WebClient] server-side error");

	// Enumerate every header without knowing the names up front.
	char name[64], value[1024];
	for (int i = 0; i < response.Headers; i++)
	{
		response.GetHeaderName(i, name, sizeof(name));
		response.GetHeaderValue(i, value, sizeof(value));
		PrintToServer("[WebClient]   %s: %s", name, value);
	}
}

/* ----- HTTP POST ----- */

Action Cmd_Post(int client, int args)
{
	HTTPRequest request = new HTTPRequest("https://httpbin.org/post");
	request.SetHeader("Content-Type", "application/json");
	request.Post(OnPostComplete, "{\"hello\":\"sourcemod\"}", client);
	ReplyToCommand(client, "[WebClient] POST dispatched...");
	return Plugin_Handled;
}

void OnPostComplete(HTTPResponse response, any data, bool failure, const char[] error)
{
	if (failure)
	{
		PrintToServer("[WebClient] POST failed: %s", error);
		return;
	}

	char[] body = new char[response.Length + 1];
	response.GetData(body, response.Length + 1);
	PrintToServer("[WebClient] POST status=%d body=%s", response.Status, body);
}

/* ----- HTTP download to file ----- */

Action Cmd_Download(int client, int args)
{
	HTTPRequest request = new HTTPRequest("https://example.com/");
	// Streamed to disk, so it isn't bound by the in-memory body cap. The path is
	// confined to addons/sourcemod/data/webclient (".." / absolute paths are rejected).
	request.DownloadFile("example.html", OnDownloadComplete, client);
	ReplyToCommand(client, "[WebClient] download dispatched...");
	return Plugin_Handled;
}

void OnDownloadComplete(HTTPResponse response, any data, bool failure, const char[] error)
{
	if (failure)
	{
		PrintToServer("[WebClient] download failed: %s", error);
		return;
	}
	// The body isn't buffered for a download; it's on disk in addons/sourcemod/data/webclient.
	PrintToServer("[WebClient] download ok=%d status=%d (saved to addons/sourcemod/data/webclient)",
		response.IsSuccess(), response.Status);
}

/* ----- WebSocket echo ----- */

// Demo uses one socket at a time. A socket ends with exactly one terminal
// callback (error XOR disconnect), so this flag only makes the single-handle
// delete idempotent. For concurrent sockets, track closed state per handle.
bool g_WsClosed;

Action Cmd_Ws(int client, int args)
{
	g_WsClosed = false;
	WebSocket ws = new WebSocket("wss://echo.websocket.org");
	ws.SetMessageCallback(OnWsMessage);
	ws.SetDisconnectCallback(OnWsDisconnect);
	ws.SetErrorCallback(OnWsError);
	// If Connect() throws (see its @error) no terminal callback fires, so this
	// handle would leak until unload; a real plugin should delete it on throw.
	ws.Connect(OnWsConnect, client);
	ReplyToCommand(client, "[WebClient] WebSocket connecting...");
	return Plugin_Handled;
}

void OnWsConnect(WebSocket ws, any data)
{
	PrintToServer("[WebClient] WS connected; sending hello");
	ws.Send("hello from sourcemod");
}

void OnWsMessage(WebSocket ws, any data, const char[] payload, int length, bool isText)
{
	// %s is safe here only because this echo server sends text (isText); binary
	// frames should be read as `length` bytes (they may contain interior NULs).
	PrintToServer("[WebClient] WS message (%d bytes, text=%d): %s", length, isText, payload);
	// One round-trip is enough for the demo.
	ws.Close(1000, "done");
}

void OnWsDisconnect(WebSocket ws, any data, int code, const char[] reason)
{
	PrintToServer("[WebClient] WS disconnected code=%d reason=%s", code, reason);
	if (!g_WsClosed)
	{
		g_WsClosed = true;
		delete ws;
	}
}

void OnWsError(WebSocket ws, any data, const char[] error)
{
	PrintToServer("[WebClient] WS error: %s", error);
	if (!g_WsClosed)
	{
		g_WsClosed = true;
		delete ws;
	}
}
