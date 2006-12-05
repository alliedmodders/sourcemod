#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "sm_trie.h"

/**
 * Double Array Trie algorithm, based on:
 * An Efficient Implementation of Trie Structures, by
 *  Jun-ichi Aoe and Katsushi Maromoto, and Takashi Sato
 * from Sofiware - Practice and Experience, Vol. 22(9), 695-721 (September 1992)
 *
 *  A Trie is a simple data structure which stores strings as DFAs, with each 
 * transition state being a string entry.  For example, observe the following strings:
 *
 * BAILOPAN, BAT, BACON, BACK
 *  These transition as the follow production rules:
 *  B -> ...                  B
 *       A -> ...             BA
 *            I -> ...        BAI
 *                 LOPAN      BAILOPAN
 *            T -> ...        BAT
 *            C ->            BAC
 *                 O -> ...   BACO
 *                      N     BACON
 *                 K          BACK
 *
 *  The standard implementation for this - using lists - gives a slow linear lookup, somewhere between
 * O(N+M) or O(log n).  A faster implementation is proposed in the paper above, which is based on compacting
 * the transition states into two arrays.  In the paper's implementation, two arrays are used, and thus it is
 * called the "Double Array" algorithm.  However, the CHECK array's size is maintained the same as BASE, 
 * so they can be combined into one structure.  The array seems complex at first, but is very simple: it is a
 * tree structure flattened out into a single vector.  I am calling this implementation the Flat Array Trie.
 *
 *  BASE[] is an array where each member is a node in the Trie.  The node can either be UNUSED (empty), an ARC
 * (containing an offset to the next set of ARCs), or a TERMINATOR (contains the rest of a string).
 * Each node has an index which must be interpeted based on the node type.  If the node is a TERMINATOR, then the
 * index is an index into a string table, to find the rest of the string.  
 *  If the node is an ARC, the index is another index into BASE.  For each possible token that can follow the
 * current token, the value of those tokens can be added to the index given in the ARC.  Thus, given a current
 * position and the next desired token, the current arc will jump to another arc which can contain either:
 *   1) An invalid production (collision, no entry exists)
 *   2) An empty production (no entry exists)
 *   3) Another arc label (the string ends here or continues into more productions)
 *   4) A TERMINATOR (the string ends here and contains an unused set of productions)
 * 
 *  So, given current offset N (starting at N=1), jumping to token C means the next offset will be:
 *      offs = BASE[n] + C
 *  Thus, the next node will be at:
 *      BASE[BASE[n] + C]
 * 
 *  This allows each ARC to specify the base offset for any of its ARC children, like a tree.  Each node specifies
 * its parent ARC -- so if an invalid offset is specified, the parent will not match, and thus no such derived 
 * string exists.
 *
 *  This means that arrays can be laid out "sparsely," maximizing their usage.  Note that N need not be related to
 * the range of tokens (1-256).  I.e., a base index does not have to be at 1, 256, 512, et cetera.  This is because
 * insertion comes with a small deal of complexity.  To insert a new set of tokens T, the algorithm finds a new 
 * BASE index N such that BASE[N+T[i]] is unused for each T[i].  Thus, indirection is not necessarily linear; 
 * traversing a chain of ARC nodes can _and will_ jump around BASE.
 *
 *  Of course, given this level of flexibility in the array organization, there are collisions.  This is largely 
 * where insertions become slow, as the old chain must be relocated before the new one is used.  Relocation means 
 * finding one or more new base indexes, and this means traversing BASE until an acceptable index is found, such 
 * that each offset is unused (see description in previous paragraph).
 *
 *  However, it is not insertion time we are concerned about.  The "trie" name comes from reTRIEval.  We are only
 * concerned with lookup and deletion.  Both lookup and deletion are O(k), where k is relative to the length of the 
 * input string.  Note that it is best case O(1) and worst case O(k).  Deleting the entire trie is always O(1).
 */

/**
 * Optimization ideas for the future:
 * 1) Store a reference count for each arc, with the count of sub-children.
 *    This could let us break out of "children searches" for case 4 easily.
 * 2) Use a 'free list' so we can easily search the trie for free points.
 *    This would drastically speed up x_check*
 */

enum NodeType
{
	Node_Unused = 0,		/* Node is not being used (sparse) */
	Node_Arc,				/* Node is part of an arc and does not terminate */
	Node_Term,				/* Node is a terminator */
};

struct TrieNode
{
	/**
	 * For Node_Arc, this index stores the 'base' offset to the next arc chain.
	 *   I.e. to jump from this arc to character C, it will be at base[idx+C].
	 * For Node_Term, this is an index into the string table.
	 */
	unsigned int idx;

	/**
	 * This contains the prior arc that we must have come from.
	 * For example, if arc 63 has a base jump of index 12, and we want to see if
	 * there is a valid character C, the parent of 12+C must be 63.
	 */
	unsigned int parent;

	void *value;			/* Value associated with this node */
	NodeType mode;			/* Current usage type of the node */
	bool valset;			/* Whether or not a value is set */
};

struct Trie
{
	TrieNode *base;			/* Base array for the sparse tables */
	char *stringtab;		/* String table pointer */
	unsigned int baseSize;	/* Size of the base array, in members */
	unsigned int stSize;	/* Size of the string table, in bytes */
	unsigned int tail;		/* Current unused offset into the string table */
};

inline unsigned char charval(char c)
{
	unsigned char _c = (unsigned char)c;
	return _c - 'A' + 2;
}

unsigned int x_check(Trie *trie, char c)
{
	TrieNode *base = trie->base;
	unsigned char _c = charval(c);
	unsigned int to_check = trie->baseSize - _c;
	for (unsigned int i=1; i<to_check; i++)
	{
		if (base[i+_c].mode == Node_Unused)
		{
			return i;
		}
	}

	trie->base = (TrieNode *)realloc(trie->base, trie->baseSize * sizeof(TrieNode) * 2);
	memset(trie->base + trie->baseSize, 0, trie->baseSize * sizeof(TrieNode));
	to_check = trie->baseSize;
	trie->baseSize *= 2;

	return to_check;
}

unsigned int x_check2(Trie *trie, char c1, char c2)
{
	TrieNode *base = trie->base;
	unsigned char _c1 = charval(c1);
	unsigned char _c2 = charval(c2);
	unsigned int to_check = trie->baseSize - (_c1 > _c2 ? _c1 : _c2);
	for (unsigned int i=1; i<to_check; i++)
	{
		if (base[i+_c1].mode == Node_Unused
			&& base[i+_c2].mode == Node_Unused)
		{
			return i;
		}
	}

	trie->base = (TrieNode *)realloc(trie->base, trie->baseSize * sizeof(TrieNode) * 2);
	memset(trie->base + trie->baseSize, 0, trie->baseSize * sizeof(TrieNode));
	to_check = trie->baseSize;
	trie->baseSize *= 2;

	return to_check;
}

unsigned int x_check_multi(Trie *trie, 
							unsigned int offsets[], 
							unsigned int count)
{
	TrieNode *base = trie->base;
	TrieNode *cur;
	unsigned int to_check = trie->baseSize;
	for (unsigned int i=1; i<to_check; i++)
	{
		bool okay = true;
		for (unsigned int j=0; j<count; j++)
		{
			cur = &base[i+offsets[j]];
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

	trie->base = (TrieNode *)realloc(trie->base, trie->baseSize * sizeof(TrieNode) * 2);
	memset(trie->base + trie->baseSize, 0, trie->baseSize * sizeof(TrieNode));
	to_check = trie->baseSize;
	trie->baseSize *= 2;

	return to_check;
}

unsigned int x_addstring(Trie *trie, const char *ptr)
{
	size_t len = strlen(ptr) + 1;

	if (len > trie->stSize)
	{		
		while (trie->tail + len >= trie->stSize)
		{
			trie->stSize *= 2;
		}
		trie->stringtab = (char *)realloc(trie->stringtab, trie->stSize);
	}

	unsigned int tail = trie->tail;
	strcpy(&trie->stringtab[tail], ptr);
	trie->tail += len;

	return tail;
}

Trie *sm_trie_create()
{
	Trie *t = new Trie;

	t->base = (TrieNode *)malloc(sizeof(TrieNode) * 256);
	t->stringtab = (char *)malloc(sizeof(char) * 256);
	t->baseSize = 256;
	t->stSize = 256;
	t->tail = 0;

	memset(t->base, 0, sizeof(TrieNode) * 256);
	memset(t->stringtab, 0, sizeof(char) * 256);

	/* Sentinel root node */
	t->base[1].idx = 1;
	t->base[1].mode = Node_Arc;
	t->base[1].parent = 1;

	return t;
}

void sm_trie_destroy(Trie *trie)
{
	free(trie->base);
	free(trie->stringtab);
	free(trie);
}

bool sm_trie_retrieve(Trie *trie, const char *key, void **value)
{
	unsigned int lastidx = 1;		/* the last node index */
	unsigned int curidx;			/* current node index */
	const char *keyptr = key;		/* input stream at current token */
	TrieNode *node = NULL;			/* current node being processed */
	TrieNode *base = trie->base;

	if (!*key)
	{
		return false;
	}

	/* Start traversing at the root node */
	do
	{
		/* Find where the next character is, then advance */
		curidx = base[lastidx].idx;
		node = &base[curidx];
		curidx += charval(*keyptr);
		node = &base[curidx];
		keyptr++;

		/* Check if this slot is supposed to be empty or is a collision */
		if (node->mode == Node_Unused || node->parent != lastidx)
		{
			return false;
		} else if (node->mode == Node_Term) {
			char *term = &trie->stringtab[node->idx];
			if (strcmp(keyptr, term) == 0)
			{
				break;
			}
		}
		lastidx = curidx;
	} while (*keyptr != '\0');

	assert(node != NULL);

	if (!node->valset)
	{
		return false;
	}

	if (value)
	{
		*value = node->value;
	}

	return true;
}

bool sm_trie_insert(Trie *trie, const char *key, void *value)
{
	unsigned int lastidx = 1;		/* the last node index */
	unsigned int curidx;			/* current node index */
	const char *keyptr = key;		/* input stream at current token */
	TrieNode *node = NULL;			/* current node being processed */
	TrieNode *basenode = NULL;		/* current base node being processed */
	unsigned int q;					/* temporary var for x_check results */
	TrieNode *base = trie->base;
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
		curidx = base[lastidx].idx;
		basenode = &base[curidx];
		curoffs = charval(*keyptr);
		curidx += curoffs;
		node = &base[curidx];
		keyptr++;

		/* Check if this slot is supposed to be empty.  If so, we need to handle CASES 1/2:
		 * Insertion without collisions
		 */
		if (node->mode == Node_Unused)
		{
			/* Assign backcheck reference */
			node->parent = lastidx;
			node->idx = x_addstring(trie, keyptr);
			node->mode = Node_Term;
			node->valset = true;
			node->value = value;
			
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
			TrieNode *cur;
			
			/* Find every node arcing from the last node.
			 * I.e. for BACHELOR, BADGE, BABY,
			 * The arcs leaving A will be C and D, but our current node is B -> *.
			 * Thus, we use the last index (A) to find the base for arcs leaving A.
			 */
			unsigned int outgoing_base = base[lastidx].idx;
			unsigned int outgoing_list[256];
			unsigned int outgoing_count = 0;	/* count the current index here */
			cur = &base[outgoing_base] + 1;
			for (unsigned int i=1; i<=255; i++,cur++)
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
			unsigned int incoming_list[256];
			unsigned int incoming_base = base[node->parent].idx;
			unsigned int incoming_count = 0;
			cur = &base[incoming_base] + 1;
			for (unsigned int i=1; i<=255; i++,cur++)
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
				unsigned int q = x_check_multi(trie, incoming_list, incoming_count);

				/* If we're incoming, we need to modify our parent */
				base[incoming_base].idx = q;

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
					base[newidx] = base[oldidx];
					memset(&base[oldidx], 0, sizeof(TrieNode));
					/* If we are not a terminator, we have children we must take care of */
					if (base[newidx].mode == Node_Arc)
					{
						TrieNode *check_base = &base[base[newidx].idx] + 1;
						for (unsigned int i=1; i<=255; i++, check_base++)
						{
							if (check_base->parent == oldidx)
							{
								check_base->parent = newidx;
							}
						}
					}
				}
			} else {
				unsigned int q = x_check_multi(trie, outgoing_list, outgoing_count);

				/* If we're outgoing, we need to modify our own base */
				base[lastidx].idx = q;

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
					base[newidx] = base[oldidx];
					memset(&base[oldidx], 0, sizeof(TrieNode));
					/* If we are not a terminator, we have children we must take care of */
					if (base[newidx].mode == Node_Arc)
					{
						TrieNode *check_base = &base[base[newidx].idx] + 1;
						for (unsigned int i=1; i<=255; i++, check_base++)
						{
							if (check_base->parent == oldidx)
							{
								check_base->parent = newidx;
							}
						}
					}
				}

				/* Take the invisible node and use it as our new node */
				node = &base[q + outgoing_list[outgoing_count]];
			}

			/* We're finally done! */
			node->parent = lastidx;
			if (*keyptr == '\0')
			{
				node->mode = Node_Arc;
			} else {
				node->idx = x_addstring(trie, keyptr);
				node->mode = Node_Term;
			}
			node->valset = true;
			node->value = value;

			return true;
		} else {
			/* See what's in the next node - special case if terminator! */
			if (node->mode == Node_Term)
			{
				/* If we're a terminator, we need to handle CASE 3:
				 * Insertion when a terminating collision occurs
				 */
				char *term = &trie->stringtab[node->idx];
				/* Do an initial browsing to make sure they're not the same string */
				if (strcmp(keyptr, term) == 0)
				{
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
				void *oldvalue = node->value;
				bool oldvalset = node->valset;
				while (*term == *keyptr)
				{
					/* Find the next free slot in the check array.
					 * This is the "vector base" essentially
					 */
					q = x_check(trie, *term);
					/* Point the node to the next new base */
					node->idx = q;
					node->mode = Node_Arc;
					node->valset = false;
					/* Advance the input stream and local variables */
					lastidx = curidx;
					curidx = q + charval(*term);
					node = &base[curidx];
					/* Make sure the new current node has its parent set. */
					node->parent = lastidx;
					node->mode = Node_Arc;	/* Just in case we run x_check again */
					*term = '\0';	/* Unmark the string table here */
					term++;
					keyptr++;
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
					node->value = oldvalue;

					/* The terminator was split up, but pieces of keyptr remain.
					 * We need to generate a new production, in this example, R -> H,
					 * with H being a terminator to OUSE.  Thus we get:
					 * B,A,D,G,E,R*,H*->OUSE (* = value set).
					 * NOTE: parent was last set at the end of the while loop.
					 */
					q = x_check(trie, *keyptr);
					node->idx = q;
					node->mode = Node_Arc;
					lastidx = curidx;
					/* Finish the final node */
					curidx = q + charval(*keyptr);
					node = &trie->base[curidx];
					keyptr++;
					/* Optimize - don't add to string table if there's nothing more to eat */
					if (*keyptr == '\0')
					{
						node->mode = Node_Arc;
					} else {
						node->idx = x_addstring(trie, keyptr);
						node->mode = Node_Term;
					}
					node->parent = lastidx;
					node->valset = true;
					node->value = value;
				} else if (*keyptr == '\0') {	//EX: BADGER added over B -> ADGERHOUSE
					/* First backpatch the current node - it ends newly split input string.
					 * This is the exact opposite of the above procedure.
					 */
					node->valset = true;
					node->value = value;

					q = x_check(trie, *term);
					node->idx = q;
					node->mode = Node_Arc;
					lastidx = curidx;
					/* Finish the final node */
					curidx = q + charval(*term);
					node = &trie->base[curidx];
					term++;
					/* Optimize - don't add to string table if there's nothing more to eat */
					if (*term == '\0')
					{
						node->mode = Node_Arc;
					} else {
						node->idx = (term - trie->stringtab); /* Already in the string table! */
						node->mode = Node_Term;
					}
					node->parent = lastidx;
					node->valset = oldvalset;
					node->value = oldvalue;
				} else {
					/* Finally, we have to create two new nodes instead of just one. */
					node->mode = Node_Arc;
					
					q = x_check2(trie, *keyptr, *term);
					node->idx = q;
					lastidx = curidx;

					/* Re-create the old terminated node */
					curidx = q + charval(*term);
					node = &trie->base[curidx];
					term++;
					node->valset = oldvalset;
					node->value = oldvalue;
					node->parent = lastidx;
					if (*term == '\0')
					{
						node->mode = Node_Arc;
					} else {
						node->mode = Node_Term;
						node->idx = (term - trie->stringtab); /* Already in the string table! */
					}

					/* Create the new keyed input node */
					curidx = q + charval(*keyptr);
					node = &trie->base[curidx];
					keyptr++;
					node->valset = true;
					node->value = value;
					node->parent = lastidx;
					if (*keyptr == '\0')
					{
						node->mode = Node_Arc;
					} else {
						node->mode = Node_Term;
						node->idx = x_addstring(trie, keyptr);
					}
				}

				/* Phew! */
				return true;
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

	/* Furthermore, if this is the last arc label in an arc, we should have a value set. */
	if (node->idx == 0)
	{
		assert(node->valset == true);
	}

	if (!node->valset)
	{
		/* Insert is only possible if we have no production */
		node->valset = true;
		node->value = value;
		return true;
	}

	return false;
}
