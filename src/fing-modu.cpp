#include "fingerprint.hpp"
#include "context.hpp"

//Go with whatever CCBI does, since I don't know the exact definitions of these.
namespace {
	template<class CellT>
	CellT floorDiv(CellT x, CellT y) {
		x /= y;
		if(x < 0)
			return x - 1;
		return x;
	}
}

namespace stinkhorn {
	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::ModuFingerprint::handleInstruction(CellT instruction, Context& ctx) {
		StackStack& stack = ctx.stack();

		switch(instruction) {
			case 'M': 
				{
					CellT y = stack.pop();
					CellT x = stack.pop();

					if(y == 0) {
						stack.push(0);
					} else {
						stack.push(x - floorDiv(x, y) * y);
					}
					return true;
				}

			case 'U':
				{
					CellT y = stack.pop();
					CellT x = stack.pop();
					if(!y) {
						stack.push(0);
					} else {
						//Don't ask me how this works
						CellT rem = x % y;
						if(rem < 0) {
							CellT mask = y >> (sizeof(y)*CHAR_BIT - 1);
							rem += (y + mask) ^ mask;
						}
						stack.push(rem);
					}
					return true;
				}

			case 'R':
				{
					CellT y = stack.pop();
					CellT x = stack.pop();

					if(!y) {
						stack.push(0);
					} else {
						CellT rem = x % y;
						if(x > 0 != rem > 0)
						//if((x <= 0 && r <= 0) || (x >= 0 && r >= 0))
							rem = -rem;
						stack.push(rem);
					}
					return true;
				}
		}

		return false;
	}

	template<class CellT, int Dimensions>
	IdT Stinkhorn<CellT, Dimensions>::ModuFingerprint::id() {
		return MODU_FINGERPRINT;
	}

	template<class CellT, int Dimensions>
	char const* Stinkhorn<CellT, Dimensions>::ModuFingerprint::handledInstructions() {
		return "MUR";
	}

}

INSTANTIATE(struct, ModuFingerprint);
