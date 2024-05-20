/**************************************************************************/
/*  hash_set.h                                                            */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef HASH_SET_H
#define HASH_SET_H

#include "core/math/math_funcs.h"
#include "core/os/memory.h"
#include "core/templates/hash_map.h"
#include "core/templates/hashfuncs.h"
#include "core/templates/paged_allocator.h"

/**
 * Implementation of Set using a bidi indexed hash map.
 * Use RBSet instead of this only if the following conditions are met:
 *
 * - You need to keep an iterator or const pointer to Key and you intend to add/remove elements in the meantime.
 * - Iteration order does matter (via operator<).
 *
 */

template <typename TKey,
		typename Hasher = HashMapHasherDefault,
		typename Comparator = HashMapComparatorDefault<TKey>>
class HashSet {
public:
	// Must be 2^n.
	static constexpr uint32_t INITIAL_CAPACITY = 32;
	static constexpr uint32_t EMPTY_HASH = 0;

private:
	TKey *keys = nullptr;
	uint32_t *hash_to_key = nullptr;
	uint32_t *key_to_hash = nullptr;
	uint32_t *hashes = nullptr;

	// Due to optimization, this is `capacity - 1`. Use + 1 to get normal capacity.
	uint32_t capacity = 0;
	uint32_t num_elements = 0;

	_FORCE_INLINE_ uint32_t _hash(const TKey &p_key) const {
		uint32_t hash = Hasher::hash(p_key);

		if (unlikely(hash == EMPTY_HASH)) {
			hash = EMPTY_HASH + 1;
		}

		return hash;
	}

	static _FORCE_INLINE_ uint32_t _get_resize_count(uint32_t p_capacity) {
		return p_capacity ^ (p_capacity + 1) >> 2; // = get_capacity() * 0.75 - 1; Works only if p_capacity = 2^n - 1.
	}

	static _FORCE_INLINE_ uint32_t _get_probe_length(uint32_t p_pos, uint32_t p_hash, uint32_t p_local_capacity) {
		const uint32_t original_pos = p_hash & p_local_capacity;
		return (p_pos - original_pos + p_local_capacity + 1) & p_local_capacity;
	}

	_FORCE_INLINE_ bool _lookup_pos(const TKey &p_key, uint32_t &r_pos) const {
		if (unlikely(hashes == nullptr)) {
			return false; // Failed lookups, no elements.
		}
		return _lookup_pos_with_hash(p_key, r_pos, _hash(p_key));
	}

	_FORCE_INLINE_ bool _lookup_pos_with_hash(const TKey &p_key, uint32_t &r_pos, uint32_t p_hash) const {
		if (unlikely(hashes == nullptr)) {
			return false; // Failed lookups, no elements.
		}

		uint32_t pos = p_hash & capacity;

		if (hashes[pos] == p_hash && Comparator::compare(keys[hash_to_key[pos]], p_key)) {
			r_pos = hash_to_key[pos];
			return true;
		}

		if (hashes[pos] == EMPTY_HASH) {
			return false;
		}

		// A collision occurred.
		pos = (pos + 1) & capacity;
		uint32_t distance = 1;
		while (true) {
			if (hashes[pos] == p_hash && Comparator::compare(keys[hash_to_key[pos]], p_key)) {
				r_pos = hash_to_key[pos];
				return true;
			}

			if (hashes[pos] == EMPTY_HASH) {
				return false;
			}

			if (distance > _get_probe_length(pos, hashes[pos], capacity)) {
				return false;
			}

			pos = (pos + 1) & capacity;
			distance++;
		}
	}

	_FORCE_INLINE_ uint32_t _insert_with_hash(uint32_t p_hash, uint32_t p_index) {
		uint32_t hash = p_hash;
		uint32_t index = p_index;
		uint32_t distance = 0;
		uint32_t pos = p_hash & capacity;

		while (true) {
			if (hashes[pos] == EMPTY_HASH) {
				hashes[pos] = hash;
				key_to_hash[index] = pos;
				hash_to_key[pos] = index;
				return pos;
			}

			// Not an empty slot, let's check the probing length of the existing one.
			uint32_t existing_probe_len = _get_probe_length(pos, hashes[pos], capacity);
			if (existing_probe_len < distance) {
				key_to_hash[index] = pos;
				SWAP(hash, hashes[pos]);
				SWAP(index, hash_to_key[pos]);
				distance = existing_probe_len;
			}

			pos = (pos + 1) & capacity;
			distance++;
		}
	}

	void _resize_and_rehash(uint32_t p_new_capacity) {
		// Capacity can't be 0 and must be 2^n - 1.
		capacity = MAX(4u, p_new_capacity);
		uint32_t real_capacity = next_power_of_2(capacity - 1);
		capacity = real_capacity - 1;

		uint32_t *old_hashes = hashes;
		uint32_t *old_key_to_hash = key_to_hash;

		hashes = reinterpret_cast<uint32_t *>(Memory::alloc_static(sizeof(uint32_t) * real_capacity));
		keys = reinterpret_cast<TKey *>(Memory::realloc_static(keys, sizeof(TKey) * (_get_resize_count(capacity) + 1)));
		key_to_hash = reinterpret_cast<uint32_t *>(Memory::alloc_static(sizeof(uint32_t) * (_get_resize_count(capacity) + 1)));
		hash_to_key = reinterpret_cast<uint32_t *>(Memory::realloc_static(hash_to_key, sizeof(uint32_t) * real_capacity));

		memset(hashes, EMPTY_HASH, real_capacity * sizeof(uint32_t));

		for (uint32_t i = 0; i < num_elements; i++) {
			uint32_t h = old_hashes[old_key_to_hash[i]];
			_insert_with_hash(h, i);
		}

		Memory::free_static(old_hashes);
		Memory::free_static(old_key_to_hash);
	}

	_FORCE_INLINE_ int32_t _insert(const TKey &p_key) {
		if (unlikely(hashes == nullptr)) {
			// Allocate on demand to save memory.

			uint32_t real_capacity = capacity + 1;
			hashes = reinterpret_cast<uint32_t *>(Memory::alloc_static(sizeof(uint32_t) * real_capacity));
			keys = reinterpret_cast<TKey *>(Memory::alloc_static(sizeof(TKey) * (_get_resize_count(capacity) + 1)));
			key_to_hash = reinterpret_cast<uint32_t *>(Memory::alloc_static(sizeof(uint32_t) * (_get_resize_count(capacity) + 1)));
			hash_to_key = reinterpret_cast<uint32_t *>(Memory::alloc_static(sizeof(uint32_t) * real_capacity));

			memset(hashes, EMPTY_HASH, real_capacity * sizeof(uint32_t));
		}

		uint32_t pos = 0;
		uint32_t hash = _hash(p_key);
		bool exists = _lookup_pos_with_hash(p_key, pos, hash);

		if (exists) {
			return pos;
		} else {
			if (unlikely(num_elements > _get_resize_count(capacity))) {
				_resize_and_rehash(capacity * 2);
			}

			if constexpr (!std::is_trivially_constructible_v<TKey>) {
				memnew_placement(&keys[num_elements], TKey(p_key));
			} else {
				TKey key = p_key;
				keys[num_elements] = key;
			}

			_insert_with_hash(hash, num_elements);
			num_elements++;
			return num_elements - 1;
		}
	}

	void _init_from(const HashSet &p_other) {
		capacity = p_other.capacity;
		uint32_t real_capacity = capacity + 1;
		num_elements = p_other.num_elements;

		if (p_other.num_elements == 0) {
			return;
		}

		hashes = reinterpret_cast<uint32_t *>(Memory::alloc_static(sizeof(uint32_t) * real_capacity));
		keys = reinterpret_cast<TKey *>(Memory::alloc_static(sizeof(TKey) * (_get_resize_count(capacity) + 1)));
		key_to_hash = reinterpret_cast<uint32_t *>(Memory::alloc_static(sizeof(uint32_t) * (_get_resize_count(capacity) + 1)));
		hash_to_key = reinterpret_cast<uint32_t *>(Memory::alloc_static(sizeof(uint32_t) * real_capacity));

		if constexpr (std::is_trivially_copyable_v<TKey>) {
			memcpy(keys, p_other.keys, sizeof(TKey) * num_elements);
		} else {
			for (uint32_t i = 0; i < num_elements; i++) {
				memnew_placement(&keys[i], TKey(p_other.keys[i]));
			}
		}

		memcpy(key_to_hash, p_other.key_to_hash, sizeof(uint32_t) * num_elements);
		memcpy(hashes, p_other.hashes, sizeof(uint32_t) * real_capacity);
		memcpy(hash_to_key, p_other.hash_to_key, sizeof(uint32_t) * real_capacity);
	}

public:
	_FORCE_INLINE_ uint32_t get_capacity() const { return capacity + 1; }
	_FORCE_INLINE_ uint32_t size() const { return num_elements; }

	/* Standard Godot Container API */

	_FORCE_INLINE_ bool is_empty() const {
		return num_elements == 0;
	}

	void clear() {
		if (hashes == nullptr || num_elements == 0) {
			return;
		}

		memset(hashes, EMPTY_HASH, (capacity + 1) * sizeof(uint32_t));
		if constexpr (!std::is_trivially_destructible_v<TKey>) {
			for (uint32_t i = 0; i < num_elements; i++) {
				keys[i].~TKey();
			}
		}

		num_elements = 0;
	}

	bool has(const TKey &p_key) const {
		uint32_t _pos = 0;
		return _lookup_pos(p_key, _pos);
	}

	bool erase(const TKey &p_key) {
		uint32_t pos = 0;
		bool exists = _lookup_pos(p_key, pos);

		if (!exists) {
			return false;
		}

		uint32_t key_pos = pos;
		pos = key_to_hash[pos]; // Make hash pos.

		uint32_t next_pos = (pos + 1) & capacity;
		while (hashes[next_pos] != EMPTY_HASH && _get_probe_length(next_pos, hashes[next_pos], capacity) != 0) {
			uint32_t kpos = hash_to_key[pos];
			uint32_t kpos_next = hash_to_key[next_pos];
			SWAP(key_to_hash[kpos], key_to_hash[kpos_next]);
			SWAP(hashes[next_pos], hashes[pos]);
			SWAP(hash_to_key[next_pos], hash_to_key[pos]);

			pos = next_pos;
			next_pos = (next_pos + 1) & capacity;
		}

		hashes[pos] = EMPTY_HASH;
		if constexpr (!std::is_trivially_destructible_v<TKey>) {
			keys[key_pos].~TKey();
		}
		num_elements--;
		if (key_pos < num_elements) {
			// Not the last key, move the last one here to keep keys lineal.
			if constexpr (!std::is_trivially_constructible_v<TKey>) {
				memnew_placement(&keys[key_pos], TKey(keys[num_elements]));
			} else {
				TKey key = keys[num_elements];
				keys[key_pos] = key;
			}
			if constexpr (!std::is_trivially_destructible_v<TKey>) {
				keys[num_elements].~TKey();
			}
			key_to_hash[key_pos] = key_to_hash[num_elements];
			hash_to_key[key_to_hash[num_elements]] = key_pos;
		}

		return true;
	}

	// Reserves space for a number of elements, useful to avoid many resizes and rehashes.
	// If adding a known (possibly large) number of elements at once, must be larger than old capacity.
	void reserve(uint32_t p_new_capacity) {
		ERR_FAIL_COND_MSG(p_new_capacity < get_capacity(), "It is impossible to reserve less capacity than is currently available.");
		if (hashes == nullptr) {
			capacity = MAX(4u, p_new_capacity);
			capacity = next_power_of_2(capacity - 1) - 1;
			return; // Unallocated yet.
		}
		_resize_and_rehash(p_new_capacity);
	}

	/** Iterator API **/

	struct Iterator {
		_FORCE_INLINE_ const TKey &operator*() const {
			return *key;
		}
		_FORCE_INLINE_ const TKey *operator->() const {
			return key;
		}
		_FORCE_INLINE_ Iterator &operator++() {
			key++;
			return *this;
		}
		_FORCE_INLINE_ Iterator &operator--() {
			key--;
			return *this;
		}

		_FORCE_INLINE_ bool operator==(const Iterator &b) const { return key == b.key; }
		_FORCE_INLINE_ bool operator!=(const Iterator &b) const { return key != b.key; }

		_FORCE_INLINE_ explicit operator bool() const {
			return key != end;
		}

		_FORCE_INLINE_ Iterator(TKey *p_key, TKey *p_end) {
			key = p_key;
			end = p_end;
		}
		_FORCE_INLINE_ Iterator() {}
		_FORCE_INLINE_ Iterator(const Iterator &p_it) {
			key = p_it.key;
			end = p_it.end;
		}
		_FORCE_INLINE_ void operator=(const Iterator &p_it) {
			key = p_it.key;
			end = p_it.end;
		}

	private:
		TKey *key = nullptr;
		TKey *end = nullptr;
	};

	_FORCE_INLINE_ Iterator begin() const {
		return Iterator(keys, keys + num_elements);
	}
	_FORCE_INLINE_ Iterator end() const {
		return Iterator(keys + num_elements, keys + num_elements);
	}
	_FORCE_INLINE_ Iterator last() const {
		return Iterator(keys + num_elements - 1, keys + num_elements);
	}

	Iterator find(const TKey &p_key) const {
		uint32_t pos = 0;
		bool exists = _lookup_pos(p_key, pos);
		if (!exists) {
			return end();
		}
		return Iterator(keys + pos, keys + num_elements);
	}

	_FORCE_INLINE_ void remove(const Iterator &p_iter) {
		if (p_iter) {
			erase(*p_iter);
		}
	}

	/* Insert */

	Iterator insert(const TKey &p_key) {
		uint32_t pos = _insert(p_key);
		return Iterator(keys + pos, keys + num_elements);
	}

	/* Constructors */

	HashSet(const HashSet &p_other) {
		_init_from(p_other);
	}

	void operator=(const HashSet &p_other) {
		if (this == &p_other) {
			return; // Ignore self assignment.
		}

		reset();

		_init_from(p_other);
	}

	HashSet(uint32_t p_initial_capacity) {
		// Capacity can't be 0 and must be 2^n - 1.
		capacity = MAX(4u, p_initial_capacity);
		capacity = next_power_of_2(capacity - 1) - 1;
	}
	HashSet() :
			capacity(INITIAL_CAPACITY - 1) {
	}

	void reset() {
		if (hashes != nullptr) {
			if constexpr (!std::is_trivially_destructible_v<TKey>) {
				for (uint32_t i = 0; i < num_elements; i++) {
					keys[i].~TKey();
				}
			}
			Memory::free_static(keys);
			Memory::free_static(key_to_hash);
			Memory::free_static(hash_to_key);
			Memory::free_static(hashes);
			keys = nullptr;
			hashes = nullptr;
			hash_to_key = nullptr;
			key_to_hash = nullptr;
		}
		capacity = INITIAL_CAPACITY - 1;
	}

	~HashSet() {
		reset();
	}
};

#endif // HASH_SET_H
