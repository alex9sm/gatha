#pragma once

#include "../core/types.hpp"
#include "../core/memory.hpp"
#include "../core/array.hpp"

namespace ecs {

	using Entity = u32;
	constexpr Entity INVALID_ENTITY = ~0u;
	constexpr u32 MAX_ENTITIES = 65536;

	struct EntityPool {
		bool* alive;
		arr::Array<u32> free_list;
		u32 count;
		u32 next_id;
	};

	inline void pool_init(EntityPool* pool) {
		pool->alive = (bool*)memory::malloc(MAX_ENTITIES * sizeof(bool));
		memory::set(pool->alive, 0, MAX_ENTITIES * sizeof(bool));
		pool->free_list = {};
		pool->count = 0;
		pool->next_id = 0;
	}

	inline void pool_destroy(EntityPool* pool) {
		memory::free(pool->alive);
		arr::array_destroy(&pool->free_list);
		*pool = {};
	}

	inline Entity pool_create(EntityPool* pool) {
		u32 id;
		if (pool->free_list.count > 0) {
			id = arr::array_pop(&pool->free_list);
		} else {
			id = pool->next_id++;
			if (id >= MAX_ENTITIES) return INVALID_ENTITY;
		}
		pool->alive[id] = true;
		pool->count++;
		return id;
	}

	inline void pool_release(EntityPool* pool, Entity e) {
		if (e >= MAX_ENTITIES || !pool->alive[e]) return;
		pool->alive[e] = false;
		arr::array_push(&pool->free_list, e);
		pool->count--;
	}

	inline bool pool_alive(const EntityPool* pool, Entity e) {
		return e < MAX_ENTITIES && pool->alive[e];
	}

	template<typename T>
	struct Store {
		arr::Array<T>      data;
		arr::Array<Entity> entities;
		u32*               sparse;
	};

	template<typename T>
	void store_init(Store<T>* store) {
		store->data = {};
		store->entities = {};
		store->sparse = (u32*)memory::malloc(MAX_ENTITIES * sizeof(u32));
		memory::set(store->sparse, 0xFF, MAX_ENTITIES * sizeof(u32)); // fill with INVALID_ENTITY
	}

	template<typename T>
	void store_destroy(Store<T>* store) {
		arr::array_destroy(&store->data);
		arr::array_destroy(&store->entities);
		memory::free(store->sparse);
		store->sparse = nullptr;
	}

	template<typename T>
	bool store_has(const Store<T>* store, Entity e) {
		return e < MAX_ENTITIES && store->sparse[e] != INVALID_ENTITY;
	}

	template<typename T>
	T* store_get(Store<T>* store, Entity e) {
		if (!store_has(store, e)) return nullptr;
		return &store->data.data[store->sparse[e]];
	}

	template<typename T>
	T* store_add(Store<T>* store, Entity e, const T& component) {
		if (store_has(store, e)) return store_get(store, e);
		u32 dense_index = (u32)store->data.count;
		arr::array_push(&store->data, component);
		arr::array_push(&store->entities, e);
		store->sparse[e] = dense_index;
		return &store->data.data[dense_index];
	}

	template<typename T>
	void store_remove(Store<T>* store, Entity e) {
		if (!store_has(store, e)) return;
		u32 dense_index = store->sparse[e];
		u32 last_index = (u32)store->data.count - 1;

		if (dense_index != last_index) {
			Entity last_entity = store->entities.data[last_index];
			store->data.data[dense_index] = store->data.data[last_index];
			store->entities.data[dense_index] = last_entity;
			store->sparse[last_entity] = dense_index;
		}

		store->data.count--;
		store->entities.count--;
		store->sparse[e] = INVALID_ENTITY;
	}

}
