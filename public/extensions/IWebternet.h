/**
 * vim: set ts=4 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_WEBTERNET_INTERFACE_H_
#define _INCLUDE_SOURCEMOD_WEBTERNET_INTERFACE_H_

#include <IShareSys.h>

/**
 * @file IWebternet.h
 * @brief Interface header for lib cURL.
 */

#define SMINTERFACE_WEBTERNET_NAME		"IWebternet"
#define SMINTERFACE_WEBTERNET_VERSION	3

namespace SourceMod
{
	/**
	 * @brief Status to return via the OnDownloadWrite function of ITransferHandler.
	 */
	enum DownloadWriteStatus
	{
		DownloadWrite_Okay,		/**< Data transfer was successful. */
		DownloadWrite_Error,	/**< Halt the transfer and return an error. */
	};

	class IWebTransfer;
	class IWebternet;

	/**
	 * @brief Form for POSTing data.
	 */
	class IWebForm
	{
	public:
		/**
		 * @brief Free with delete.
		 */
		virtual ~IWebForm()
		{
		}
	public:
		/**
		 * @brief Adds raw data to the form.
		 *
		 * All data is copied locally and may go out of scope.
		 *
		 * @param name				Field name (null terminated).
		 * @param data				Field data (null terminated).
		 * @return					True on success, false on failure.
		 */
		virtual bool AddString(const char *name, const char *data) = 0;

		/**
		 * @brief Adds a file to the form.
		 *
		 * All data is copied locally and may go out of scope.
		 *
		 * @param name				Field name (null terminated).
		 * @param path				Local file path (null terminated).
		 * @return					True on success, false on failure.
		 */
		virtual bool AddFile(const char *name, const char *path) = 0;
	};

	/**
	 * @brief Transfer handler interface.
	 */
	class ITransferHandler
	{
	public:
		/**
		 * @brief Must return the interface version this listener is compatible with.
		 *
		 * @return					Interface version.
		 */
		virtual unsigned int GetURLInterfaceVersion()
		{
			return SMINTERFACE_WEBTERNET_VERSION;
		}

		/**
		 * @brief Called when a downloader needs to write data it has received.
		 *
		 * @param downloader		Downloader object.
		 * @param userdata			User data passed to download function.
		 * @param ptr				Memory containing the received data.
		 * @param size				Size of each block in ptr.
		 * @param nmemb				Number of blocks in ptr.
		 * @return					Download status.
		 */
		virtual DownloadWriteStatus OnDownloadWrite(IWebTransfer *session,
			void *userdata,
			void *ptr,
			size_t size,
			size_t nmemb)
		{
			return DownloadWrite_Error;
		}
	};

	/**
	 * @brief Transfer interface.
	 *
	 * Though one instance may be used across multiple threads, the interface cannot be modified 
	 * while a transfer is in progress.
	 */
	class IWebTransfer
	{
	public:
		/**
		 * @brief Virtual destructor.  Call delete to release the resources.
		 */
		virtual ~IWebTransfer()
		{
		}

		/**
		 * @brief Returns a human-readable error message from the last failure.
		 *
		 * @return					Error message.
		 */
		virtual const char *LastErrorMessage() = 0;

		/**
		 * @brief Returns the last error code, if any.
		 *
		 * @return					Last error code.
		 */
		virtual int LastErrorCode() = 0;

		/**
		 * @brief Sets whether header information is returned, if any.
		 *
		 * @param recv_hdr			True to return headers, false otherwise.
		 * @return					True on success, false on failure.
		 */
		virtual bool SetHeaderReturn(bool recv_hdr) = 0;

		/**
		 * @brief Downloads a URL.
		 *
		 * @param url				URL to download.
		 * @param handler			Handler object.
		 * @param userdata			User data pointer.
		 * @return					True on success, false on failure.
		 */
		virtual bool Download(const char *url, ITransferHandler *handler, void *data) = 0;

		/**
		 * @brief Downloads a URL with POST options.
		 *
		 * @param url				URL to download.
		 * @param form				Form to read POST info from.
		 * @param handler			Handler object.
		 * @param userdata			User data pointer.
		 * @return					True on success, false on failure.
		 */
		virtual bool PostAndDownload(const char *url,
			IWebForm *form,
			ITransferHandler *handler,
			void *data) = 0;

		/**
		 * @brief Sets whether an HTTP failure (>= 400) returns false from Download().
		 *
		 * Note: defaults to false.
		 *
		 * @param fail				True to fail, false otherwise.
		 * @return					True on success, false otherwise.
		 */
		virtual bool SetFailOnHTTPError(bool fail) = 0;
	};

	/**
	 * @brief Interface for managing web URL sessions.
	 */
	class IWebternet : public SMInterface
	{
	public:
		virtual unsigned int GetInterfaceVersion() = 0;
		virtual const char *GetInterfaceName() = 0;
	public:
		/**
		 * @brief Create a URL transfer session object.
		 *
		 * @return				Object, or NULL on failure.
		 */
		virtual IWebTransfer *CreateSession() = 0;

		/**
		 * @brief Creates a form for building POST data.
		 *
		 * @return				New form, or NULL on failure.
		 */
		virtual IWebForm *CreateForm() = 0;
	};
}

#endif //_INCLUDE_SOURCEMOD_WEBTERNET_INTERFACE_H_

