#ifndef _INCLUDE_KNIGHT_KE_SECTOR_STACK_H_
#define _INCLUDE_KNIGHT_KE_SECTOR_STACK_H_

#include <KnightAllocator.h>
#include <KeVector.h>

namespace Knight
{
	template <class T>
	class KeSectorStack
	{
	public:
		static const size_t DEFAULT_SECTOR_SIZE = 64;

		KeSectorStack() : m_SectorSize(DEFAULT_SECTOR_SIZE), m_UsedSize(0), m_MaxUsedSize(0)
		{
			m_pAlloc = NULL;
		}

		KeSectorStack(size_t sectorSize) : m_SectorSize(sectorSize), m_UsedSize(0), m_MaxUsedSize(0)
		{
			m_pAlloc = NULL;
		}

		KeSectorStack(size_t sectorSize, ke_allocator_t *alloc) : 
			m_SectorSize(sectorSize), m_UsedSize(0), m_pAlloc(alloc), m_MaxUsedSize(0)
		{
		}

		~KeSectorStack()
		{
			clear();
		}

		void clear()
		{
			T *sector;
			size_t last_sector;
			size_t last_sector_item;

			if (m_MaxUsedSize == 0)
			{
				return;
			}

			last_sector = (m_MaxUsedSize - 1) / m_SectorSize;
			last_sector_item = (m_MaxUsedSize - 1) % m_SectorSize;

			for (size_t i = 0; i < last_sector; i++)
			{
				sector = m_Sectors[i];

				for (size_t j = 0; j < m_SectorSize; j++)
				{
					sector[j].~T();
				}
			}

			sector = m_Sectors[last_sector];
			for (size_t i = 0; i <= last_sector_item; i++)
			{
				sector[i].~T();
			}

			clear_no_dtors();
		}

		void clear_no_dtors()
		{
			for (size_t i = 0; i < m_Sectors.size(); i++)
			{
				free_sector(m_Sectors[i]);
			}

			m_Sectors.clear();
		}

		bool empty()
		{
			return (m_UsedSize == 0) ? true : false;
		}

		void push(const T & val)
		{
			if ((m_UsedSize / m_SectorSize) >= m_Sectors.size())
			{
				/* Create a new sector */
				T * sector;
				
				if (m_pAlloc == NULL)
				{
					sector = (T *)malloc(sizeof(T) * m_SectorSize);
				}
				else 
				{
					sector = (T *)m_pAlloc->alloc(m_pAlloc, sizeof(T) * m_SectorSize);
				}

				m_Sectors.push_back(sector);
			}

			at(m_UsedSize) = val;

			m_UsedSize++;

			/* Keep track of the maximum used size so we can defer the 
			 * massive destruction job until the end.
			 */
			if (m_UsedSize > m_MaxUsedSize)
			{
				m_MaxUsedSize = m_UsedSize;
			}
		}

		void pop()
		{
			m_UsedSize--;
		}

		void pop_all()
		{
			m_UsedSize = 0;
		}

		T & front()
		{
			return at(m_UsedSize - 1);
		}

		size_t size()
		{
			return m_UsedSize;
		}

	private:
		T & at(size_t x)
		{
			return m_Sectors[x / m_SectorSize][x % m_SectorSize];
		}

		void free_sector(T * sector)
		{
			if (m_pAlloc == NULL)
			{
				free(sector);
			}
			else if (m_pAlloc->dealloc != NULL)
			{
				m_pAlloc->dealloc(m_pAlloc, sector);
			}
		}

	private:
		KeVector<T *> m_Sectors;
		size_t m_SectorSize;
		size_t m_UsedSize;
		ke_allocator_t *m_pAlloc;
		size_t m_MaxUsedSize;
	};
}

#endif //_INCLUDE_KNIGHT_KE_SECTOR_STACK_H_
