#include <IADTFactory.h>
#include "sm_globals.h"
#include "sm_trie.h"

using namespace SourceMod;

class BaseTrie : public IBasicTrie
{
public:
	BaseTrie();
	virtual ~BaseTrie();
	virtual bool Insert(const char *key, void *value);
	virtual bool Replace(const char *key, void *value);
	virtual bool Retrieve(const char *key, void **value);
	virtual bool Delete(const char *key);
	virtual void Clear();
	virtual void Destroy();
private:
	Trie *m_pTrie;
};

class ADTFactory : 
	public SMGlobalClass,
	public IADTFactory
{
public: //SMInterface
	const char *GetInterfaceName();
	unsigned int GetInterfaceVersion();
public: //SMGlobalClass
	void OnSourceModAllInitialized();
public: //IADTFactory
	IBasicTrie *CreateBasicTrie();
};

