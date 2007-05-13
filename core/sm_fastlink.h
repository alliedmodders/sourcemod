/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */


#ifndef _INCLUDE_SOURCEMOD_FASTLINK_H_
#define _INCLUDE_SOURCEMOD_FASTLINK_H_

template <typename T>
class FastLink
{
	friend class iterator;
public:
	struct FastLinkNode
	{
		unsigned int prev;
		unsigned int next;
		unsigned int freeNode;
		T obj;
	};
public:
	FastLink(unsigned int maxsize) : m_Size(0), m_FirstLink(0), m_FreeNodes(0), m_LastLink(0)
	{
		m_MaxSize = maxsize;
		m_Nodes = new FastLinkNode[m_MaxSize+1];
	}
public:
	bool push_back(const T & obj)
	{
		unsigned int new_node = GetFreeIndex();
		if (!new_node)
		{
			return false;
		}
		m_Nodes[new_node].obj = obj;
		m_Nodes[new_node].next = 0;
		if (!m_FirstLink)
		{
			m_Nodes[new_node].prev = 0;
			m_FirstLink = new_node;
			m_LastLink = new_node;
		} else {
			m_Nodes[new_node].prev = m_LastLink;
			m_Nodes[m_LastLink].next = new_node;
			m_LastLink = new_node;
		}
		m_Size++;
		return true;
	}
	size_t size() const
	{
		return m_Size;
	}
private:
	unsigned int GetFreeIndex()
	{
		if (m_FreeNodes)
		{
			return m_Nodes[m_FreeNodes--].freeNode;
		} else {
			if (m_LastLink >= m_MaxSize)
			{
				return 0;
			}
			return m_LastLink + 1;
		}
	}
public:
	class iterator
	{
		friend class FastLink;
	public:
		iterator()
		{
			link = NULL;
			position = 0;
		}
		iterator(const FastLink *_link, unsigned int _position)
		{
			link = _link;
			position = _position;
		}
	public:
		bool operator ==(const iterator &where) const
		{
			return (link == where.link && position == where.position);
		}
		bool operator !=(const iterator &where) const
		{
			return (link != where.link || position != where.position);
		}
		T & operator *()
		{
			return link->m_Nodes[position].obj;
		}
		const T & operator *() const
		{
			return link->m_Nodes[position].obj;
		}
		iterator & operator++()
		{
			position = link->m_Nodes[position].next;
			return *this;
		}
	private:
		const FastLink *link;
		unsigned int position;
	};
public:
	iterator begin() const
	{
		return iterator(this, m_FirstLink);
	}
	iterator end() const
	{
		return iterator(this, 0);
	}
	void remove(const T & obj)
	{
		for (iterator iter=begin(); iter!=end(); ++iter)
		{
			if ((*iter) == obj)
			{
				erase(iter);
				return;
			}
		}
	}
	iterator erase(const iterator &where)
	{
		/* Whoa, this better be right! */
		assert(where.link == this);

		iterator iter(where.link, where.position);
		++iter;

		unsigned int index = where.position;

		/* Each case is different, we have no sentinel.
		 * CASE: We're the HEAD AND the TAIL */
		if (index == m_FirstLink
			&& index == m_LastLink)
		{
			m_FirstLink = 0;
			m_LastLink = 0;
		}
		/* We're the HEAD */
		else if (index == m_FirstLink)
		{
			m_FirstLink = m_Nodes[index].next;
			m_Nodes[index].prev = 0;
		}
		/* We're the TAIL */
		else if (index == m_LastLink)
		{
			m_LastLink = m_Nodes[index].prev;
			m_Nodes[index].next = 0;
		}
		/* We're in the middle! */
		else
		{
			/* Patch forward reference */
			m_Nodes[m_Nodes[index].next].prev = m_Nodes[index].prev;
			/* Patch backward reference */
			m_Nodes[m_Nodes[index].prev].next = m_Nodes[index].next;
		}

		/* Add us to the free list */
		m_Nodes[++m_FreeNodes].freeNode = index;

		return iter;
	}
private:
	size_t m_Size;
	unsigned int m_FirstLink;
	unsigned int m_FreeNodes;
	unsigned int m_LastLink;
	unsigned int m_MaxSize;
	FastLinkNode *m_Nodes;
};

#endif //_INCLUDE_SOURCEMOD_FASTLINK_H_
