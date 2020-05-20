#ifndef _INCLUDE_CSTRIKE_HASH_H_
#define _INCLUDE_CSTRIKE_HASH_H_

#include <amtl/am-string.h>
#include <amtl/am-hashmap.h>
#include <amtl/am-hashtable.h>

#define MAX_WEAPON_NAME_LENGTH 80
class CEconItemDefinition;

struct HashItemDef_Node
{
	int key;
	CEconItemDefinition *pDef;
	int iunknown;
};

class CHashItemDef
{
public:
#ifdef PLATFORM_X86
	unsigned char padding[36];
#else
	unsigned char padding[56];
#endif
	HashItemDef_Node *pMem;
	int nAllocatedCount;
	int nGrowSize;
	int iunknown;
	int currentElements;
	int maxElements;
};

struct ItemDefHashValue
{
	ItemDefHashValue(int iLoadoutSlot, unsigned int iPrice, SMCSWeapon iWeaponID, unsigned int iDefIdx, const char *szClassname)
	{
		m_iLoadoutSlot = iLoadoutSlot;
		m_iPrice = iPrice;
		m_iDefIdx = iDefIdx;
		m_iWeaponID = iWeaponID;

		Q_strncpy(m_szClassname, szClassname, sizeof(m_szClassname));

		//Create the item name
		Q_strncpy(m_szItemName, GetWeaponNameFromClassname(szClassname), sizeof(m_szItemName));
	}
	int m_iLoadoutSlot;
	SMCSWeapon m_iWeaponID;
	unsigned int m_iPrice;
	unsigned int m_iDefIdx;
	char m_szClassname[MAX_WEAPON_NAME_LENGTH];
	char m_szItemName[MAX_WEAPON_NAME_LENGTH];
};

struct StringPolicy
{
	static inline uint32_t hash(const char *key)
	{
		return ke::FastHashCharSequence(key, strlen(key));
	}
	static inline bool matches(const char *find, const std::string &key)
	{
		return key.compare(find) == 0;
	}
};

struct IntegerPolicy
{
	static inline uint32_t hash(const int key)
	{
		return ke::HashInt32(key);
	}
	static inline bool matches(const int find, const int &key)
	{
		return (key == find);
	}
};

struct WeaponIDPolicy
{
	static inline uint32_t hash(const SMCSWeapon key)
	{
		return ke::HashInt32((int)key);
	}
	static inline bool matches(const SMCSWeapon find, const SMCSWeapon &key)
	{
		return (key == find);
	}
};

typedef ke::HashMap<std::string, ItemDefHashValue, StringPolicy> ClassnameMap;
typedef ke::HashMap<int, ItemDefHashValue, IntegerPolicy> ItemIndexMap;
typedef ke::HashMap<SMCSWeapon, ItemDefHashValue, WeaponIDPolicy> WeaponIDMap;

extern ClassnameMap g_mapClassToDefIdx;
extern ItemIndexMap g_mapDefIdxToClass;
extern WeaponIDMap g_mapWeaponIDToDefIdx;
#endif //_INCLUDE_CSTRIKE_HASH_H_
