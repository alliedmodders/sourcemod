#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "StackTracker.h"

using namespace SourcePawn;

StackTracker::StackTracker() : m_Entries(NULL), m_NumEntries(0), m_MaxEntries(0)
{
}

StackTracker::~StackTracker()
{
	free(m_Entries);
}

JIns *StackTracker::get(int position)
{
	stack_entry_t *etr;

	if ((etr = internal_get(position)) == NULL)
	{
		return NULL;
	}

	return etr->ins;
}

void StackTracker::set(int position, JIns *ins)
{
	stack_entry_t *etr;

	if ((etr = internal_get(position)) == NULL)
	{
		assert(0);
	}

	etr->ins = ins;
}

void StackTracker::pop(int new_stk)
{
	while (m_NumEntries > 0 && m_Entries[m_NumEntries - 1].pos < new_stk)
	{
		m_NumEntries--;
	}
}

stack_entry_t *StackTracker::internal_get(int pos)
{
	if (m_NumEntries == 0 || pos < m_Entries[m_NumEntries-1].pos)
	{
		stack_entry_t *etr;

		if (m_MaxEntries == 0)
		{
			m_MaxEntries = 8;
			m_Entries = (stack_entry_t *)malloc(sizeof(stack_entry_t) * m_MaxEntries);
		}
		else if (m_NumEntries == m_MaxEntries)
		{
			m_MaxEntries *= 2;
			m_Entries = (stack_entry_t *)realloc(m_Entries, sizeof(stack_entry_t) * m_MaxEntries);
		}

		etr = &m_Entries[m_NumEntries++];
		etr->ins = NULL;
		etr->pos = pos;

		return etr;
	}
	else
	{
		for (size_t i = m_NumEntries - 1;
			 i >= 0 && i < m_NumEntries && pos >= m_Entries[i].pos;
			 i--)
		{
			if (m_Entries[i].pos == pos)
			{
				return &m_Entries[i];
			}
		}
	}

	return NULL;
}
