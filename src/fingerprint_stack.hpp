#ifndef B98_FINGERPRINT_STACK_HPP_INCLUDED
#define B98_FINGERPRINT_STACK_HPP_INCLUDED

#include "stinkhorn.hpp"

#include "fingerprint.hpp"
#include <vector>
#include <stack>

namespace stinkhorn {
	template<class CellT, int Dimensions>
	class Stinkhorn<CellT, Dimensions>::FingerprintStack {
	public:
		FingerprintStack(FingerprintRegistry& registry);
		~FingerprintStack();

		IFingerprint* get(IdT id);

		bool execute(CellT c, Context& ctx);

		bool push(IdT id);
		bool push(IFingerprint* builtin);
		bool pop(IdT id);

	protected:
		void release(IFingerprint* fp);

	private:
		std::vector<IFingerprint*> stack;

		//This is kept so that we can avoid duplicating existing fingerprints. 
		//It also seems better than scanning the list of loaded semantics.
		std::vector<IFingerprint*> all;
		FingerprintRegistry& registry;
		std::stack<IFingerprint*, std::vector<IFingerprint*> > semantics[26];
	};
}

#endif
