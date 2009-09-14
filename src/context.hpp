#ifndef B98_CONTEXT_HPP_INCLUDED
#define B98_CONTEXT_HPP_INCLUDED

#include "stinkhorn.hpp"

#include "thread.hpp"
#include "stack.hpp"
#include "cursor.hpp"
#include "interpreter.hpp"
#include "fingerprint_stack.hpp"

#include "boost/noncopyable.hpp"

template<class CellT, int Dimensions>
class stinkhorn::Stinkhorn<CellT, Dimensions>::Context : boost::noncopyable {
public:
	Context(Thread& owner, Context* parent, Tree& funge_space);
	~Context();

	//Accessors (can invoke debugger, can be redefined easily)
public:
	bool stringMode() { return m_string_mode; }
	void stringMode(bool value) { m_string_mode = value; }

	bool quitFlag() { return m_quitting; }
	void setQuitFlag() { m_quitting = true; }

	bool space() { return m_space; }
	void space(bool value) { m_space = value; }

	Vector const& storageOffset() { return m_storage_offset; }
	void storageOffset(Vector const& new_storage_offset) { m_storage_offset = new_storage_offset; }

public:
	bool execute(CellT c);

	StackStackT& stack() { return m_stack; }
	Cursor& cursor() { return m_cursor; }
	Tree& fungeSpace() { return m_funge_space; }

public:
	Context* parent() { return m_parent; }
	Thread& owner() { return m_owner; }
	Interpreter& interpreter() { return m_owner.interpreter(); }

	bool pushFingerprint(IFingerprint* fp);
	bool pushFingerprint(IdT id);
	bool popFingerprint(IdT id);

private:
	bool m_string_mode, m_quitting, m_space;
	Vector m_storage_offset;

private:
	FingerprintStack m_fp_stack;

	StackStackT m_stack;
	Tree& m_funge_space;
	Cursor m_cursor;

	Thread& m_owner;
	Context* m_parent;
};

#endif
