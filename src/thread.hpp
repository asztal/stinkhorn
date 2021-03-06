#ifndef B98_THREAD_HPP_INCLUDED
#define B98_THREAD_HPP_INCLUDED

#include "stinkhorn.hpp"

#include <vector>
#include <string>

#include "boost/noncopyable.hpp"

namespace stinkhorn {
	template<class CellT, int Dimensions>
	class Stinkhorn<CellT, Dimensions>::Thread 
		: boost::noncopyable 
	{
	public:	    
		Thread(Interpreter& owner, Tree& funge_space, CellT threadID);
		~Thread();

		Interpreter& interpreter() const;

		bool advance();
		bool execute(CellT c);

		CellT threadID() const;
	    
		std::vector<std::string> const& includeDirectories() const;
		Context& topContext() const;

	private:
		CellT m_threadID;
		Context* m_context;
		Interpreter& owner;
	};
}

#endif
