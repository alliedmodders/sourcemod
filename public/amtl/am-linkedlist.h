// vim: set sts=8 ts=2 sw=2 tw=99 et:
//
// Copyright (C) 2013, David Anderson and AlliedModders LLC
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
//  * Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//  * Neither the name of AlliedModders LLC nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef _include_amtl_linkedlist_h_
#define _include_amtl_linkedlist_h_

#include <new>
#include <stdlib.h>
#include <am-allocator-policies.h>
#include <am-utility.h>

namespace ke {

// LinkedList, analagous to std::list or SourceHook::List. Since it performs a
// malloc() and free() on every contained node, it should be avoided unless
// absolutely necessary, or for when allocation performance is not a factor. It
// is provided here to safely port old AlliedModders code to AMTL.
//
// In order to use a circular chain, LinkedList's allocation size includes
// exactly one T. If T is very large, LinkedList should be allocated on the
// heap, to avoid using the stack.
template <class T, class AllocPolicy = SystemAllocatorPolicy>
class LinkedList : public AllocPolicy
{
 public:
  friend class iterator;

  class Node
  {
   public:
    Node(const T &o)
     : obj(o)
    {
    }

    T obj;
    Node *next;
    Node *prev;
  };

public:
  LinkedList(AllocPolicy = AllocPolicy())
   : length_(0)
  {
  }
  ~LinkedList() {
    clear();
  }

  bool append(const T &obj) {
    Node *node = allocNode(obj);
    if (!node)
      return false;

    node->prev = head()->prev;
    node->next = head();
    head()->prev->next = node;
    head()->prev = node;

    length_++;
  }

  bool prepend(const T &obj) {
    return insert(begin(), obj);
  }

  size_t size() const {
    return length_;
  }

  void clear() {
    Node *node = head()->next;
    Node *temp;
    head()->next = head();
    head()->prev = head();

    // Iterate through the nodes until we find the sentinel again.
    while (node != head()) {
      temp = node->next;
      freeNode(node);
      node = temp;
    }
    length_ = 0;
  }

  bool empty() const {
    return (length_ == 0);
  }

  T &front() {
    assert(!empty());
    return head()->next->obj;
  }
  T &back() {
    assert(!empty());
    return head()->prev->obj;
  }

 private:
  const Node *head() const {
    return sentinel_.address();
  }
  Node *head() {
    return sentinel_.address();
  }

  Node *allocNode(const T &obj) {
    Node *node = (Node *)this->malloc(sizeof(Node));
    if (!node)
      return NULL;
    new (node) Node(obj);
    return node;
  }

  void freeNode(Node *node) {
    node->obj.~T();
    this->free(node);
  }

 private:
  StorageBuffer<Node> sentinel_;
  size_t length_;

 public:
  class iterator
  {
    friend class LinkedList;

   public:
    iterator()
     : this_(NULL)
    {
    }
    iterator(const LinkedList &src)
     : this_(src.head())
    {
    }
    iterator(Node *n)
     : this_(n)
    {
    }
    iterator(const iterator &where)
     : this_(where.this_)
    {
    }

    iterator &operator --() {
      if (this_)
        this_ = this_->prev;
      return *this;
    }
    iterator operator --(int) {
      iterator old(*this);
      if (this_)
        this_ = this_->prev;
      return old;
    }  
    iterator &operator ++() {
      if (this_)
        this_ = this_->next;
      return *this;
    }
    iterator operator ++(int) {
      iterator old(*this);
      if (this_)
        this_ = this_->next;
      return old;
    }
    
    const T &operator * () const {
      return this_->obj;
    }
    T &operator * () {
      return this_->obj;
    }
    T *operator ->() {
      return &this_->obj;
    }
    const T *operator ->() const {
      return &(this_->obj);
    }
    
    bool operator !=(const iterator &where) const {
      return (this_ != where.this_);
    }
    bool operator ==(const iterator &where) const {
      return (this_ == where.this_);
    }

    operator bool() {
      return !!this_;
    }

   private:
    Node *this_;
  };

 public:
  iterator begin() const {
    return iterator(head()->next);
  }
  iterator end() const {
    return iterator(head());
  }
  iterator erase(iterator &where) {
    Node *pNode = where.this_;
    iterator iter(where);
    iter++;

    pNode->prev->next = pNode->next;
    pNode->next->prev = pNode->prev;

    freeNode(pNode);
    length_--;

    return iter;
  }
  iterator insert(iterator where, const T &obj) {
    // Insert obj right before where
    Node *node = allocNode(obj);
    if (!node)
      return where;

    Node *pWhereNode = where.this_;
    
    pWhereNode->prev->next = node;
    node->prev = pWhereNode->prev;
    pWhereNode->prev = node;
    node->next = pWhereNode;

    length_++;
    return iterator(node);
  }

 public:
  void remove(const T &obj) {
    for (iterator b = begin(); b != end(); b++) {
      if (*b == obj) {
        erase(b);
        break;
      }
    }
  }

  template <typename U>
  iterator find(const U &equ) const {
    for (iterator iter = begin(); iter != end(); iter++) {
      if (*iter == equ)
        return iter;
    }
    return end();
  }


 private:
  // These are disallowed because they basically violate the failure handling
  // model for AllocPolicies and are also likely to have abysmal performance.
  LinkedList &operator =(const LinkedList<T> &other) KE_DELETE;
  LinkedList(const LinkedList<T> &other) KE_DELETE;
};

} // namespace ke

#endif //_INCLUDE_CSDM_LIST_H
