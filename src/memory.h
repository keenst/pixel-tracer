typedef struct {
	void*  base;
	void*  head;
	uint32 size;
} Arena;

enum { ARENA_STACK_MAX = 64 };

#define array_count(array) (sizeof(array) / sizeof(array[0]))
