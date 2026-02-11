#pragma once

#include "types.hpp"

namespace memory {

void* malloc(usize size);
void* realloc(void* block, usize size);
void free(void* block);
void copy(void* dst, const void* src, usize size);
void move(void* dst, const void* src, usize size);
void set(void* dst, byte value, usize size);
void* malloc_aligned(usize size, usize alignment);
void free_aligned(void* ptr);

struct Arena {

	byte* buffer;
	usize size;
	usize offset;

};

Arena arena_create(usize size);
void arena_destroy(Arena* arena);
void* arena_alloc(Arena* arena, usize bytes, usize alignment = 8);
void arena_reset(Arena* arena);

}