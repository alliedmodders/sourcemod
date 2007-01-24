#ifndef _INCLUDE_SOURCEMOD_ADMINCACHE_H_
#define _INCLUDE_SOURCEMOD_ADMINCACHE_H_

#include "sm_memtable.h"
#include <sm_trie.h>
#include <sh_list.h>
#include <IAdminSystem.h>
#include "sm_globals.h"
#include <IForwardSys.h>

using namespace SourceHook;

#define GRP_MAGIC_SET		0xDEADFADE
#define GRP_MAGIC_UNSET		0xFACEFACE

struct AdminGroup
{
	uint32_t magic;					/* Magic flag, for memory validation (ugh) */
	bool immune_global;				/* Global immunity? */
	bool immune_default;			/* Default immunity? */
	/* Immune from target table (-1 = nonexistant)
	 * [0] = number of entries
	 * [1...N] = immune targets
	 */
	int immune_table;			
	Trie *pCmdTable;				/* Command override table (can be NULL) */
	Trie *pCmdGrpTable;				/* Command group override table (can be NULL) */
	int next_grp;					/* Next group in the chain */
	int prev_grp;					/* Previous group in the chain */
	int nameidx;					/* Name */
	bool addflags[AdminFlags_TOTAL];	/* Additive flags */
};

class AdminCache : 
	public IAdminSystem,
	public SMGlobalClass
{
public:
	AdminCache();
	~AdminCache();
public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModShutdown();
public: //IAdminSystem
	/** Command cache stuff */
	void AddCommandOverride(const char *cmd, OverrideType type, AdminFlag flag);
	bool GetCommandOverride(const char *cmd, OverrideType type, AdminFlag *pFlag);
	void UnsetCommandOverride(const char *cmd, OverrideType type);
	/** Group cache stuff */
	GroupId AddGroup(const char *group_name);
	GroupId FindGroupByName(const char *group_name);
	void SetGroupAddFlag(GroupId id, AdminFlag flag, bool enabled);
	bool GetGroupAddFlag(GroupId id, AdminFlag flag);
	unsigned int GetGroupAddFlagBits(GroupId id, bool flags[], unsigned int total);
	void SetGroupGenericImmunity(GroupId id, ImmunityType type, bool enabled);
	bool GetGroupGenericImmunity(GroupId id, ImmunityType type);
	void InvalidateGroup(GroupId id);
	void AddGroupImmunity(GroupId id, GroupId other_id);
	unsigned int GetGroupImmunityCount(GroupId id);
	GroupId GetGroupImmunity(GroupId id, unsigned int number);
	void AddGroupCommandOverride(GroupId id, const char *name, OverrideType type, OverrideRule rule);
	bool GetGroupCommandOverride(GroupId id, const char *name, OverrideType type, OverrideRule *pRule);
	void DumpAdminCache(int cache_flags, bool rebuild);
	void AddAdminListener(IAdminListener *pListener);
	void RemoveAdminListener(IAdminListener *pListener);
private:
	void _AddCommandOverride(const char *cmd, AdminFlag flag);
	void _AddCommandGroupOverride(const char *group, AdminFlag flag);
	bool _GetCommandOverride(const char *cmd, AdminFlag *pFlag);
	bool _GetCommandGroupOverride(const char *group, AdminFlag *pFlag);
	void _UnsetCommandOverride(const char *cmd);
	void _UnsetCommandGroupOverride(const char *group);
	void InvalidateGroupCache();
	void DumpCommandOverrideCache(OverrideType type);
public:
	BaseStringTable *m_pStrings;
	BaseMemTable *m_pMemory;
	Trie *m_pCmdOverrides;
	Trie *m_pCmdGrpOverrides;
	int m_FirstGroup;
	int m_LastGroup;
	int m_FreeGroupList;
	Trie *m_pGroups;
	List<IAdminListener *> m_hooks;
	IForward *m_pCacheFwd;
};

extern AdminCache g_Admins;

#endif //_INCLUDE_SOURCEMOD_ADMINCACHE_H_
