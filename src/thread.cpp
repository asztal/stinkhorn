#include "config.hpp"
#include "thread.hpp"
#include "octree.hpp"
#include "context.hpp"
#include "fingerprint.hpp"

#include <iostream>

using std::cerr;
using std::vector;
using std::string;

using boost::shared_ptr;

namespace stinkhorn {
	/**
	* Start the IP at (0, 0, 0) moving east.
	*/
	template<class CellT, int Dimensions>
	Stinkhorn<CellT, Dimensions>::Thread::Thread(Interpreter& owner, Tree& funge_space, CellT threadID)
		: owner(owner)
		, m_threadID(threadID)
	{
		m_context = new Context(*this, 0, funge_space);

		IFingerprint* f;

		if(owner.isBefunge93())
			f = new Befunge93Fingerprint;
		else if(owner.isTrefunge())
			f = new TrefungeFingerprint;
		else
			f = new Befunge98Fingerprint;

		m_context->pushFingerprint(f);

		//It is now the fingerprint stack's responsibility.
		f->release();
	}

	template<class CellT, int Dimensions>
	Stinkhorn<CellT, Dimensions>::Thread::~Thread() {
		delete m_context;
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::Thread::advance() {
		assert(m_context);

		Cursor& cr = m_context->cursor();
		CellT c = cr.currentCharacter();

		if(m_context->stringMode()) {
			if(m_context->space() && c != ' ') {
				m_context->stack().push(' ');
				m_context->space(false);
			}

			if(c == '\"') {
				m_context->stringMode(false);
			} else {
				if(c == ' ') {
					m_context->space(true);
				} else {
					m_context->stack().push(c);
				}
			}
		} else {
			if(!execute(c)) {
				if(owner.warnings())
					std::cerr << /*stinkhorn::warning <<*/ "Unknown instruction: " << 
					static_cast<char>(c) << " (" << static_cast<int>(c) << ")" <<
					std::endl;
			}
		}

		Vector old_ip = cr.position();

		if(cr.advance(!m_context->stringMode())) {
			//If we skipped over some spaces, make sure we account for them. Go
			//backwards one step so that we take one tick longer.
			if(m_context->stringMode() && cr.position() - cr.direction() != old_ip) {
				m_context->space(true);
				cr.position(cr.position() - cr.direction());
				Vector v = cr.position();
			}
		} else {
			endl(cerr << "\n\n** COMMENCING INFINITE LOOP **");
			while(1);
		}

		return !m_context->quitFlag();
	}

	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::Thread::execute(CellT c) {
		assert(m_context);

		return m_context->execute(c);
	}

	template<class CellT, int Dimensions>
	CellT Stinkhorn<CellT, Dimensions>::Thread::threadID() const {
		return m_threadID;
	}

	template<class CellT, int Dimensions>
	vector<string> const& Stinkhorn<CellT, Dimensions>::Thread::includeDirectories() const {
		return owner.includeDirectories();
	}

	template<class CellT, int Dimensions>
	typename Stinkhorn<CellT, Dimensions>::Context&
	Stinkhorn<CellT, Dimensions>::Thread::topContext() const
	{
		assert(m_context);
		return *m_context;
	}

	template<class CellT, int Dimensions>
	typename Stinkhorn<CellT, Dimensions>::Interpreter& 
	Stinkhorn<CellT, Dimensions>::Thread::interpreter() const
	{
		return owner;
	}
}

INSTANTIATE(class, Thread);
