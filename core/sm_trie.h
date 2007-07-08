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

#ifndef _INCLUDE_SOURCEMOD_SIMPLE_TRIE_H_
#define _INCLUDE_SOURCEMOD_SIMPLE_TRIE_H_

struct Trie;

Trie *sm_trie_create();
void sm_trie_destroy(Trie *trie);
bool sm_trie_insert(Trie *trie, const char *key, void *value);
bool sm_trie_replace(Trie *trie, const char *key, void *value);
bool sm_trie_retrieve(Trie *trie, const char *key, void **value);
bool sm_trie_delete(Trie *trie, const char *key);
void sm_trie_clear(Trie *trie);

#endif //_INCLUDE_SOURCEMOD_SIMPLE_TRIE_H_
