#ifndef _INCLUDE_SOURCEMOD_NATIVE_OWNER_H_
#define _INCLUDE_SOURCEMOD_NATIVE_OWNER_H_

#include <sp_vm_types.h>
#include <sh_list.h>

class CPlugin;
struct NativeEntry;

struct WeakNative
{
	WeakNative(CPlugin *plugin, uint32_t index) : 
		pl(plugin), idx(index), entry(NULL)
	{
		pl = plugin;
		idx = index;
	}
	WeakNative(CPlugin *plugin, uint32_t index, NativeEntry *pEntry) : 
		pl(plugin), idx(index), entry(pEntry)
	{
		pl = plugin;
		idx = index;
	}
	CPlugin *pl;
	uint32_t idx;
	NativeEntry *entry;
};

using namespace SourceHook;

class CNativeOwner
{
public:
	virtual void DropEverything();
public:
	void AddNatives(const sp_nativeinfo_t *info);
public:
	void SetMarkSerial(unsigned int serial);
	unsigned int GetMarkSerial();
	void PropogateMarkSerial(unsigned int serial);
public:
	void AddDependent(CPlugin *pPlugin);
	void AddWeakRef(const WeakNative & ref);
	void DropRefsTo(CPlugin *pPlugin);
	void AddReplacedNative(NativeEntry *pEntry);
private:
	void DropWeakRefsTo(CPlugin *pPlugin);
	void UnbindWeakRef(const WeakNative & ref);
protected:
	List<CPlugin *> m_Dependents;
	unsigned int m_nMarkSerial;
	List<WeakNative> m_WeakRefs;
	List<NativeEntry *> m_Natives;
	List<NativeEntry *> m_ReplacedNatives;
};

#endif //_INCLUDE_SOURCEMOD_NATIVE_OWNER_H_
