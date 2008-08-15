#ifndef _INCLUDE_KNIGHT_ALLOCATOR_H_
#define _INCLUDE_KNIGHT_ALLOCATOR_H_

#include <stddef.h>
#include <stdlib.h>

struct ke_allocator_s;
typedef struct ke_allocator_s ke_allocator_t;

typedef void *(*KEFN_ALLOCATOR)(ke_allocator_t *, size_t);
typedef void (*KEFN_DEALLOCATOR)(ke_allocator_t *, void *);

struct ke_allocator_s
{
	KEFN_ALLOCATOR alloc;
	KEFN_DEALLOCATOR dealloc;
	void *user;
};

inline void *operator new(size_t size, ke_allocator_t *alloc)
{
	return alloc->alloc(alloc, size);
}

inline void *operator new [](size_t size, ke_allocator_t *alloc)
{
	return alloc->alloc(alloc, size);
}

template <typename T>
void ke_destroy(ke_allocator_t *alloc, T * data)
{
	data->~T();
	alloc->dealloc(alloc, data);
}

#endif //_INCLUDE_KNIGHT_ALLOCATOR_H_
