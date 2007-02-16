/**
* vim: set ts=4 :
* ===============================================================
* SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
* ===============================================================
*
* This file is not open source and may not be copied without explicit
* written permission of AlliedModders LLC.  This file may not be redistributed 
* in whole or significant part.
* For information, see LICENSE.txt or http://www.sourcemod.net/license.php
*
* Version: $Id$
*/

#ifndef _INCLUDE_SOURCEMOD_CMSGIDPOOL_H_
#define _INCLUDE_SOURCEMOD_CMSGIDPOOL_H_

#include "sm_trie.h"
#include "sourcemm_api.h"

#define INVALID_MESSAGE_ID -1

class CMessageIdPool
{
public:
	CMessageIdPool()
	{
		m_Names = sm_trie_create();
	}
	~CMessageIdPool()
	{
		sm_trie_destroy(m_Names);
	}
public:
	int GetMessageId(const char *name)
	{
		int msgid;
		if (!sm_trie_retrieve(m_Names, name, reinterpret_cast<void **>(&msgid)))
		{
			char buf[255];
			int dummy;
			msgid = 0;

			while (gamedll->GetUserMessageInfo(msgid, buf, sizeof(buf), dummy))
			{
				if (strcmp(name, buf) == 0)
				{
					sm_trie_insert(m_Names, name, reinterpret_cast<void *>(msgid));
					return msgid;
				}
				msgid++;
			}

			return INVALID_MESSAGE_ID;
		}

		return msgid;
	}
private:
	Trie *m_Names;
};

#endif //_INCLUDE_SOURCEMOD_CMSGIDPOOL_H_
