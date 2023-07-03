// vim: set ts=4 sw=4 tw=99 noet :
// =============================================================================
// SourceMod
// Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
// =============================================================================
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License, version 3.0, as published by the
// Free Software Foundation.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
//
// As a special exception, AlliedModders LLC gives you permission to link the
// code of this program (as well as its derivative works) to "Half-Life 2," the
// "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
// by the Valve Corporation.  You must obey the GNU General Public License in
// all respects for all other code used.  Additionally, AlliedModders LLC grants
// this exception to all derivative works.  AlliedModders LLC defines further
// exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
// or <http://www.sourcemod.net/license.php>.
#ifndef _include_sourcemod_reentrant_iterator_h_
#define _include_sourcemod_reentrant_iterator_h_

#include <algorithm>
#include <list>
#include <utility>

#include <amtl/am-function.h>

namespace SourceMod {

// ReentrantList is a wrapper around a std::list, with special attention twoard
// reentrancy. The list may be mutated during iteration as long as its iterator
// protocol is obeyed: iterators may only be used on the stack in a LIFO manner,
// and it is illegal to request access to an iterator's item after the list has
// been mutated.
//
// We guard against this using assertions. If an item in a list is removed, and
// this affects any active iterators, those iterators cannot be used until their
// next() function is called.
template <typename T>
class ReentrantList : public std::list<T>
{
	typedef typename std::list<T> BaseType;
	typedef typename std::list<T>::iterator BaseTypeIter;

public:
	ReentrantList()
	  : top_(nullptr)
	{
	}

	// Begin and end are disallowed here.
	void begin() = delete;
	void end() = delete;

	class iterator
	{
		friend class ReentrantList;
	public:
		iterator(ReentrantList& list)
			: iterator(&list)
		{}
		iterator(ReentrantList* list)
			: list_(list),
			  prev_(list_->top_),
			  impl_(list->impl_begin()),
			  mutated_(false)
		{
			list_->top_ = this;
		}
		~iterator() {
			assert(list_->top_ == this);
			list_->top_ = prev_;
		}

		// Returns true if the underlying iterator mutated during iteration.
		bool mutated() const {
			return mutated_;
		}
		bool more() {
			return impl_ != list_->impl_end();
		}
		bool done() {
			return impl_ == list_->impl_end();
		}

		// Accessors. It is illegal to access the current item if the list has
		// mutated.
		const T& operator *() const {
			assert(!mutated());
			return *impl_;
		}
		T& operator *() {
			assert(!mutated());
			return *impl_;
		}
		T* operator ->() {
			assert(!mutated());
			return &*impl_;
		}
		const T* operator ->() const {
			assert(!mutated());
			return &*impl_;
		}

		void next() {
			if (mutated_) {
				mutated_ = false;
				return;
			}
			impl_++;
		}

		void remove() {
			BaseTypeIter old_impl = impl_;
			
			impl_ = list_->erase(impl_);
			mutated_ = true;
			for (iterator* p = prev_; p; p = p->prev_) {
				if (p->impl_ == old_impl) {
					p->impl_ = impl_;
					p->mutated_ = true;
				}
			}
		}

	private:
		ReentrantList* list_;
		iterator* prev_;
		BaseTypeIter impl_;
		bool mutated_;
	};

	// Same protocol as LinkedList::remove, but we use our own iterator.
	void remove(const T& obj) {
		for (iterator iter(this); !iter.done(); iter.next()) {
			if (*iter == obj) {
				iter.remove();
				break;
			}
		}
	}

	template <typename U>
	void insertBefore(iterator& where, U &&obj) {
		BaseType::insert(where.impl_, std::forward<U>(obj));
	}

	template <typename U>
	bool contains(const U &obj) {
		return std::find(BaseType::begin(), BaseType::end(), obj) != BaseType::end();
	}

private:
	BaseTypeIter impl_begin() {
		return BaseType::begin();
	}
	BaseTypeIter impl_end() {
		return BaseType::end();
	}

private:
	iterator* top_;
};

} // namespace SourceMod

#endif // _include_sourcemod_reentrant_iterator_h_
