#include <malloc.h>
#include <assert.h>
#include "sc.h"
#include "sctracker.h"

heapuse_list_t *heapusage = NULL;

/**
* Creates a new heap allocation tracker entry
*/
void pushheaplist()
{
	heapuse_list_t *newlist=(heapuse_list_t *)malloc(sizeof(heapuse_list_t));
	newlist->prev=heapusage;
	newlist->head=NULL;
	heapusage=newlist;
}

/**
* Generates code to free all heap allocations on a tracker
*/
void freeheapusage(heapuse_list_t *heap)
{
	heapuse_t *cur=heap->head;
	heapuse_t *tmp;
	while (cur) {
		if (cur->type == HEAPUSE_STATIC) {
			modheap((-1)*cur->size*sizeof(cell));
		} else {
			modheap_i();
		}
		tmp=cur->prev;
		free(cur);
		cur=tmp;
	}
	heap->head=NULL;
}

/**
* Pops a heap list but does not free it.
*/
heapuse_list_t *popsaveheaplist()
{
	heapuse_list_t *oldlist=heapusage;
	heapusage=heapusage->prev;
	return oldlist;
}

/**
* Pops a heap list and frees it.
*/
void popheaplist()
{
	heapuse_list_t *oldlist;
	assert(heapusage!=NULL);

	freeheapusage(heapusage);
	assert(heapusage->head==NULL);

	oldlist=heapusage->prev;
	free(heapusage);
	heapusage=oldlist;
}

/*
* Returns the size passed in
*/
int markheap(int type, int size)
{
	heapuse_t *use;
	if (type==HEAPUSE_STATIC && size==0)
		return 0;
	use=heapusage->head;
	if (use && (type==HEAPUSE_STATIC) 
		&& (use->type == type))
	{
		use->size += size;
	} else {
		use=(heapuse_t *)malloc(sizeof(heapuse_t));
		use->type=type;
		use->size=size;
		use->prev=heapusage->head;
		heapusage->head=use;
	}
	return size;
}