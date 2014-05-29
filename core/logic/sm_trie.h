/**
 * vim: set ts=4 :
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

#ifndef _INCLUDE_SOURCEMOD_SIMPLE_TRIE_H_
#define _INCLUDE_SOURCEMOD_SIMPLE_TRIE_H_

struct Trie;

typedef void (*SM_TRIE_BAD_ITERATOR)(Trie *pTrie, const char *key, void **value, void *data);

Trie *sm_trie_create();
void sm_trie_destroy(Trie *trie);
bool sm_trie_insert(Trie *trie, const char *key, void *value);
bool sm_trie_replace(Trie *trie, const char *key, void *value);
bool sm_trie_retrieve(Trie *trie, const char *key, void **value);
bool sm_trie_delete(Trie *trie, const char *key);
void sm_trie_clear(Trie *trie);
size_t sm_trie_mem_usage(Trie *trie);
void sm_trie_bad_iterator(Trie *trie,
						  char *buffer,
						  size_t maxlength,
						  SM_TRIE_BAD_ITERATOR iter,
						  void *data);

#endif //_INCLUDE_SOURCEMOD_SIMPLE_TRIE_H_
