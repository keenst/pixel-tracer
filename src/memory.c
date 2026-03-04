int kb(int bytes) {
	return 1024 * bytes;
}

int mb(int bytes) {
	return 1024 * kb(bytes);
}

int gb(int bytes) {
	return 1024 * mb(bytes);
}

void push_arena(Arena* arena) {
	ASSERT(global->arena_stack_size < ARENA_STACK_MAX);
	global->arena_stack[global->arena_stack_size++] = arena;
}

void pop_arena() {
	ASSERT(global->arena_stack_size > 0);
	global->arena_stack_size--;
}

void* alloc(uint size) {
	Arena* arena = global->arena_stack[global->arena_stack_size - 1];
	ASSERT((char*)arena->head + size <= (char*)arena->base + arena->size);
	void* head = arena->head;
	arena->head = (char*)arena->head + size;
	return head;
}

Arena branch_arena(uint size) {
	void* base = alloc(size);
	memset(base, 0, size);
	Arena new_arena = {
		.base = base,
		.head = base,
		.size = size
	};
	return new_arena;
}

// Creates an arena on heap
Arena spawn_arena(uint size) {
	void* base = malloc(size);
	memset(base, 0, size);
	Arena new_arena = {
		.base = base,
		.head = base,
		.size = size
	};
	return new_arena;
}

// Free arena allocated on heap
void free_arena(Arena* arena) {
	// Clear memory to make it more noticeable if something goes wrong
	memset(arena->base, 0, arena->size);
	free(arena->base);
}

void clear_arena(Arena* arena) {
	arena->head = arena->base;
	memset(arena->base, 0, arena->size);
}
