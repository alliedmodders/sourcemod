/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SM_QUEUE_H
#define _INCLUDE_SM_QUEUE_H

#include <new>
#include <stdlib.h>
#include <sh_stack.h>

using namespace SourceHook;

/*
	A circular, doubly-linked List with one sentinel node

	Empty:
	m_Head = sentinel
	m_Head->next = m_Head;
	m_Head->prev = m_Head;
	One element:
	m_Head = sentinel
	m_Head->next = node1
	m_Head->prev = node1
	node1->next = m_Head
	node1->prev = m_Head
	Two elements:
	m_Head = sentinel
	m_Head->next = node1
	m_Head->prev = node2
	node1->next = node2
	node1->prev = m_Head
	node2->next = m_Head
	node2->prev = node1
*/
template <class T>
class Queue
{
public:
	class iterator;
	friend class iterator;
	class QueueNode
	{
	public:
		T obj;
		QueueNode *next;
		QueueNode *prev;
	};
private:
	// Initializes the sentinel node.
	QueueNode *_Initialize()
	{
		QueueNode *n = (QueueNode *)malloc(sizeof(QueueNode));
		n->next = n;
		n->prev = n;
		return n;
	}
public:
	Queue() : m_Head(_Initialize()), m_Size(0)
	{
	}

	Queue(const Queue &src) : m_Head(_Initialize()), m_Size(0)
	{
		iterator iter;
		for (iter=src.begin(); iter!=src.end(); iter++)
		{
			push_back( (*iter) );
		}
	}

	~Queue()
	{
		clear();

		// Don't forget to free the sentinel
		if (m_Head)
		{
			free(m_Head);
			m_Head = NULL;
		}

		while (!m_FreeNodes.empty())
		{
			free(m_FreeNodes.front());
			m_FreeNodes.pop();
		}
	}

	void push(const T &obj)
	{
		QueueNode *node;
		
		if (m_FreeNodes.empty())
		{
			node = (QueueNode *)malloc(sizeof(QueueNode));
		} else {
			node = m_FreeNodes.front();
			m_FreeNodes.pop();
		}

		/* Copy the object */
		new (&node->obj) T(obj);

		/* Install into the Queue */
		node->prev = m_Head->prev;
		node->next = m_Head;
		m_Head->prev->next = node;
		m_Head->prev = node;

		m_Size++;
	}

	size_t size() const
	{
		return m_Size;
	}

	void clear()
	{
		QueueNode *node = m_Head->next;
		QueueNode *temp;
		m_Head->next = m_Head;
		m_Head->prev = m_Head;

		// Iterate through the nodes until we find g_Head (the sentinel) again
		while (node != m_Head)
		{
			temp = node->next;
			node->obj.~T();
			m_FreeNodes.push(node);
			node = temp;
		}
		m_Size = 0;
	}

	bool empty() const
	{
		return (m_Size == 0);
	}

private:
	QueueNode *m_Head;
	size_t m_Size;
	CStack<QueueNode *> m_FreeNodes;
public:
	class iterator
	{
		friend class Queue;
	public:
		iterator()
		{
			m_This = NULL;
		}
		iterator(const Queue &src)
		{
			m_This = src.m_Head;
		}
		iterator(QueueNode *n) : m_This(n)
		{
		}
		iterator(const iterator &where)
		{
			m_This = where.m_This;
		}
		//pre decrement
		iterator & operator--()
		{
			if (m_This)
				m_This = m_This->prev;
			return *this;
		}
		//post decrement
		iterator operator--(int)
		{
			iterator old(*this);
			if (m_This)
				m_This = m_This->prev;
			return old;
		}	

		//pre increment
		iterator & operator++()
		{
			if (m_This)
				m_This = m_This->next;
			return *this;
		}
		//post increment
		iterator operator++(int)
		{
			iterator old(*this);
			if (m_This)
				m_This = m_This->next;
			return old;
		}

		const T & operator * () const
		{
			return m_This->obj;
		}
		T & operator * ()
		{
			return m_This->obj;
		}

		T * operator -> ()
		{
			return &(m_This->obj);
		}
		const T * operator -> () const
		{
			return &(m_This->obj);
		}

		bool operator != (const iterator &where) const
		{
			return (m_This != where.m_This);
		}
		bool operator ==(const iterator &where) const
		{
			return (m_This == where.m_This);
		}
	private:
		QueueNode *m_This;
	};
public:
	iterator begin() const
	{
		return iterator(m_Head->next);
	}

	iterator end() const
	{
		return iterator(m_Head);
	}

	iterator erase(iterator &where)
	{
		QueueNode *pNode = where.m_This;
		iterator iter(where);
		iter++;


		// Works for all cases: empty Queue, erasing first element, erasing tail, erasing in the middle...
		pNode->prev->next = pNode->next;
		pNode->next->prev = pNode->prev;

		pNode->obj.~T();
		m_FreeNodes.push(pNode);
		m_Size--;

		return iter;
	}
public:
	void remove(const T & obj)
	{
		iterator b;
		for (b=begin(); b!=end(); b++)
		{
			if ( (*b) == obj )
			{
				erase( b );
				break;
			}
		}
	}
	template <typename U>
	iterator find(const U & equ) const
	{
		iterator iter;
		for (iter=begin(); iter!=end(); iter++)
		{
			if ( (*iter) == equ )
			{
				return iter;
			}
		}
		return end();
	}
	Queue & operator =(const Queue &src)
	{
		clear();
		iterator iter;
		for (iter=src.begin(); iter!=src.end(); iter++)
		{
			push_back( (*iter) );
		}
		return *this;
	}
public:
	T & first() const
	{
		iterator i = begin();

		return (*i);
	}

	void pop()
	{
		iterator iter = begin();
		erase(iter);
	}
};

#endif //_INCLUDE_SM_QUEUE_H
