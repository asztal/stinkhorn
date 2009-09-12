#include "context.hpp"

using stinkhorn::Stinkhorn;
using stinkhorn::IdT;

template<class CellT, int Dimensions>
Stinkhorn<CellT, Dimensions>::Context::Context(Thread& pwner, Context* parent, Tree& funge_space)
	:
	m_quitting(false), 
	m_storage_offset(),
	m_space(false),
	m_string_mode(false),
	m_owner(pwner),
	m_parent(parent), 
	m_funge_space(funge_space),
	m_cursor(funge_space),
	m_fp_stack(pwner.interpreter().registry())
{
	assert(0 == m_stack.topStackSize());
}

template<class CellT, int Dimensions>
Stinkhorn<CellT, Dimensions>::Context::~Context() {
}

template<class CellT, int Dimensions>
bool Stinkhorn<CellT, Dimensions>::Context::pushFingerprint(IFingerprint* fp) {
	return m_fp_stack.push(fp);
}

template<class CellT, int Dimensions>
bool Stinkhorn<CellT, Dimensions>::Context::pushFingerprint(IdT id) {
	return m_fp_stack.push(id);
}

template<class CellT, int Dimensions>
bool Stinkhorn<CellT, Dimensions>::Context::popFingerprint(IdT id) {
	return m_fp_stack.pop(id);
}

template<class CellT, int Dimensions>
bool Stinkhorn<CellT, Dimensions>::Context::execute(CellT instruction) {
	return m_fp_stack.execute(instruction, *this);
}

INSTANTIATE(class, Context);
