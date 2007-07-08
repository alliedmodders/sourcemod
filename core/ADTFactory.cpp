#include "ADTFactory.h"
#include "sm_globals.h"
#include "ShareSys.h"

ADTFactory g_AdtFactory;

const char *ADTFactory::GetInterfaceName()
{
	return SMINTERFACE_ADTFACTORY_NAME;
}

unsigned int ADTFactory::GetInterfaceVersion()
{
	return SMINTERFACE_ADTFACTORY_VERSION;
}

void ADTFactory::OnSourceModAllInitialized()
{
	g_ShareSys.AddInterface(NULL, this);
}

IBasicTrie *ADTFactory::CreateBasicTrie()
{
	return new BaseTrie();
}

BaseTrie::BaseTrie()
{
	m_pTrie = sm_trie_create();
}

BaseTrie::~BaseTrie()
{
	sm_trie_destroy(m_pTrie);
}

bool BaseTrie::Insert(const char *key, void *value)
{
	return sm_trie_insert(m_pTrie, key, value);
}

bool BaseTrie::Retrieve(const char *key, void **value)
{
	return sm_trie_retrieve(m_pTrie, key, value);
}

bool BaseTrie::Delete(const char *key)
{
	return sm_trie_delete(m_pTrie, key);
}

void BaseTrie::Clear()
{
	sm_trie_clear(m_pTrie);
}

void BaseTrie::Destroy()
{
	delete this;
}

bool BaseTrie::Replace(const char *key, void *value)
{
	return sm_trie_replace(m_pTrie, key, value);
}
