#ifndef _INCLUDE_SOURCEMOD_SIMPLE_TRIE_H_
#define _INCLUDE_SOURCEMOD_SIMPLE_TRIE_H_

struct Trie;

Trie *sm_trie_create();
void sm_trie_destroy(Trie *trie);
bool sm_trie_insert(Trie *trie, const char *key, void *value);
bool sm_trie_retrieve(Trie *trie, const char *key, void **value);

#endif //_INCLUDE_SOURCEMOD_SIMPLE_TRIE_H_
