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

#include <malloc.h>
#include <string.h>

class CellArray
{
public:
	CellArray(size_t blocksize) : m_Data(NULL), m_BlockSize(blocksize), m_AllocSize(0), m_Size(0)
	{
	}
	~CellArray()
	{
		free(m_Data);
	}
	size_t size() const
	{
		return m_Size;
	}
	cell_t *push()
	{
		if (!GrowIfNeeded(1))
		{
			return NULL;
		}
		cell_t *arr = &m_Data[m_Size * m_BlockSize];
		m_Size++;
		return arr;
	}
	cell_t *at(size_t b) const
	{
		return &m_Data[b * m_BlockSize];
	}
	size_t blocksize() const
	{
		return m_BlockSize;
	}
	void clear()
	{
		m_Size = 0;
	}
	bool swap(size_t item1, size_t item2)
	{
		/* Make sure there is extra space available */
		if (!GrowIfNeeded(1))
		{
			return false;
		}

		cell_t *pri = at(item1);
		cell_t *alt = at(item2);

		/* Get our temporary array 1 after the limit */
		cell_t *temp = &m_Data[m_Size * m_BlockSize];

		memcpy(temp, pri, sizeof(cell_t) * m_BlockSize);
		memcpy(pri, alt, sizeof(cell_t) * m_BlockSize);
		memcpy(alt, temp, sizeof(cell_t) * m_BlockSize);

		return true;
	}
	void remove(size_t index)
	{
		/* If we're at the end, take the easy way out */
		if (index == m_Size - 1)
		{
			m_Size--;
			return;
		}

		/* Otherwise, it's time to move stuff! */
		size_t remaining_indexes = (m_Size - 1) - index;
		cell_t *src = at(index + 1);
		cell_t *dest = at(index);
		memmove(dest, src, sizeof(cell_t) * m_BlockSize * remaining_indexes);

		m_Size--;
	}
	cell_t *insert_at(size_t index)
	{
		/* Make sure it'll fit */
		if (!GrowIfNeeded(1))
		{
			return NULL;
		}

		/* move everything up */
		cell_t *src = at(index);
		cell_t *dst = at(index + 1);
		memmove(dst, src, sizeof(cell_t) * m_BlockSize * m_Size);

		m_Size++;

		return src;
	}
	bool resize(size_t count)
	{
		if (count <= m_Size)
		{
			m_Size = count;
			return true;
		}

		if(!GrowIfNeeded(count - m_Size))
		{
			return false;
		}

		m_Size = count;
		return true;
	}
private:
	bool GrowIfNeeded(size_t count)
	{
		/* Shortcut out if we can store this */
		if (m_Size + count <= m_AllocSize)
		{
			return true;
		}
		/* Set a base allocation size of 8 items */
		if (!m_AllocSize)
		{
			m_AllocSize = 8;
		}
		/* If it's not enough, keep doubling */
		while (m_Size + count > m_AllocSize)
		{
			m_AllocSize *= 2;
		}
		/* finally, allocate the new block */
		if (m_Data)
		{
			m_Data = (cell_t *)realloc(m_Data, sizeof(cell_t) * m_BlockSize * m_AllocSize);
		} else {
			m_Data = (cell_t *)malloc(sizeof(cell_t) * m_BlockSize * m_AllocSize);
		}
		return (m_Data != NULL);
	}
private:
	cell_t *m_Data;
	size_t m_BlockSize;
	size_t m_AllocSize;
	size_t m_Size;
};
