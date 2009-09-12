/**
All code in this file is copyright (c) 2006 Lee Houghton <lee at chunkybacon
dot org>.

Permission is granted to republish this work on the condition that the above
copyright message and this message remain included unchanged in all copies.

* * *

Some of the documentation in this file (paragraphs marked by ~) is taken
from the Funge-98 technical specification by Chris Pressey, which is subject
to the following license terms:

Copyright (c) 2000 Chris Pressey, Cat's Eye Technologies. Permission is
granted to republish this work on the condition that the above copyright
message and this message remain included unchanged in all copies.

The Funge-98 technical specification can be found at:
http://catseye.mine.nu:8080/projects/funge98/doc/funge98.html
**/

#ifndef B98_STACK_HPP_INCLUDED
#define B98_STACK_HPP_INCLUDED

#include "vector.hpp"
#include "config.hpp"

#include <stack>
#include <string>
#include <vector>
#include <iterator>
#include <algorithm>

#ifndef B98_NO_STACK_POOL
#define NOMINMAX //Sigh, boost includes windows.h
#include <boost/pool/pool_alloc.hpp>
#endif

namespace stinkhorn {
	/**
	The stack stack transparently overlays the stack - that is to say, the top
	stack of Funge-98's stack stack is treated the same as Befunge-93's sole
	stack. The Funge programmer will never notice the difference unless they use
	the {, }, or u instructions of Funge-98.

	When working with different stacks on the stack stack, though, it's useful
	to give two of them names: the top of stack stack or TOSS, which indicates
	the topmost stack on the stack stack, which works to emulate the sole stack
	of Befunge-93; and the second on stack stack or SOSS, which is the stack
	directly under the TOSS.

	TODO:
	- Support reverse mode and clear mode for the MODE fingerprint.
	**/
	template<class T>
	class StackStack {
	public:
		typedef T CellT;
		typedef vector3<T> VectorT;
		typedef std::basic_string<CellT> String;
#ifndef B98_NO_STACK_POOL
		typedef std::deque<T, boost::pool_allocator<T, boost::default_user_allocator_new_delete, boost::details::pool::null_mutex> > StorageT;
#else
		typedef std::deque<T> StorageT;
#endif

		StackStack();
		StackStack(StackStack<T> const& other);

		void push(T value);
		T pop();
		VectorT popVector(int dimensions);
		VectorT pushVector(VectorT const& v, int dimensions);

		void pushStack(VectorT const& storageOffset, int dimensions);
		bool popStack(VectorT& storageOffset, int dimensions);

		void pushStackNoSemantics();
		void popStackNoSemantics();

		//Implements u.
		bool transfer(CellT elements);

		void clearTopStack();

		void pushBack(T value);
		CellT popBack();

		void pushFront(CellT value);
		CellT popFront();

		T nth(std::size_t index);
		std::size_t stackCount();
		std::size_t topStackSize();
		void resizeTopStack(std::size_t size);

		template<class OutIter>
		void getStackSizes(OutIter);

		template<class U> //U is some sort of string (could be char, could be T)
		void readString(U& out);
		void pushString(String const& str);

		//STRN operations (put here because it's easier and faster)
		CellT strnGetLength();

		//These two modes correspond to the I and Q instructions of the MODE fingerprint
		bool invertMode() const { return m_invert_mode; }
		void invertMode(bool value) { m_invert_mode = value; }

		bool queueMode() const { return m_queue_mode; }
		void queueMode(bool value) { m_queue_mode = value; }

	private:
		bool m_invert_mode;
		bool m_queue_mode;

		StorageT values;
		std::stack<std::size_t> indices;
	};

	template<class T>
	StackStack<T>::StackStack() {
		m_invert_mode = m_queue_mode = false;
	}

	template<class T>
	StackStack<T>::StackStack(StackStack<T> const& other) 
		: m_invert_mode(other.m_invert_mode),
		m_queue_mode(other.m_queue_mode),
		values(other.values),
		indices(other.indices)
	{
	}

	template<class T>
	void StackStack<T>::push(CellT value) {
		if(m_invert_mode)
			pushFront(value);
		else
			pushBack(value);
	}

	template<class T>
	typename StackStack<T>::CellT StackStack<T>::pop() {
		if(m_queue_mode)
			return popFront();
		else
			return popBack();
	}

	template<class T>
	void StackStack<T>::pushBack(T value) {
		values.push_back(value);
	}

	template<class T>
	T StackStack<T>::popBack() {
		if(indices.size() && indices.top() == values.size()) {
			return 0;
		}

		if(values.size()) {
			T x = values.back();
			values.pop_back();
			return x;
		}

		return 0;
	}

	//I *assume* this is how it should work in Queue Mode.
	template<class T>
	T StackStack<T>::nth(std::size_t index) {
		if(index >= topStackSize())
			return CellT();

		if(m_queue_mode) {
			std::size_t toss_begin = indices.size() ? indices.top() : 0;
			return values[toss_begin + index];
		} else {
			return values.rbegin()[index];
		}
	}

	template<class T>
	std::size_t StackStack<T>::stackCount() {
		return indices.size() + 1;
	}

	template<class T>
	std::size_t StackStack<T>::topStackSize() {
		return values.size() - (indices.size() ? indices.top() : 0);
	}

	template<class T>
	void StackStack<T>::resizeTopStack(std::size_t size) {
		std::size_t toss_begin = indices.size() ? indices.top() : 0;
		values.resize(toss_begin + size);
	}

	template<class T>
	template<class OutIter>
	void StackStack<T>::getStackSizes(OutIter out) {
		std::stack<size_t> x = indices;

		*out++ = values.size() - (indices.size() ? indices.top() : 0);
		while(x.size()) {
			*out++ = x.top();
			x.pop();
		}
	}

	/**
	Pushes a new stack onto the stack stack.

	~ The { "Begin Block" instruction pops a cell it calls n, then pushes a new
	stack on the top of the stack stack, transfers n elements from the SOSS to
	the TOSS, then pushes the storage offset as a vector onto the SOSS, then
	sets the new storage offset to the location to be executed next by the IP
	(storage offset <- position + delta). It copies these elements as a block,
	so order is preserved.

	~ If the SOSS contains k elements, where k is less than n, the k elements are
	transferred as the top k elements and the remaining bottom (n-k) elements
	are filled in with zero-value cells.

	~ If n is zero, no elements are transferred.

	~ If n is negative, |n| zeroes are pushed onto the SOSS. 

	I think the last two paragraphs mean k, not n. This function takes a reference
	to the thread's state in order to modify its storage offset.
	**/
	template<class T>
	void StackStack<T>::pushStack(VectorT const& current_storage_offset, int dimensions) {
		CellT transfer = pop();
		std::size_t zeroes = 0;

		std::vector<CellT> buffer;

		//Work out how many zeroes we need and copy any elements we are transferring
		//into the temporary buffer. 
		if(transfer > 0) {
			std::size_t soss_begin = indices.size() ? indices.top() : 0;
			std::size_t available = values.size() - soss_begin;

			if(static_cast<std::size_t>(transfer) > available) {
				zeroes = static_cast<std::size_t>(transfer) - available;
				transfer = static_cast<CellT>(available);
				assert(transfer >= 0); //Could happen due to overflow
			}

			buffer.reserve(static_cast<std::size_t>(transfer));
			std::copy(values.end() - static_cast<std::size_t>(transfer), values.end(), std::back_inserter(buffer));
			values.resize(values.size() - static_cast<std::size_t>(transfer));
		} else {
			zeroes = static_cast<std::size_t>(-transfer);
		}

		//Push the current storage offset
		push(current_storage_offset.x);
		push(current_storage_offset.y);
		if(dimensions > 2)
			push(current_storage_offset.z);

		if(transfer < 0) {
			//This goes before indices.push(), not after, because it's the SOSS.
			while(zeroes-- > 0)
				values.push_back(CellT(0));
			indices.push(values.size());
		} else {
			indices.push(values.size());
			while(zeroes-- > 0)
				values.push_back(CellT(0));
		}

		std::copy(buffer.begin(), buffer.end(), std::back_inserter(values));
	}

	/**
	~ The corresponding } "End Block" instruction pops a cell off the stack that
	it calls n, then pops a vector off the SOSS which it assigns to the storage
	offset, then transfers n elements (as a block) from the TOSS to the SOSS,
	then pops the top stack off the stack stack.

	~ The transfer of elements for } "End Block" is in all respects similar to the
	transfer of elements for { "Begin Block", except for the direction in which
	elements are transferred. "Transfer" is used here in the sense of "move,"
	not "copy": the original cells are removed.

	~ If n is zero, no elements are transferred.

	~ If n is negative, |n| cells are popped off of the (original) SOSS. 

	I think the last two paragraphs mean k, not n. This function returns false if
	there is no SOSS, that is, a stack-stack underflow would occur. Returns true
	if the operation is successful.
	**/
	template<class T>
	bool StackStack<T>::popStack(VectorT& storage_offset, int dimensions) {
		CellT transfer;
		std::size_t zeroes = 0;
		VectorT old_storage_offset(0, 0, 0);
		std::vector<CellT> buffer;

		//Before modifying the stack stack, ensure that the operation will succeed.
		//The operation can only fail if there isn't a SOSS to return to.
		if(indices.empty())
			return false;

		//Determine how many cells to transfer and how many zeroes to copy onto the
		//SOSS
		transfer = pop();
		if(transfer > 0) {
			//indices is guaranteed to have entries
			std::size_t toss_begin = indices.top();
			std::size_t available = values.size() - toss_begin;

			if(static_cast<std::size_t>(transfer) > available) {
				zeroes = static_cast<std::size_t>(transfer) - available;
				transfer = static_cast<CellT>(available);
				assert(transfer >= 0); //Could happen due to overflow
			}

			buffer.reserve(static_cast<std::size_t>(transfer));
			std::copy(values.begin() + toss_begin, values.end(), std::back_inserter(buffer));
		} else {
			zeroes = static_cast<std::size_t>(-transfer);
		}

		//Remove the TOSS
		values.resize(indices.top());
		indices.pop();

		//Restore the old storage offset, which is stored at the top of the SOSS.
		//Probably should be moved to a separate function if the need arises.
		if(dimensions > 2)
			storage_offset.z = pop();
		storage_offset.y = pop();
		storage_offset.x = pop();

		//Transfer cells and copy zeroes onto the SOSS
		if(transfer > 0)
			while(zeroes-- > 0)
				values.push_back(CellT(0));
		else
			while(transfer++)
				pop();

		std::copy(buffer.begin(), buffer.end(), std::back_inserter(values));

		return true;
	}

	/**
	Transfers elements from the SOSS to the TOSS. If there is no SOSS, the
	function returns false, and it is up to the caller to reflect the IP's
	delta.

	~ The u "Stack under Stack" instruction pops a count and transfers that many
	cells from the SOSS to the TOSS. It transfers these cells in a pop-push
	loop. In other words, the order is not preserved during transfer, it is
	reversed.

	~ If there is no SOSS (the TOSS is the only stack), u should act like r.

	~ If count is negative, |count| cells are transferred (similarly in a pop-push
	loop) from the TOSS to the SOSS.

	~ If count is zero, nothing happens. 
	**/
	template<class T>
	bool StackStack<T>::transfer(CellT elements) {
		if(indices.empty())
			return false;

		//In the negative case, pop() n times into a buffer, then splice the buffer into this->values and bump up the index of where the TOSS starts.
		if(elements < 0) {
			std::size_t avail = topStackSize();
			std::vector<CellT> buffer;

			CellT n = -elements;
			while(n--)
				buffer.push_back(pop());

			values.insert(values.begin() + indices.top(), buffer.begin(), buffer.end());
			indices.top() += std::size_t(-elements);
			return true;
		}

		if(elements == 0)
			return true;

		std::size_t toss_begin, soss_begin;

		toss_begin = indices.top();
		indices.pop();
		soss_begin = indices.size() ? indices.top() : 0;
		indices.push(toss_begin);

		//Determine how many elements we're *really* going to transfer, and how many
		//zeroes will be needed.
		std::size_t available = toss_begin - soss_begin;
		std::size_t zeroes = 0;
		if(static_cast<std::size_t>(elements) > available) {
			zeroes = static_cast<std::size_t>(elements) - available;
			elements = static_cast<CellT>(available);
			assert(elements >= 0); //Could happen due to overflow
		}

		std::vector<CellT> buffer;
		buffer.reserve(static_cast<std::size_t>(elements));
		std::copy(values.begin() + toss_begin - static_cast<std::size_t>(elements), values.begin() + toss_begin, std::back_inserter(buffer));

		//Copy them in backwards, remove them from the SOSS and add on the necessary zeroes
		std::copy(buffer.rbegin(), buffer.rend(), std::back_inserter(values));
		values.erase(values.begin() + toss_begin - static_cast<std::size_t>(elements), values.begin() + toss_begin);
		indices.top() -= static_cast<std::size_t>(elements);

		while(zeroes--)
			values.push_back(CellT(0));

		return true;
	}

	template<class T>
	void StackStack<T>::clearTopStack() {
		if(indices.empty()) {
			values.clear();
			return;
		}

		values.resize(indices.top());
	}

	/**
	Reads a string from the top stack into out.

	The following snippet pushes the string "Beyond Daylight" onto the stack:
	0"thgilyaD dnoyeB"

	That is, the characters of the string are pushed on last character first.
	Strings on the stack use null-terminators to indicate the end of the string.
	**/
	//TODO: Handle queue mode/insert mode
	template<class T>
	template<class U>
	void StackStack<T>::readString(U& out) {
		typedef typename U::size_type size_type;
		size_type lower = indices.size() ? indices.top() : 0;
		out.clear();

		if(values.size() == 0)
			return;

		size_type pos = values.size() - 1;
		while(true) {
			if(pos == lower || values[pos] == 0) {
				if(pos == lower && values[pos] != 0)
					out += char(values[pos]);
				break;
			}

			out += char(values[pos]);

			--pos;
		}

		values.resize(pos);
	}

	//TODO: Handle queue mode/insert mode
	template<class T>
	void StackStack<T>::pushString(String const& str) {
		values.push_back(0);
		for(typename String::size_type i = 0, len = str.size(); i < len; i++)
			values.push_back(str[len - 1 - i]);
	}

	//TODO: Handle queue mode/insert mode
	template<class T>
	T StackStack<T>::strnGetLength() {
		//Find how many cells we would need to pop to get a zero.
		typename String::size_type lower = indices.size() ? indices.top() : 0;
		typename String::size_type max = values.size() - lower;
		
		for(typename StorageT::const_reverse_iterator itr = values.rbegin(), rend = values.rend(); itr != rend; ++itr) {
			if(*itr == 0)
				return static_cast<CellT>(itr - values.rbegin());
		}

		return max;
	}

	//Ouch. I didn't foresee needing to implement something like this.
	//O(n) complexity just for a push is not nice (only when there is
	//more than one stack on the stack stack)
	template<class T>
	void StackStack<T>::pushFront(CellT value) {
		if(indices.empty())
			values.push_front(value);
		else
			values.insert(values.begin() + indices.top(), value);
	}

	template<class T>
	typename StackStack<T>::CellT StackStack<T>::popFront() {
		if(indices.empty()) {
			if(values.empty())
				return CellT(); //Whole stack stack is empty

			//No SOSS
			CellT c = values.front();
			values.pop_front();
			return c;
		} else {
			if(values.size() == indices.top())
				return CellT(); //TOSS is empty
			typename StorageT::iterator itr = values.begin() + indices.top();
			CellT c = *itr;
			values.erase(itr);
			return c;
		}
	}

	template<class T>
	typename StackStack<T>::VectorT StackStack<T>::popVector(int dimensions) {
		VectorT v;
		if(dimensions > 2)
			v.z = pop();
		if(dimensions > 1)
			v.y = pop();
		v.x = pop();
		return v;
	}

	template<class T>
	typename StackStack<T>::VectorT StackStack<T>::pushVector(VectorT const& v, int dimensions) {
		push(v.x);
		if(dimensions > 1)
			push(v.y);
		if(dimensions > 2)
			push(v.z);
		return v;
	}


	template<class T>
	void StackStack<T>::popStackNoSemantics() {
		if(indices.empty())
			return;

		values.resize(indices.top());
		indices.pop();
	}

	template<class T>
	void StackStack<T>::pushStackNoSemantics() {
		indices.push(values.size());
	}
}

#endif
