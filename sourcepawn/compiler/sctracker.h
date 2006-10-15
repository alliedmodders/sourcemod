#ifndef _INCLUDE_SOURCEPAWN_COMPILER_TRACKER_H_
#define _INCLUDE_SOURCEPAWN_COMPILER_TRACKER_H_

#define HEAPUSE_STATIC	0
#define HEAPUSE_DYNAMIC	1

typedef struct heapuse_s {
	int type;   /* HEAPUSE_STATIC or HEAPUSE_DYNAMIC */
	int size;   /* size of array for static (0 for dynamic) */
	struct heapuse_s *prev; /* previous array on the list */
} heapuse_t;

typedef struct heapuse_list_s {
	struct heapuse_list_s *prev;   /* last used list */
	heapuse_t *head;               /* head of the current list */
} heapuse_list_t;

extern heapuse_list_t *heapusage;

void pushheaplist();
void freeheapusage(heapuse_list_t *heap);
heapuse_list_t *popsaveheaplist();
void popheaplist();
int markheap(int type, int size);

#endif //_INCLUDE_SOURCEPAWN_COMPILER_TRACKER_H_
