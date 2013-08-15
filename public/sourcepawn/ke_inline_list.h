/**
 * vim: set sts=2 ts=8 sw=2 tw=99 noet :
 * =============================================================================
 * SourcePawn
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 */
#ifndef _include_sourcepawn_inline_list_h_
#define _include_sourcepawn_inline_list_h_

#include <stddef.h>

template <typename T> class InlineList;

template <typename T>
class InlineListNode
{
  friend class InlineList<T>;

  public:
  InlineListNode()
    : next_(NULL),
    prev_(NULL)
  {
  }

  InlineListNode(InlineListNode *next, InlineListNode *prev)
    : next_(next),
    prev_(prev)
  {
  }

  protected:
  InlineListNode *next_;
  InlineListNode *prev_;
};

template <typename T>
class InlineList
{
  typedef InlineListNode<T> Node;

  Node head_;

 public:
  InlineList()
    : head_(&head_, &head_)
  {
  }

 public:
  class iterator
  {
    friend class InlineList;
    Node *iter_;

   public:
    iterator(Node *iter)
      : iter_(iter)
    {
    }

    iterator & operator ++() {
      iter_ = iter_->next;
      return *iter_;
    }
    iterator operator ++(int) {
      iterator old(*this);
      iter_ = iter_->next_;
      return old;
    }
    T * operator *() {
      return static_cast<T *>(iter_);
    }
    T * operator ->() {
      return static_cast<T *>(iter_);
    }
    bool operator !=(const iterator &where) const {
      return iter_ != where.iter_;
    }
    bool operator ==(const iterator &where) const {
      return iter_ == where.iter_;
    }
    iterator prev() const {
      iterator p(iter_->prev_);
      return p;
    }
    iterator next() const {
      iterator p(iter_->next_);
      return p;
    }
  };

  iterator begin() {
    return iterator(head_.next_);
  }

  iterator end() {
    return iterator(&head_);
  }

  void erase(Node *t) {
    t->prev_->next_ = t->next_;
    t->next_->prev_ = t->prev_;
  }

  void insert(Node *t) {
    t->prev_ = head_.prev_;
    t->next_ = &head_;
    head_.prev_->next_ = t;
    head_.prev_ = t;
  }
};

#endif // _include_sourcepawn_inline_list_h_

