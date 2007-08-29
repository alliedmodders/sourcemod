/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_TEMPLATED_TRIE_H_
#define _INCLUDE_SOURCEMOD_TEMPLATED_TRIE_H_

#include "sm_trie.h"

/**
 * See sm_trie.h for the main implementation, this is a quick templatized version.
 */

template <typename K>
class KTrie
{
private:
	class KTrieNode
	{
		friend class KTrie;
	private:
		unsigned int idx;
		unsigned int parent;
		K value;				/* Value associated with this node */
		NodeType mode;			/* Current usage type of the node */
		bool valset;			/* Whether or not a value is set */
	};
public:
	KTrie()
	{
		m_base = (KTrieNode *)malloc(sizeof(KTrieNode) * (256 + 1));
		m_stringtab = (char *)malloc(sizeof(char) * 256);
		m_baseSize = 256;
		m_stSize = 256;

		internal_clear();
	}
	~KTrie()
	{
		run_destructors();
		free(m_base);
		free(m_stringtab);
	}
public:
	void clear()
	{
		run_destructors();
		internal_clear();
	}
	bool remove(const char *key)
	{
		KTrieNode *node = internal_retrieve(key);
		if (!node || !node->valset)
		{
			return false;
		}

		node->value.~K();
		node->valset = false;

		return true;
	}
	K * retrieve(const char *key)
	{
		KTrieNode *node = internal_retrieve(key);
		if (!node || !node->valset)
		{
			return NULL;
		}
		return &node->value;
	}
	bool replace(const char *key, const K & obj)
	{
		KTrieNode *prev_node = internal_retrieve(key);
		if (!prev_node)
		{
			return insert(key, obj);
		}

		if (prev_node->valset)
		{
			prev_node->value.~K();
		}

		new (&prev_node->value) K(obj);

		return true;
	}
	bool insert(const char *key, const K & obj)
	{
		unsigned int lastidx = 1;		/* the last node index */
		unsigned int curidx;			/* current node index */
		const char *keyptr = key;		/* input stream at current token */
		KTrieNode *node = NULL;			/* current node being processed */
		KTrieNode *basenode = NULL;		/* current base node being processed */
		unsigned int q;					/* temporary var for x_check results */
		unsigned int curoffs;			/* current offset */

		/* Do not handle empty strings for simplicity */
		if (!*key)
		{
			return false;
		}

		/* Start traversing at the root node (1) */
		do
		{
			/* Find where the next character is, then advance */
			curidx = m_base[lastidx].idx;
			basenode = &m_base[curidx];
			curoffs = charval(*keyptr);
			curidx += curoffs;
			node = &m_base[curidx];
			keyptr++;

			/* Check if this slot is supposed to be empty.  If so, we need to handle CASES 1/2:
			 * Insertion without collisions
			 */
			if ( (curidx > m_baseSize) || (node->mode == Node_Unused) )
			{
				if (curidx > m_baseSize)
				{
					if (!grow())
					{
						return false;
					}
					node = &m_base[curidx];
				}
				node->parent = lastidx;
				if (*keyptr == '\0')
				{
					node->mode = Node_Arc;
				} else {
					node->idx = x_addstring(keyptr);
					node->mode = Node_Term;
				}
				node->valset = true;
				new (&node->value) K(obj);

				return true;
			} else if (node->parent != lastidx) {
				/* Collision! We have to split up the tree here.  CASE 4:
				 * Insertion when a new word is inserted with a collision.
				 * NOTE: This is the hardest case to handle.  All below examples are based on:
				 * BACHELOR, BADGE, inserting BABY.
				 * The problematic production here is A -> B, where B is already being used.
			     *
				 * This process has to rotate one half of the 'A' arc.  We generate two lists:
				 *  Outgoing Arcs - Anything leaving this 'A'
				 *  Incoming Arcs - Anything going to this 'A'
				 * Whichever list is smaller will be moved.  Note that this works because the intersection
				 * affects both arc chains, and moving one will make the slot available to either.
				 */
				KTrieNode *cur;

				/* Find every node arcing from the last node.
				 * I.e. for BACHELOR, BADGE, BABY,
				 * The arcs leaving A will be C and D, but our current node is B -> *.
				 * Thus, we use the last index (A) to find the base for arcs leaving A.
				 */
				unsigned int outgoing_base = m_base[lastidx].idx;
				unsigned int outgoing_list[256];
				unsigned int outgoing_count = 0;	/* count the current index here */
				cur = &m_base[outgoing_base] + 1;
				unsigned int outgoing_limit = 255;

				if (outgoing_base + outgoing_limit > m_baseSize)
				{
					outgoing_limit = m_baseSize - outgoing_base;
				}

				for (unsigned int i=1; i<=outgoing_limit; i++,cur++)
				{
					if (cur->mode == Node_Unused || cur->parent != lastidx)
					{
						continue;
					}
					outgoing_list[outgoing_count++] = i;
				}
				outgoing_list[outgoing_count++] = curidx - outgoing_base;

				/* Now we need to find all the arcs leaving our parent...
				 * Note: the inconsistency is the base of our parent.  
				 */
				assert(m_base[node->parent].mode == Node_Arc);
				unsigned int incoming_list[256];
				unsigned int incoming_base = m_base[node->parent].idx;
				unsigned int incoming_count = 0;
				unsigned int incoming_limit = 255;
				cur = &m_base[incoming_base] + 1;

				if (incoming_base + incoming_limit > m_baseSize)
				{
					incoming_limit = m_baseSize - incoming_base;
				}

				assert(incoming_limit > 0 && incoming_limit <= 255);

				for (unsigned int i=1; i<=incoming_limit; i++,cur++)
				{
					if (cur->mode == Node_Arc || cur->mode == Node_Term)
					{
						if (cur->parent == node->parent)
						{
							incoming_list[incoming_count++] = i;
						}
					}
				}

				if (incoming_count < outgoing_count + 1)
				{
					unsigned int q = x_check_multi(incoming_list, incoming_count);

					node = &m_base[curidx];

					/* If we're incoming, we need to modify our parent */
					m_base[node->parent].idx = q;

					/* For each node in the "to move" list,
					 * Relocate the node's info to the new position.
					 */
					unsigned int idx, newidx, oldidx;
					for (unsigned int i=0; i<incoming_count; i++)
					{
						idx = incoming_list[i];
						newidx = q + idx;
						oldidx = incoming_base + idx;
						if (oldidx == lastidx)
						{
							/* Important! Make sure we're not invalidating our sacred lastidx */
							lastidx = newidx;
						}
						/* Fully copy the node */
						memcpy(&m_base[newidx], &m_base[oldidx], sizeof(KTrieNode));
						if (m_base[oldidx].valset)
						{
							new (&m_base[newidx].value) K(m_base[oldidx].value);
							m_base[oldidx].value.~K();
						}
						assert(m_base[m_base[newidx].parent].mode == Node_Arc);
						/* Erase old data */
						memset(&m_base[oldidx], 0, sizeof(KTrieNode));
						/* If we are not a terminator, we have children we must take care of */
						if (m_base[newidx].mode == Node_Arc)
						{
							KTrieNode *check_base = &m_base[m_base[newidx].idx] + 1;
							outgoing_limit = (m_base + m_baseSize + 1) - check_base;
							if (outgoing_limit > 255)
							{
								outgoing_limit = 255;
							}
							for (unsigned int j=1; j<=outgoing_limit; j++, check_base++)
							{
								if (check_base->parent == oldidx)
								{
									check_base->parent = newidx;
								}
							}
						}
					}
				} else {
					unsigned int q = x_check_multi(outgoing_list, outgoing_count);

					node = &m_base[curidx];

					/* If we're outgoing, we need to modify our own base */
					m_base[lastidx].idx = q;

					/* Take the last index (curidx) out of the list.  Technically we are not moving this,
					 * since it's already being used by something else.  
					 */
					outgoing_count--;

					/* For each node in the "to move" list,
					 * Relocate the node's info to the new position.
					 */
					unsigned int idx, newidx, oldidx;
					for (unsigned int i=0; i<outgoing_count; i++)
					{
						idx = outgoing_list[i];
						newidx = q + idx;
						oldidx = outgoing_base + idx;
						if (oldidx == lastidx)
						{
							/* Important! Make sure we're not invalidating our sacred lastidx */
							lastidx = newidx;
						}
						/* Fully copy the node */
						memcpy(&m_base[newidx], &m_base[oldidx], sizeof(KTrieNode));
						if (m_base[oldidx].valset)
						{
							new (&m_base[newidx].value) K(m_base[oldidx].value);
							m_base[oldidx].value.~K();
						}
						assert(m_base[m_base[newidx].parent].mode == Node_Arc);
						/* Erase old data */
						memset(&m_base[oldidx], 0, sizeof(KTrieNode));
						/* If we are not a terminator, we have children we must take care of */
						if (m_base[newidx].mode == Node_Arc)
						{
							KTrieNode *check_base = &m_base[m_base[newidx].idx] + 1;
							outgoing_limit = (m_base + m_baseSize + 1) - check_base;
							if (outgoing_limit > 255)
							{
								outgoing_limit = 255;
							}
							for (unsigned int j=1; j<=outgoing_limit; j++, check_base++)
							{
								if (check_base->parent == oldidx)
								{
									check_base->parent = newidx;
								}
							}
						}
					}

					/* Take the invisible node and use it as our new node */
					node = &m_base[q + outgoing_list[outgoing_count]];
				}

				/* We're finally done! */
				node->parent = lastidx;
				if (*keyptr == '\0')
				{
					node->mode = Node_Arc;
				} else {
					node->idx = x_addstring(keyptr);
					node->mode = Node_Term;
				}
				node->valset = true;
				new (&node->value) K(obj);

				return true;
			} else {
				/* See what's in the next node - special case if terminator! */
				if (node->mode == Node_Term)
				{
					/* If we're a terminator, we need to handle CASE 3:
					 * Insertion when a terminating collision occurs
					 */
					char *term = &m_stringtab[node->idx];
					/* Do an initial browsing to make sure they're not the same string */
					if (strcmp(keyptr, term) == 0)
					{
						if (!node->valset)
						{
							node->valset = true;
							new (&node->value) K(obj);
							return true;
						}
						/* Same string.  We can't insert. */
						return false;
					}
					/* For each matching character pair, we need to disband the terminator.
					 * This splits the similar prefix into a single arc path.
					 * First, save the old values so we can move them to a new node.
					 * Next, for each loop:
					 *  Take the current (invalid) node, and point it to the next arc base.
					 *  Set the current node to the node at the next arc.
					 */
					K oldvalue;
					bool oldvalset = node->valset;
					if (oldvalset)
					{
						oldvalue = node->value;
					}
					if (*term == *keyptr)
					{
						while (*term == *keyptr)
						{
							/* Find the next free slot in the check array.
							 * This is the "vector base" essentially
							 */
							q = x_check(*term);
							node = &m_base[curidx];
							/* Point the node to the next new base */
							node->idx = q;
							node->mode = Node_Arc;
							if (node->valset == true)
							{
								node->value.~K();
								node->valset = false;
							}
							/* Advance the input stream and local variables */
							lastidx = curidx;
							curidx = q + charval(*term);
							node = &m_base[curidx];
							/* Make sure the new current node has its parent set. */
							node->parent = lastidx;
							node->mode = Node_Arc;	/* Just in case we run x_check again */
							*term = '\0';	/* Unmark the string table here */
							term++;
							keyptr++;
						}
					} else if (node->valset) {
						node->valset = false;
						node->value.~K();
					}
					/* We're done inserting new pairs.  If one of them is exhausted,
					 * we take special shortcuts.
					 */
					if (*term == '\0')				//EX: BADGERHOUSE added over B -> ADGER.  
					{
						/* First backpatch the current node - it ends the newly split terminator.
						 * In the example, this would mean the node is the production from R -> ?
						 * This node ends the old BADGER, so we set it here.
						 */
						node->valset = oldvalset;
						if (node->valset)
						{
							new (&node->value) K(oldvalue);
						}

						/* The terminator was split up, but pieces of keyptr remain.
						 * We need to generate a new production, in this example, R -> H,
						 * with H being a terminator to OUSE.  Thus we get:
						 * B,A,D,G,E,R*,H*->OUSE (* = value set).
						 * NOTE: parent was last set at the end of the while loop.
						 */
						/* Get the new base and apply re-basing */
						q = x_check(*keyptr);
						node = &m_base[curidx];

						node->idx = q;
						node->mode = Node_Arc;
						lastidx = curidx;
						/* Finish the final node */
						curidx = q + charval(*keyptr);
						node = &m_base[curidx];
						keyptr++;
						/* Optimize - don't add to string table if there's nothing more to eat */
						if (*keyptr == '\0')
						{
							node->mode = Node_Arc;
						} else {
							node->idx = x_addstring(keyptr);
							node->mode = Node_Term;
						}
						node->parent = lastidx;
						node->valset = true;
						new (&node->value) K(obj);
					} else if (*keyptr == '\0') {	//EX: BADGER added over B -> ADGERHOUSE
						/* First backpatch the current node - it ends newly split input string.
						 * This is the exact opposite of the above procedure.
						 */
						node->valset = true;
						new (&node->value) K(obj);

						/* Get the new base and apply re-basing */
						q = x_check(*term);
						node = &m_base[curidx];

						node->idx = q;
						node->mode = Node_Arc;
						lastidx = curidx;
						/* Finish the final node */
						curidx = q + charval(*term);
						node = &m_base[curidx];
						term++;
						/* Optimize - don't add to string table if there's nothing more to eat */
						if (*term == '\0')
						{
							node->mode = Node_Arc;
						} else {
							node->idx = (term - m_stringtab); /* Already in the string table! */
							node->mode = Node_Term;
						}
						node->parent = lastidx;
						node->valset = oldvalset;
						if (node->valset)
						{
							new (&node->value) K(oldvalue);
						}
					} else {
						/* Finally, we have to create two new nodes instead of just one. */
						node->mode = Node_Arc;

						/* Get the new base and apply re-basing */
						q = x_check2(*keyptr, *term);
						node = &m_base[curidx];

						node->idx = q;
						lastidx = curidx;

						/* Re-create the old terminated node */
						curidx = q + charval(*term);
						node = &m_base[curidx];
						term++;
						node->valset = oldvalset;
						if (node->valset)
						{
							new (&node->value) K(oldvalue);
						}
						node->parent = lastidx;
						if (*term == '\0')
						{
							node->mode = Node_Arc;
						} else {
							node->mode = Node_Term;
							node->idx = (term - m_stringtab); /* Already in the string table! */
						}

						/* Create the new keyed input node */
						curidx = q + charval(*keyptr);
						node = &m_base[curidx];
						keyptr++;
						node->valset = true;
						new (&node->value) K(obj);
						node->parent = lastidx;
						if (*keyptr == '\0')
						{
							node->mode = Node_Arc;
						} else {
							node->mode = Node_Term;
							node->idx = x_addstring(keyptr);
						}
					}

					/* Phew! */
					return true;
				} else {
					assert(node->mode == Node_Arc);
				}
			}
			lastidx = curidx;
		} while (*keyptr != '\0');

		assert(node);

		/* If we've exhausted the string and we have a valid reached node,
		* the production rule already existed.  Make sure it's valid to set first.
		*/

		/* We have to be an Arc.  If the last result was anything else, we would have returned a new 
		* production earlier.
		*/
		assert(node->mode == Node_Arc);

		if (!node->valset)
		{
			node->valset = true;
			new (&node->value) K(obj);
			return true;
		}

		return false;
	}
private:
	KTrieNode *internal_retrieve(const char *key)
	{
		unsigned int lastidx = 1;		/* the last node index */
		unsigned int curidx;			/* current node index */
		const char *keyptr = key;		/* input stream at current token */
		KTrieNode *node = NULL;			/* current node being processed */

		if (!*key)
		{
			return NULL;
		}

		/* Start traversing at the root node */
		do
		{
			/* Find where the next character is, then advance */
			curidx = m_base[lastidx].idx;
			node = &m_base[curidx];
			curidx += charval(*keyptr);
			node = &m_base[curidx];
			keyptr++;

			/* Check if this slot is supposed to be empty or is a collision */
			if ((curidx > m_baseSize) || node->mode == Node_Unused || node->parent != lastidx)
			{
				return NULL;
			} else if (node->mode == Node_Term) {
				char *term = &m_stringtab[node->idx];
				if (strcmp(keyptr, term) == 0)
				{
					break;
				}
			}
			lastidx = curidx;
		} while (*keyptr != '\0');

		return node;
	}
	bool grow()
	{
		/* The current # of nodes in the tree is trie->baseSize + 1 */
		unsigned int cur_size = m_baseSize;
		unsigned int new_size = cur_size * 2;

		KTrieNode *new_base = (KTrieNode *)malloc((new_size + 1) * sizeof(KTrieNode));
		if (!new_base)
		{
			return false;
		}

		memcpy(new_base, m_base, sizeof(KTrieNode *) * (m_baseSize + 1));
		memset(&new_base[cur_size + 1], 0, (new_size - cur_size) * sizeof(KTrieNode));

		for (size_t i = 0; i <= m_baseSize; i++)
		{
			if (m_base[i].valset)
			{
				/* Placement construct+copy the object, then placement destroy the old. */
				new (&new_base[i].value) K(m_base[i].value);
				m_base[i].value.~K();
			}
		}

		free(m_base);
		m_base = new_base;
		m_baseSize = new_size;

		return true;
	}
	inline unsigned char charval(char c)
	{
		return (unsigned char)c;
	}
	void internal_clear()
	{
		m_tail = 0;

		memset(m_base, 0, sizeof(KTrieNode) * (m_baseSize + 1));
		memset(m_stringtab, 0, sizeof(char) * m_stSize);

		/* Sentinel root node */
		m_base[1].idx = 1;
		m_base[1].mode = Node_Arc;
		m_base[1].parent = 1;
	}
	void run_destructors()
	{
		for (size_t i = 0; i <= m_baseSize; i++)
		{
			if (m_base[i].valset)
			{
				m_base[i].value.~K();
			}
		}
	}
	unsigned int x_addstring(const char *ptr)
	{
		size_t len = strlen(ptr) + 1;

		if (m_tail + len >= m_stSize)
		{		
			while (m_tail + len >= m_stSize)
			{
				m_stSize *= 2;
			}
			m_stringtab = (char *)realloc(m_stringtab,m_stSize);
		}

		unsigned int tail = m_tail;
		strcpy(&m_stringtab[tail], ptr);
		m_tail += len;

		return tail;
	}
	unsigned int x_check(char c, unsigned int start=1)
	{
		unsigned char _c = charval(c);
		unsigned int to_check = m_baseSize - _c;
		for (unsigned int i=start; i<=to_check; i++)
		{
			if (m_base[i+_c].mode == Node_Unused)
			{
				return i;
			}
		}

		grow();

		return x_check(c, to_check+1);
	}
	unsigned int x_check2(char c1, char c2, unsigned int start=1)
	{
		unsigned char _c1 = charval(c1);
		unsigned char _c2 = charval(c2);
		unsigned int to_check = m_baseSize - (_c1 > _c2 ? _c1 : _c2);
		for (unsigned int i=start; i<=to_check; i++)
		{
			if (m_base[i+_c1].mode == Node_Unused
				&& m_base[i+_c2].mode == Node_Unused)
			{
				return i;
			}
		}

		grow();

		return x_check2(c1, c2, to_check+1);
	}
	unsigned int x_check_multi(
		unsigned int offsets[], 
		unsigned int count,
		unsigned int start=1)
	{
		KTrieNode *cur;
		unsigned int to_check = m_baseSize;
		unsigned int highest = 0;

		for (unsigned int i=0; i<count; i++)
		{
			if (offsets[i] > highest)
			{
				highest = offsets[i];
			}
		}

		to_check -= highest;

		for (unsigned int i=start; i<=to_check; i++)
		{
			bool okay = true;
			for (unsigned int j=0; j<count; j++)
			{
				cur = &m_base[i+offsets[j]];
				if (cur->mode != Node_Unused)
				{
					okay = false;
					break;
				}
			}
			if (okay)
			{
				return i;
			}
		}

		grow();

		return x_check_multi(offsets, count, to_check+1);
	}
private:
	KTrieNode *m_base;			/* Base array for the sparse tables */
	char *m_stringtab;			/* String table pointer */
	unsigned int m_baseSize;	/* Size of the base array, in members */
	unsigned int m_stSize;		/* Size of the string table, in bytes */
	unsigned int m_tail;		/* Current unused offset into the string table */
};

#endif //_INCLUDE_SOURCEMOD_TEMPLATED_TRIE_H_
