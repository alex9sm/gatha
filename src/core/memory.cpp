#include "memory.hpp"

extern "C" void* malloc(size_t);
extern "C" void* realloc(void*, size_t);
extern "C" void free(void*);
extern "C" void* memcpy(void*, const void*, size_t);
extern "C" void* memmove(void*, const void*, size_t);
extern "C" void* memset(void*, int, size_t);

extern "C" void* _aligned_malloc(size_t size, size_t alignment);
extern "C" void _aligned_free(void* ptr);

void* memory::malloc(usize size) {

	return ::malloc(size);

}

void* memory::realloc(void* block, usize size) {

	return ::realloc(block, size);

}

void memory::free(void* block) {

	::free(block);

}

void memory::copy(void* dst, const void* src, usize size) {

	::memcpy(dst, src, size);

}

void memory::move(void* dst, const void* src, usize size) {

	::memmove(dst, src, size);

}

void memory::set(void* dst, byte value, usize size) {

	::memset(dst, value, size);

}

void* memory::malloc_aligned(usize size, usize alignment) {

	return ::_aligned_malloc(size, alignment);

}

void memory::free_aligned(void* ptr) {

	::_aligned_free(ptr);

}

memory::Arena memory::arena_create(usize size) {

	Arena arena;
	arena.buffer = (byte*)memory::malloc(size);
	arena.size = size;
	arena.offset = 0;
	return arena;

}

void memory::arena_destroy(Arena* arena) {

	memory::free(arena->buffer);
	arena->buffer = nullptr;
	arena->size = 0;
	arena->offset = 0;

}

void* memory::arena_alloc(Arena* arena, usize bytes, usize alignment) {

	usize padding = 0;
	usize modulo = arena->offset % alignment;
	if (modulo != 0) {
		padding = alignment - modulo;
	}

	usize padded_offset = arena->offset + padding;

	if (padded_offset + bytes > arena->size) {
		return nullptr;
	}

	void* ptr = arena->buffer + padded_offset;
	arena->offset = padded_offset + bytes;

	return ptr;

}

void memory::arena_reset(Arena* arena) {

	arena->offset = 0;

}