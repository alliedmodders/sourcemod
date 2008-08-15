#ifndef _INCLUDE_KNIGHT_KE_VECTOR_H_
#define _INCLUDE_KNIGHT_KE_VECTOR_H_

#include <new>

namespace Knight
{
	template <class T>
	class KeVector
	{
	public:
		KeVector<T>() : m_Data(NULL), m_Size(0), m_CurrentUsedSize(0)
		{
		}

		KeVector<T>(const KeVector<T> & other)
		{
			m_Size = other.m_CurrentUsedSize;
			m_CurrentUsedSize = other.m_CurrentUsedSize;

			if (m_Size > 0)
			{
				m_Data = new T[other.m_CurrentUsedSize];

				for (size_t i = 0; i < m_Size; i++)
				{
					m_Data[i] = other.m_Data[i];
				}
			}
			else
			{
				m_Data = NULL;
			}
		}

		~KeVector<T>()
		{
			clear();
		}

		KeVector & operator =(const KeVector<T> & other)
		{
			clear();

			if (other.m_CurrentUsedSize)
			{
				m_Data = new T[other.m_CurrentUsedSize];
				m_Size = other.m_CurrentUsedSize;
				m_CurrentUsedSize = other.m_CurrentUsedSize;

				for (size_t i = 0; i < m_Size; i++)
				{
					m_Data[i] = other.m_Data[i];
				}
			}
		}

		size_t size() const
		{
			return m_CurrentUsedSize;
		}

		void push_back(const T & elem)
		{
			GrowIfNeeded(1);

			new (&m_Data[m_CurrentUsedSize]) T(elem);

			m_CurrentUsedSize++;
		}

		void pop_back(const T & elem)
		{
			if (m_CurrentUsedSize == 0)
			{
				return;
			}

			m_CurrentUsedSize--;
			m_Data[m_CurrentUsedSize].~T();
		}

		bool is_empty()
		{
			return (m_CurrentUsedSize == 0);
		}

		T & operator [](size_t pos)
		{
			return m_Data[pos];
		}

		const T & operator [](size_t pos) const
		{
			return m_Data[pos];
		}

		void clear()
		{
			for (size_t i = 0; i < m_CurrentUsedSize; i++)
			{
				m_Data[i].~T();
			}

			free(m_Data);
			
			m_Data = NULL;
			m_Size = 0;
			m_CurrentUsedSize = 0;
		}
	private:
		void Grow(size_t amount)
		{
			T *new_data;
			size_t new_size;

			if (m_Size == 0)
			{
				new_size = 8;
			}
			else
			{
				new_size = m_Size * 2;
			}

			while (m_CurrentUsedSize + amount > new_size)
			{
				new_size *= 2;
			}

			new_data = (T *)malloc(sizeof(T) * new_size);

			for (size_t i = 0; i < m_CurrentUsedSize; i++)
			{
				new (&new_data[i]) T(m_Data[i]);
				m_Data[i].~T();
			}

			free(m_Data);
			m_Data = new_data;
			m_Size = new_size;
		}

		void GrowIfNeeded(size_t amount)
		{
			if (m_CurrentUsedSize + amount >= m_Size)
			{
				Grow(amount);
			}
		}
	private:
		T *m_Data;
		size_t m_Size;
		size_t m_CurrentUsedSize;
	};
}

#endif //_INCLUDE_KNIGHT_KE_VECTOR_H_
