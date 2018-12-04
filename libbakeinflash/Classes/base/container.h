// container.h	-- Thatcher Ulrich <tu@tulrich.com> 31 July 2001

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Generic C++ containers.  Problem: STL is murder on compile times,
// and is hard to debug.  These are substitutes that compile much
// faster and are somewhat easier to debug.  Not as featureful,
// efficient or hammered-on as STL though.  You can use STL
// implementations if you want; see _TU_USE_STL.

#ifndef CONTAINER_H
#define CONTAINER_H

#include "base/tu_config.h"
#include "base/utility.h"
#include <stdlib.h>
#include <string.h>	// for strcmp and friends
#include <new>	// for placement new


// If you prefer STL implementations of array<> (i.e. std::vector) and
// hash<> (i.e. std::hash_map) instead of home cooking, then put
// -D_TU_USE_STL=1 in your compiler flags, or do it in tu_config.h, or do
// it right here:

//#define _TU_USE_STL 1

#ifdef WIN32
#define __gnu_cxx stdext
#endif

template<class T>
class fixed_size_hash
// Computes a hash of an object's representation.
{
public:

#if _TU_USE_STL == 1
	enum
	{ // parameters for hash table
		bucket_size = 4, // 0 < bucket_size
		min_buckets = 8
	}; // min_buckets = 2 ^^ N, 0 < N 
#endif

	Uint32	operator()(const T& data) const
	{
		unsigned char*	p = (unsigned char*) &data;
		Sint32	size = sizeof(T);

		return sdbm_hash(p, size);
	}

#if _TU_USE_STL == 1
	// test if s1 ordered before s2
	bool operator()(const T& s1, const T& s2) const
  {
		return s1 < s2;
  } 
#endif

};


template<class T>
class identity_hash
// Hash is just the input value; can use this for integer-indexed hash tables.
{
public:
	Uint32	operator()(const T& data) const
	{
		return (Uint32) data;
	}
};


#if _TU_USE_STL == 1


//
// Thin wrappers around STL
//


//// @@@ crap compatibility crap
//#define StlAlloc(size) malloc(size)
//#define StlFree(ptr, size) free(ptr)


#include <vector>
#include <hash_map>
#include <string>


// array<> is much like std::vector<>
//
// @@ move this towards a strict subset of std::vector ?  Compatibility is good.
template<class T> class array : public std::vector<T>
{
public:
	Sint32	size() const { return (Sint32) std::vector<T>::size(); }

	void	append(const array<T>& other)
	// Append the given data to our array.
	{
		std::vector<T>::insert(end(), other.begin(), other.end());
	}

	void	append(const T other[], Sint32 count)
	{
		// This will probably work.  Depends on std::vector<T>::iterator being typedef'd as T*
		std::vector<T>::insert(end(), &other[0], &other[count]);
	}

	void	remove(Sint32 index)
	{
		std::vector<T>::erase(begin() + index);
	}

	void	insert(Sint32 index, const T& val = T())
	{
		std::vector<T>::insert(begin() + index, val);
	}

	void	release()
	{
		// Drop all storage.
		std::vector<T>	temp;
		this->swap(temp);
	}
};


// hash<> is similar to std::hash_map<>
//
// @@ move this towards a strict subset of std::hash_map<> ?
template<class T, class U, class hash_functor = fixed_size_hash<T> >
class hash : public __gnu_cxx::hash_map<T, U, hash_functor>
{
public:

	// extra convenience interfaces
	void	set(const T& key, const U& value)
	// Set a new or existing value under the key, to the value.
	{
		(*this)[key] = value;
	}

	void	add(const T& key, const U& value)
	{
//		assert(find(key) == end());
		(*this)[key] = value;
	}

	bool	is_empty() const { return empty(); }

	bool	get(const T& key, U* value) const
	// Retrieve the value under the given key.
	//
	// If there's no value under the key, then return false and leave
	// *value alone.
	//
	// If there is a value, return true, and set *value to the entry's
	// value.
	//
	// If value == NULL, return true or false according to the
	// presence of the key, but don't touch *value.
	{
		const_iterator	it = find(key);
		if (it != end())
		{
			if (value) *value = it->second;
			return true;
		}
		else
		{
			return false;
		}
	}

	void set_capacity(Sint32 n) {}
};

// template<class U>
// class string_hash : public hash<tu_string, U, std::hash<std::string> >
// {
// };


#else // not _TU_USE_STL


//
// Homemade containers; almost strict subsets of STL.
//


#ifdef _WIN32
#pragma warning(disable : 4345)	// in MSVC 7.1, warning about placement new POD default initializer
#endif // _WIN32




template<class T>
class array {
// Resizable array.  Don't put anything in here that can't be moved
// around by bitwise copy.  Don't keep the address of an element; the
// array contents will move around as it gets resized.
//
// Default constructor and destructor get called on the elements as
// they are added or removed from the active part of the array.
public:
	typedef T value_type;

	array() : m_buffer(0), m_size(0), m_buffer_size(0) {}
	array(Sint32 size_hint) : m_buffer(0), m_size(0), m_buffer_size(0) { resize(size_hint); }
	array(const array<T>& a)
		:
		m_buffer(0),
		m_size(0),
		m_buffer_size(0)
	{
		operator=(a);
	}
	~array()
	{
		clear();
	}

	// Basic array access.
	T&	operator[](Sint32 index) { assert(index >= 0 && index < m_size); return m_buffer[index]; }
	const T&	operator[](Sint32 index) const { assert(index >= 0 && index < m_size); return m_buffer[index]; }
	Sint32	size() const { return m_size; }

	void	push_back(const T& val)
	// Insert the given element at the end of the array.
	{
		// DO NOT pass elements of your own vector into
		// push_back()!  Since we're using references,
		// resize() may munge the element storage!
		// this is irrelevant to MAC OS !!!
//		assert(&val < &m_buffer[0] || &val > &m_buffer[m_buffer_size]);

		Sint32	new_size = m_size + 1;
		resize(new_size);
		(*this)[new_size-1] = val;
	}

	void	pop_back()
	// Remove the last element.
	{
		assert(m_size > 0);
		resize(m_size - 1);
	}

	// Access the first element.
	T&	front() { return (*this)[0]; }
	const T&	front() const { return (*this)[0]; }

	// Access the last element.
	T&	back() { return (*this)[m_size-1]; }
	const T&	back() const { return (*this)[m_size-1]; }

	void	clear()
	// Empty and destruct all elements.
	{
		resize(0);
	}

	void	operator=(const array<T>& a)
	// Array copy.  Copies the contents of a into this array.
	{
		resize(a.size());
		for (Sint32 i = 0; i < m_size; i++) {
			*(m_buffer + i) = a[i];
		}
	}


	void	remove(Sint32 index)
	// Removing an element from the array is an expensive operation!
	// It compacts only after removing the last element.
	{
		assert(index >= 0 && index < m_size);

		if (m_size == 1)
		{
			clear();
		}
		else
		{
			m_buffer[index].~T();	// destructor

			memmove(m_buffer+index, m_buffer+index+1, sizeof(T) * (m_size - 1 - index));
			m_size--;
		}
	}


	void	insert(Sint32 index, const T& val = T())
	// Insert the given object at the given index shifting all the elements up.
	{
		assert(index >= 0 && index <= m_size);

		resize(m_size + 1);

		if (index < m_size - 1)
		{
			// is it safe to use memmove?
			memmove(m_buffer+index+1, m_buffer+index, sizeof(T) * (m_size - 1 - index));
		}

		// Copy-construct into the newly opened slot.
		new (m_buffer + index) T(val);
	}


	void	append(const array<T>& other)
	// Append the given data to our array.
	{
		append(other.m_buffer, other.size());
	}


	void	append(const T other[], Sint32 count)
	// Append the given data to our array.
	{
		if (count > 0)
		{
			Sint32	size0 = m_size;
			resize(m_size + count);
			// Must use operator=() to copy elements, in case of side effects (e.g. ref-counting).
			for (Sint32 i = 0; i < count; i++)
			{
				m_buffer[i + size0] = other[i];
			}
		}
	}

	void	resize(Sint32 new_size)
	// Preserve existing elements via realloc.
	// 
	// Newly created elements are initialized with default element
	// of T.  Removed elements are destructed.
	{
		assert(new_size >= 0);

		Sint32	old_size = m_size;

		// Destruct old elements (if we're shrinking).
		{for (Sint32 i = new_size; i < old_size; i++) {
			(m_buffer + i)->~T();
		}}

		if (new_size == 0) {
			m_buffer_size = 0;
			reserve(0);
		} else if (new_size <= m_buffer_size && new_size > m_buffer_size >> 1) {
			// don't compact yet.
			assert(m_buffer != 0);
		} else {
			Sint32	new_buffer_size = new_size + (new_size >> 1);
			reserve(new_buffer_size);
		}

		// Copy default T into new elements.
		{for (Sint32 i = old_size; i < new_size; i++) {
			new (m_buffer + i) T();	// placement new
		}}

		m_size = new_size;
	}

	void	reserve(Sint32 rsize)
	// @@ TODO change this to use ctor, dtor, and operator=
	// instead of preserving existing elements via binary copy via
	// realloc?
	{
		assert(m_size >= 0);

		//Sint32	old_size = m_buffer_size;
		//old_size = old_size;	// don't warn that this is unused.
		m_buffer_size = rsize;

		// Resize the buffer.
		if (m_buffer_size == 0) {
			if (m_buffer) {
				free(m_buffer);
			}
			m_buffer = 0;
		} else {
			if (m_buffer) {
				m_buffer = (T*) realloc(m_buffer, sizeof(T) * m_buffer_size);
			} else {

				m_buffer = (T*) malloc(sizeof(T) * m_buffer_size);
			}
			assert(m_buffer);	// need to throw (or something) on malloc failure!
		}			
	}

	void	transfer_members(array<T>* a)
	// UNSAFE!  Low-level utility function: replace this array's
	// members with a's members.
	{
		m_buffer = a->m_buffer;
		m_size = a->m_size;
		m_buffer_size = a->m_buffer_size;

		a->m_buffer = 0;
		a->m_size = 0;
		a->m_buffer_size = 0;
	}

	// Iterator API, for STL compatibility.
	typedef T* iterator;
	typedef const T* const_iterator;

	iterator begin() {
		return m_buffer;
	}
	iterator end() {
		return m_buffer + m_size;
	}
	const_iterator begin() const {
		return m_buffer;
	}
	const_iterator end() const {
		return m_buffer + m_size;
	}

private:
	T*	m_buffer;
	Sint32	m_size;
	Sint32	m_buffer_size;
};


template<class T, class U, class hash_functor = fixed_size_hash<T> >
class hash {
// Hash table, linear probing, internal chaining.  One
// interesting/nice thing about this implementation is that the table
// itself is a flat chunk of memory containing no pointers, only
// relative indices.  If the key and value types of the hash contain
// no pointers, then the hash can be serialized using raw IO.  Could
// come in handy.
//
// Never shrinks, unless you explicitly clear() it.  Expands on
// demand, though.  For best results, if you know roughly how big your
// table will be, default it to that size when you create it.
public:
	hash() :
		m_table(NULL),
		m_has_collisions(false)
	{
	}
	hash(Sint32 size_hint) :
		m_table(NULL),
		m_has_collisions(false)
	{
		set_capacity(size_hint); 
	}
	hash(const hash<T,U,hash_functor>& src)	:
		m_table(NULL),
		m_has_collisions(false)
	{
		*this = src;
	}
	~hash() { clear(); }

	static const Uint32 TOMBSTONE_HASH = (Uint32) -1;

	void	operator=(const hash<T,U,hash_functor>& src)
	{
		clear();
		if (src.is_empty() == false)
		{
			set_capacity(src.size());

			for (const_iterator it = src.begin(); it != src.end(); ++it)
			{
				add(it->first, it->second);
			}
		}
	}

	// For convenience
	U&	operator[](const T& key)
	{
		Sint32	index = find_index(key);
		if (index >= 0)
		{
			return E(index).second;
		}
		add(key, (U) 0);
		index = find_index(key);
		if (index >= 0)
		{
			return E(index).second;
		}

		// Doesn't look nice but removes
		// warning on non-void function not returning.
		assert(0);
		return E(index).second;
	}

	void	set(const T& key, const U& value)
	// Set a new or existing value under the key, to the value.
	{
		Sint32	index = find_index(key);
		if (index >= 0)
		{
			E(index).second = value;
			return;
		}

		// Entry under key doesn't exist.
		add(key, value);
	}

	void	add(const T& key, const U& value)
	// Add a new value to the hash table, under the specified key.
	// Can invalidate existing iterators.
	{
		assert(find_index(key) == -1);

		check_expand();
		assert(m_table);
		m_table->m_entry_count++;

		Uint32	hash_value = (Uint32) compute_hash(key);
		Sint32	index = hash_value & m_table->m_size_mask;

		entry*	natural_entry = &(E(index));
		
		if (natural_entry->is_empty()) 
		{
			// Put the new entry in.
			new (natural_entry) entry(key, value, -1, hash_value);
		} 
		else
		if (natural_entry->is_tombstone())
		{
			// Put the new entry in, without disturbing the rest of the chain.
			Sint32 next_in_chain = natural_entry->m_next_in_chain;
			new (natural_entry) entry(key, value, next_in_chain, hash_value);
		}
		else
		{
			if (natural_entry->m_hash_value == hash_value)
			{
				m_has_collisions = true;
			}

			// Find a blank spot.
			Sint32	blank_index = index;
			for (Sint32 search_count = 0; ; search_count++)
			{
				blank_index = (blank_index + 1) & m_table->m_size_mask;
				if (E(blank_index).is_empty()) break;	// found it
				if (E(blank_index).is_tombstone())
				{
					blank_index = remove_tombstone(blank_index);
					break;
				}
				assert(search_count < m_table->m_size_mask);
			}
			entry*	blank_entry = &E(blank_index);

			if (Sint32(natural_entry->m_hash_value & m_table->m_size_mask) == index)
			{
				// Collision.  Link into this chain.

				// Move existing list head.
				// @@ this could invalidate an existing iterator
				new (blank_entry) entry(*natural_entry);	// placement new, copy ctor

				// Put the new info in the natural entry.
				natural_entry->first = key;
				natural_entry->second = value;
				natural_entry->m_next_in_chain = blank_index;
				natural_entry->m_hash_value = hash_value;
			}
			else
			{
				// Existing entry does not naturally
				// belong in this slot.  Existing
				// entry must be moved.

				// Find natural location of collided element (i.e. root of chain)
				Sint32 collided_index = (Sint32) (natural_entry->m_hash_value & m_table->m_size_mask);
				for (Sint32 search_count = 0; ; search_count++)
				{
					entry*	e = &E(collided_index);
					if (e->m_next_in_chain == index)
					{
						// Here's where we need to splice.
						// @@ this could invalidate an existing iterator
						new (blank_entry) entry(*natural_entry);
						e->m_next_in_chain = blank_index;
						break;
					}
					collided_index = e->m_next_in_chain;
					assert(collided_index >= 0 && collided_index <= m_table->m_size_mask);
					assert(search_count <= m_table->m_size_mask);
				}

				// Put the new data in the natural entry.
				natural_entry->first = key;
				natural_entry->second = value;
				natural_entry->m_hash_value = hash_value;
				natural_entry->m_next_in_chain = -1;
			}
		}
	}

	void	clear()
	// Remove all entries from the hash table.
	{
		if (m_table)
		{
			// Delete the entries.
			for (Sint32 i = 0, n = m_table->m_size_mask; i <= n; i++)
			{
				entry*	e = &E(i);
				if (e->is_empty() == false && e->is_tombstone() == false)
				{
					e->clear();
				}
			}
			free(m_table);
			m_table = NULL;
		}
	}

	bool	is_empty() const
	// Returns true if the hash is empty.
	{
		return m_table == NULL || m_table->m_entry_count == 0;
	}


	bool	get(const T& key, U* value) const
	// Retrieve the value under the given key.
	//
	// If there's no value under the key, then return false and leave
	// *value alone.
	//
	// If there is a value, return true, and set *value to the entry's
	// value.
	//
	// If value == NULL, return true or false according to the
	// presence of the key, but don't touch *value.
	{
		Sint32	index = find_index(key);
		if (index >= 0)
		{
			if (value) {
				*value = E(index).second;
			}
			return true;
		}
		return false;
	}


	Sint32	size() const
	{
		return m_table == NULL ? 0 : m_table->m_entry_count;
	}


	void	check_expand()
	// Resize the hash table to fit one more entry.  Often this
	// doesn't involve any action.
	{
		if (m_table == NULL) {
			// Initial creation of table.  Make a minimum-sized table.
			set_raw_capacity(16);
		} else if (m_table->m_entry_count * 3 > (m_table->m_size_mask + 1) * 2) {
			// Table is more than 2/3rds full.  Expand.
			set_raw_capacity((m_table->m_size_mask + 1) * 2);
		}
	}

	void check_shrink()
	// Shrink our capacity, if it makes sense.
	{
		if (m_table) {
			if (size() * 3 < m_table->m_size_mask + 1) {
				// Table is less than 1/3 full.  Shrink.
				set_capacity(size());
			}
		}
	}

	void	resize(Uint32 n)
	// Hint the bucket count to >= n.
	{
		// Not really sure what this means in relation to
		// STLport's hash_map... they say they "increase the
		// bucket count to at least n" -- but does that mean
		// their real capacity after resize(n) is more like
		// n*2 (since they do linked-list chaining within
		// buckets?).
		set_capacity(n);
	}

	void	set_capacity(Sint32 new_size)
	// Size the hash so that it can comfortably contain the given
	// number of elements.  If the hash already contains more
	// elements than new_size, then this may be a no-op.
	{
		if (new_size < size()) {
			// Make sure requested size is large enough to
			// contain existing elements!
			new_size = size();
		}
		Sint32	new_raw_size = (new_size * 3) / 2;
		set_raw_capacity(new_raw_size);
	}

	// Behaves much like std::pair
	struct entry
	{
		Sint32	m_next_in_chain;	// internal chaining for collisions
		Uint32	m_hash_value;		// avoids recomputing.  Worthwhile?
		T	first;
		U	second;

		entry() : m_next_in_chain(-2) {}
		entry(const entry& e)
			: m_next_in_chain(e.m_next_in_chain), m_hash_value(e.m_hash_value), first(e.first), second(e.second)
		{
		}
		entry(const T& key, const U& value, Sint32 next_in_chain, Sint32 hash_value)
			: m_next_in_chain(next_in_chain), m_hash_value(hash_value), first(key), second(value)
		{
		}
		bool is_empty() const { return m_next_in_chain == -2; }
		bool is_end_of_chain() const { return m_next_in_chain == -1; }
		bool is_tombstone() const { return m_hash_value == TOMBSTONE_HASH; }

		void	clear()
		{
			first.~T();	// placement delete
			second.~U();	// placement delete
			m_next_in_chain = -2;
			m_hash_value = ~TOMBSTONE_HASH;
		}

		void make_tombstone() {
			first.~T();
			second.~U();
			m_hash_value = TOMBSTONE_HASH;
		}
	};
	
	// Iterator API, like STL.

	class const_iterator
	{
	public:
		T	get_key() const { return m_hash->E(m_index).first; }
		U	get_value() const { return m_hash->E(m_index).second; }

		const entry&	operator*() const
		{
			assert(is_end() == false && m_hash->E(m_index).is_empty() == false);
			assert(m_hash->E(m_index).is_tombstone() == false);
			return m_hash->E(m_index);
		}
		const entry*	operator->() const { return &(operator*()); }

		void	operator++()
		{
			assert(m_hash);

			// Find next non-empty entry.
			if (m_index <= m_hash->m_table->m_size_mask)
			{
				m_index++;
				while (m_index <= m_hash->m_table->m_size_mask
					   && (m_hash->E(m_index).is_empty() || m_hash->E(m_index).is_tombstone()))
				{
					m_index++;
				}
			}
		}

		bool	operator==(const const_iterator& it) const
		{
			if (is_end() && it.is_end())
			{
				return true;
			}
			else
			{
				return
					m_hash == it.m_hash
					&& m_index == it.m_index;
			}
		}

		bool	operator!=(const const_iterator& it) const { return ! (*this == it); }


		bool	is_end() const
		{
			return
				m_hash == NULL
				|| m_hash->m_table == NULL
				|| m_index > m_hash->m_table->m_size_mask;
		}

	protected:
		friend class hash<T,U,hash_functor>;

		const_iterator(const hash* h, Sint32 index)
			:
			m_hash(h),
			m_index(index)
		{
		}

		const hash* m_hash;
		Sint32 m_index;
	};
	friend class const_iterator;

	// non-const iterator; get most of it from const_iterator.
	class iterator : public const_iterator
	{
	public:
		// Allow non-const access to entries.
		entry&	operator*() const
		{
			assert(const_iterator::is_end() == false);
			return const_cast<hash*>(const_iterator::m_hash)->E(const_iterator::m_index);
		}
		entry*	operator->() const { return &(operator*()); }

	private:
		friend class hash<T,U,hash_functor>;

		iterator(hash* h, Sint32 i0)
			:
			const_iterator(h, i0)
		{
		}
	};
	friend class iterator;

	void erase(const iterator& pos)
	// Removes the element at pos.
	{
		if (pos.is_end() || pos.m_hash != this) {
			// Invalid iterator.
			return;
		}
		assert(m_table);

		Sint32 natural_index = (Sint32) (pos->m_hash_value & m_table->m_size_mask);

		if (pos.m_index != natural_index) {
			// We're not the head of our chain, so we can
			// be spliced out of it.
			
			// Iterate up the chain, and splice out when
			// we get to m_index.
			entry* e = &E(natural_index);
			while (e->m_next_in_chain != pos.m_index) {
				assert(e->is_end_of_chain() == false);
				e = &E(e->m_next_in_chain);
			}
			if (e->is_tombstone() && pos->is_end_of_chain()) {
				// Tombstone has nothing else to point
				// to, so mark it empty.
				e->m_next_in_chain = -2;
			} else {
				e->m_next_in_chain = pos->m_next_in_chain;
			}
			pos->clear();
		} else if (pos->is_end_of_chain() == false) {
			// We're the head of our chain, and there are
			// additional elements.
			//
			// We need to put a tombstone here.
			//
			// We can't clear the element, because the
			// rest of the elements in the chain must be
			// linked to this position.
			//
			// We can't move any of the succeeding
			// elements in the chain (i.e. to fill this
			// entry), because we don't want to invalidate
			// any other existing iterators.
			pos->make_tombstone();
		} else {
			// We're the head of the chain, but we're the
			// only member of the chain.
			pos->clear();
		}

		m_table->m_entry_count--;
	}

	void erase(const T& key)
	// Removes the element with the given key (if any).
	{
		iterator it = find(key);
		if (it != end()) {
			erase(it);
		}
	}

	iterator	begin()
	{
		if (m_table == 0) return iterator(NULL, 0);

		// Scan til we hit the first valid entry.
		Sint32	i0 = 0;
		while (i0 <= m_table->m_size_mask
			&& (E(i0).is_empty() || E(i0).is_tombstone()))
		{
			i0++;
		}
		return iterator(this, i0);
	}
	iterator	end() { return iterator(NULL, 0); }

	const_iterator	begin() const { return const_cast<hash*>(this)->begin(); }
	const_iterator	end() const { return const_cast<hash*>(this)->end(); }

	iterator	find(const T& key)
	{
		Sint32	index = find_index(key);
		if (index >= 0)
		{
			return iterator(this, index);
		}
		return iterator(NULL, 0);
	}

	const_iterator	find(const T& key) const { return const_cast<hash*>(this)->find(key); }

private:
	// A value of m_hash_value that marks an entry as a
	// "tombstone" -- i.e. a placeholder entry.

	Sint32	find_index(const T& key) const
	// Find the index of the matching entry.  If no match, then return -1.
	{
		if (m_table == NULL) return -1;

		Uint32	hash_value = compute_hash(key);
		Sint32	index = (Sint32) (hash_value & m_table->m_size_mask);

		const entry*	e = &E(index);
		if (e->is_empty()) return -1;
		if (e->is_tombstone() == false && Sint32(e->m_hash_value & m_table->m_size_mask) != index) {
			// occupied by a collider
			return -1;
		}

		for (;;)
		{
			assert(e->is_tombstone() || (e->m_hash_value & m_table->m_size_mask) == (hash_value & m_table->m_size_mask));

			if (e->m_hash_value == hash_value)
			{
				if (m_has_collisions)
				{
					if (e->first == key)
					{
						// Found it, there are collisions and key are equal
						return index;
					}
				}
				else
				{
					// no collisions, hashs are equal
					return index;
				}
			}
			//assert(e->is_tombstone() || ! (e->first == key));	// keys are equal, but hash differs!

			// Keep looking through the chain.
			index = e->m_next_in_chain;
			if (index == -1) break;	// end of chain

			assert(index >= 0 && index <= m_table->m_size_mask);
			e = &E(index);

			assert(e->is_empty() == false || e->is_tombstone());
		}
		return -1;
	}

	Uint32 compute_hash(const T& key) const {
		Uint32 hash_value = hash_functor()(key);
		if (hash_value == TOMBSTONE_HASH) {
			hash_value ^= 0x8000;
		}
		return hash_value;
	}

	// Helpers.
	entry&	E(Sint32 index)
	{
		assert(m_table);
		assert(index >= 0 && index <= m_table->m_size_mask);
		return *(((entry*) (m_table + 1)) + index);
	}
	const entry&	E(Sint32 index) const
	{
		assert(m_table);
		assert(index >= 0 && index <= m_table->m_size_mask);
		return *(((entry*) (m_table + 1)) + index);
	}

	// Return the index of the newly cleared element.
	Sint32 remove_tombstone(Sint32 index) {
		entry* e = &E(index);
		assert(e->is_tombstone());
		assert(!e->is_end_of_chain());

		// Move the next element of the chain into the
		// tombstone slot, and return the vacated element.
		Sint32 new_blank_index = e->m_next_in_chain;
		entry* new_blank = &E(new_blank_index);
		new (e) entry(*new_blank);
		new_blank->clear();
		return new_blank_index;
	}

	void	set_raw_capacity(Sint32 new_size)
	// Resize the hash table to the given size (Rehash the
	// contents of the current table).  The arg is the number of
	// hash table entries, not the number of elements we should
	// actually contain (which will be less than this).
	{
		if (new_size <= 0) {
			// Special case.
			clear();
			return;
		}

		// Force new_size to be a power of two.
		Sint32	bits = fchop((float) log2((float)(new_size-1)) + 1);
		assert((1 << bits) >= new_size);

		new_size = 1 << bits;

		// Minimum size; don't incur rehashing cost when
		// expanding very small tables.
		if (new_size < 16)
		{
			new_size = 16;
		}

		if (m_table && new_size == m_table->m_size_mask + 1) {
			// No change in raw capacity; don't do any work.
			return;
		}

		hash<T, U, hash_functor>	new_hash;
		new_hash.m_table = (table*) malloc(sizeof(table) + sizeof(entry) * new_size);
		assert(new_hash.m_table);	// @@ need to throw (or something) on malloc failure!

		new_hash.m_table->m_entry_count = 0;
		new_hash.m_table->m_size_mask = new_size - 1;
		{for (Sint32 i = 0; i < new_size; i++)
		{
			new_hash.E(i).m_next_in_chain = -2;	// mark empty
		}}
		
		// Copy stuff to new_hash
		if (m_table)
		{
			for (Sint32 i = 0, n = m_table->m_size_mask; i <= n; i++)
			{
				entry*	e = &E(i);
				if (e->is_empty() == false && e->is_tombstone() == false)
				{
					// Insert old entry into new hash.
					new_hash.add(e->first, e->second);
					e->clear();	// placement delete of old element
				}
			}

			// Delete our old data buffer.
			free(m_table);
		}

		// Steal new_hash's data.
		m_table = new_hash.m_table;
		new_hash.m_table = NULL;
	}

	struct table
	{
		Sint32 m_entry_count;
		Sint32 m_size_mask;
		// entry array goes here!
	};
	table*	m_table;
	bool		m_has_collisions;
};


#endif // not _TU_USE_STL



#if _TU_USE_STL == 1
// // tu_string is a subset of std::string, for the most part
 class tu_string : public std::string
 {
 public:

	tu_string();
	tu_string(char ch);
	tu_string(Uint16 ch);
	tu_string(Sint32 val);
	tu_string(double val);
	tu_string(const char* str);
	tu_string(const char* buf, Sint32 buflen);
	tu_string(const tu_string& str);
//	~tu_string();

	 operator const char*() const;
//	 const char*	c_str() const;

	// operator= returns void; if you want to know why, ask Charles Bloom :)
	// (executive summary: a = b = c is an invitation to bad code)
//	 void	operator=(const char* str);
//	 void	operator=(const tu_string& str);
//	 bool	operator==(const char* str) const;
//	 bool	operator!=(const char* str) const;
//	 bool	operator==(const tu_string& str) const;
//	 bool	operator!=(const tu_string& str) const;
	 inline Uint32	size() const {	return std::string::length(); }

	// index >=0
	// void erase(Uint32 index, Sint32 count);

	// insert char before index
	// void insert(Uint32 index, char ch);

	 char&	operator[](Uint32 index);
	 const char&	operator[](Uint32 index) const;
	 void	operator+=(const char* str);
	 void	operator+=(char ch);
	 void	operator+=(Uint16 ch);
	 void	operator+=(Sint32 val);
	 void	operator+=(float val);
	 void	operator+=(double val);

	// Append wide char.  Both versions of wide char.
	 void	append_wide_char(uint16 ch);
	 void	append_wide_char(uint32 ch);

	 void	operator+=(const tu_string& str);
	 tu_string	operator+(const tu_string& str) const;
	 tu_string	operator+(const char* str) const;
	// NOT EFFICIENT!  But convenient.

	 bool	operator<(const char* str) const;
	 bool operator<(const tu_string& str) const;
	 bool	operator>(const char* str) const;
	 bool	operator>(const tu_string& str) const;
	 void clear();
	
	// Sets buffer size to new_size+1 (i.e. enough room for
	// new_size chars, plus terminating 0).
	 void	resize(Sint32 new_size, bool do_copy);

	// Set *result to the UTF-8 encoded version of widechar[].
	// Specialize for different kinds of wide chars.
	//
	// Could add operator= overloads, but maybe it's better to
	// keep this very explicit.
	 static void	encode_utf8_from_uint32(tu_string* result, const uint32* wstr);
	 static void	encode_utf8_from_uint16(tu_string* result, const uint16* wstr);
	 static void	encode_utf8_from_wchar(tu_string* result, const wchar_t* wstr);

	// Utility: case-insensitive string compare.  stricmp() is not
	// ANSI or POSIX, doesn't seem to appear in Linux.
	 static Sint32	stricmp(const char* a, const char* b);

	// Return the Unicode char at the specified character
	// position.  index is in UTF-8 chars, NOT bytes.
	 uint32	utf8_char_at(Uint32 index) const;

	// Return the string in this container as all upper case letters
	 tu_string utf8_to_upper() const;

	// Return the string in this container as all lower case letters
	 tu_string utf8_to_lower() const;

	// Return the number of UTF-8 characters in the given
	// substring buffer.  You must pass in a valid buffer length;
	// this routine does not look for a terminating \0.
	 static Sint32	utf8_char_count(const char* buf, Sint32 buflen);

	 Sint32	utf8_length() const;

	// Returns a tu_string that's a substring of this.  start and
	// end are in UTF-8 character positions (NOT bytes).
	//
	// start is the index of the first character you want to include.
	//
	// end is the index one past the last character you want to include.
	 tu_string	utf8_substring(Sint32 start, Sint32 end) const;

	 void split(const tu_string& delimiter, array<double>* res) const;
	 void split(const tu_string& delimiter, array<tu_string>* res) const;

	 inline bool	get_updated_flag() const	{	return (m_flags & 1) == 0 ? false : true;	}
	 inline void	set_updated_flag() {	m_flags |= 1;	}
	 inline bool	get_hashed_flag() const	{	return (m_flags & 2) == 0 ? false : true;	}
	 inline void	set_hashed_flag() {	m_flags |= 2;	}
	 inline void	clear_flags() { m_flags = 0; }

	 Uint32	compute_hash() const;

private:

	mutable Uint32	m_hash_value;
	mutable Uint8 m_flags;		// 01:updated, 02:hashed
 };

#else

// String-like type.  Attempt to be memory-efficient with small strings.
class tu_string
{
public:
	tu_string();
	tu_string(char ch);
	tu_string(Uint16 ch);
	tu_string(Sint32 val);
	tu_string(double val);
	tu_string(const char* str);
	tu_string(const char* buf, Sint32 buflen);
	tu_string(const tu_string& str);
	~tu_string();

	 operator const char*() const;
	 const char*	c_str() const;

	// operator= returns void; if you want to know why, ask Charles Bloom :)
	// (executive summary: a = b = c is an invitation to bad code)
	 void	operator=(const char* str);
	 void	operator=(const tu_string& str);
	 bool	operator==(const char* str) const;
	 bool	operator!=(const char* str) const;
	 bool	operator==(const tu_string& str) const;
	 bool	operator!=(const tu_string& str) const;
	 inline Uint32	size() const {	return m_size; }

	// index >=0
	 void erase(Uint32 index, Sint32 count);

	// insert char before index
	 void insert(Uint32 index, char ch);

	 char&	operator[](Uint32 index);
	 const char&	operator[](Uint32 index) const;
	 void	operator+=(const char* str);
	 void	operator+=(char ch);
	 void	operator+=(Uint16 ch);
	 void	operator+=(Sint32 val);
	 void	operator+=(float val);
	 void	operator+=(double val);

	// Append wide char.  Both versions of wide char.
	 void	append_wide_char(uint16 ch);
	 void	append_wide_char(uint32 ch);

	 void	operator+=(const tu_string& str);
	 tu_string	operator+(const tu_string& str) const;
	 tu_string	operator+(const char* str) const;
	// NOT EFFICIENT!  But convenient.

	 bool	operator<(const char* str) const;
	 bool operator<(const tu_string& str) const;
	 bool	operator>(const char* str) const;
	 bool	operator>(const tu_string& str) const;
	 void clear();
	
	// Sets buffer size to new_size+1 (i.e. enough room for
	// new_size chars, plus terminating 0).
	 void	resize(Sint32 new_size, bool do_copy);

	// Set *result to the UTF-8 encoded version of widechar[].
	// Specialize for different kinds of wide chars.
	//
	// Could add operator= overloads, but maybe it's better to
	// keep this very explicit.
	 static void	encode_utf8_from_uint32(tu_string* result, const uint32* wstr);
	 static void	encode_utf8_from_uint16(tu_string* result, const uint16* wstr);
	 static void	encode_utf8_from_wchar(tu_string* result, const wchar_t* wstr);

	// Utility: case-insensitive string compare.  stricmp() is not
	// ANSI or POSIX, doesn't seem to appear in Linux.
	 static Sint32	stricmp(const char* a, const char* b);

	// Return the Unicode char at the specified character
	// position.  index is in UTF-8 chars, NOT bytes.
	 uint32	utf8_char_at(Uint32 index) const;

	// Return the string in this container as all upper case letters
	 tu_string utf8_to_upper() const;

	// Return the string in this container as all lower case letters
	 tu_string utf8_to_lower() const;

	// Return the number of UTF-8 characters in the given
	// substring buffer.  You must pass in a valid buffer length;
	// this routine does not look for a terminating \0.
	 static Sint32	utf8_char_count(const char* buf, Sint32 buflen);

	 Sint32	utf8_length() const;

	// Returns a tu_string that's a substring of this.  start and
	// end are in UTF-8 character positions (NOT bytes).
	//
	// start is the index of the first character you want to include.
	//
	// end is the index one past the last character you want to include.
	 tu_string	utf8_substring(Sint32 start, Sint32 end) const;

	 void split(const tu_string& delimiter, array<double>* res) const;
	 void split(const tu_string& delimiter, array<tu_string>* res) const;

	 inline bool	get_updated_flag() const	{	return (m_flags & 1) == 0 ? false : true;	}
	 inline void	set_updated_flag() {	m_flags |= 1;	}
	 inline bool	get_hashed_flag() const	{	return (m_flags & 2) == 0 ? false : true;	}
	 inline void	set_hashed_flag() {	m_flags |= 2;	}
	 inline void	clear_flags() { m_flags = 0; }

	 Uint32	compute_hash() const;

private:

	inline char*	get_buffer() {	return m_size < sizeof(m_local) ? m_local : m_buffer; }
	inline const char*	get_buffer() const { return m_size < sizeof(m_local) ? m_local : m_buffer; }

	Uint32 m_size;  // not counting terminating \0
	char* m_buffer;
	mutable Uint32	m_hash_value;
	mutable Uint8 m_flags;		// 01:updated, 02:hashed
	char m_local[19];
};

#endif

template<class T>
class string_hash_functor
// Computes a hash of a string-like object (something that has
// ::length() and ::[int]).
{
public:

#if _TU_USE_STL == 1
	enum
	{ // parameters for hash table
		bucket_size = 4, // 0 < bucket_size
		min_buckets = 8
	}; // min_buckets = 2 ^^ N, 0 < N 
#endif

	Uint32	operator()(const T& data) const
	{
//		int	size = data.size();
//		return bernstein_hash((const char*) data, size);
		return data.compute_hash();
	}

#if _TU_USE_STL == 1
	// test if s1 ordered before s2
	bool operator()(const char *s1, const char *s2) const
	{
		return (strcmp(s1, s2) < 0);
	} 
#endif
};


template<class U>
class string_hash : public hash<tu_string, U, string_hash_functor<tu_string> >
{
};

// Utility: handy sprintf wrapper.
tu_string string_myprintf(const char* fmt, ...)
#ifdef __GNUC__
	// use the following to catch errors: (only with gcc)
	__attribute__((format (printf, 1, 2)))
#endif	// not __GNUC__
;


#endif // CONTAINER_H

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
