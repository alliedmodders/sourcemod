#ifndef _INCLUDE_SOURCEMOD_ADT_FACTORY_H_
#define _INCLUDE_SOURCEMOD_ADT_FACTORY_H_

#include <IShareSys.h>

#define SMINTERFACE_ADTFACTORY_NAME		"IADTFactory"
#define SMINTERFACE_ADTFACTORY_VERSION	2

/**
* @file IADTFactory.h
* @brief Creates abstract data types.
*/

namespace SourceMod
{
	/**
	 * @brief A "Trie" data type.
	 */
	class IBasicTrie
	{
	public:
		/**
		 * @brief Inserts a key/value pair.
		 *
		 * @param key		Key string (null terminated).
		 * @param value		Value pointer (may be anything).
		 * @return			True on success, false if key already exists.
		 */
		virtual bool Insert(const char *key, void *value) =0;

		/**
		 * @brief Retrieves the value of a key.
		 *
		 * @param key		Key string (null terminated).
		 * @param value		Optional pointer to store value pointer.
		 * @return			True on success, false if key was not found.
		 */
		virtual bool Retrieve(const char *key, void **value) =0;

		/**
		 * @brief Deletes a key.
		 *
		 * @param key		Key string (null terminated).
		 * @return			True on success, false if key was not found.
		 */
		virtual bool Delete(const char *key) =0;

		/**
		 * @brief Flushes the entire trie of all keys.
		 */
		virtual void Clear() =0;

		/**
		 * @brief Destroys the IBasicTrie object and frees all associated 
		 * memory.
		 */
		virtual void Destroy() =0;

		/**
		 * @brief Inserts a key/value pair, replacing an old inserted
		 * value if it already exists.
		 *
		 * @param key		Key string (null terminated).
		 * @param value		Value pointer (may be anything).
		 * @return			True on success, false on failure.
		 */
		virtual bool Replace(const char *key, void *value) =0;
	};

	class IADTFactory : public SMInterface
	{
	public:
		/**
		 * @brief Creates a basic Trie object.
		 *
		 * @return			A new IBasicTrie object which must be destroyed
		 *					via IBasicTrie::Destroy().
		 */
		virtual IBasicTrie *CreateBasicTrie() =0;
	};
}

#endif //_INCLUDE_SOURCEMOD_ADT_FACTORY_H_
