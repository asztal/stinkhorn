#include "fingerprint.hpp"
#include "context.hpp"

namespace stinkhorn {
	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::OrthFingerprint::handleInstruction(CellT instruction, Context& ctx) {
		StackStack& stack = ctx.stack();

		switch(instruction) {
			case 'A': 
				stack.push(stack.pop() & stack.pop());
				return true;

			case 'O': 
				stack.push(stack.pop() | stack.pop());
				return true;

			case 'E': 
				stack.push(stack.pop() ^ stack.pop());
				return true;

			case 'X':
				{
					Vector pos = ctx.cursor().position();
					pos.x = stack.pop();
					ctx.cursor().position(pos);
					return true;
				}

			case 'Y':
				{
					Vector pos = ctx.cursor().position();
					pos.y = stack.pop();
					ctx.cursor().position(pos);
					return true;
				}

			case 'V':
				{
					Vector dir = ctx.cursor().direction();
					dir.x = stack.pop();
					ctx.cursor().direction(dir);
					return true;
				}

			case 'W':
				{
					Vector dir = ctx.cursor().direction();
					dir.y = stack.pop();
					ctx.cursor().direction(dir);
					return true;
				}

			//I guess the point of these is that they don't respect storage offsets, since they don't seem
			//to be much different from g and p (except that g fails when the cell is uninitialized, which
			//seems impractical to fully implement in this interpreter).
			//Another differences is that these are explicitly 2D.
			case 'G':
				{
					Vector v;
					v.x = stack.pop();
					v.y = stack.pop();
					stack.push(ctx.cursor().get(v));
					return true;
				}

			case 'P':
				{
					Vector v;
					v.x = stack.pop();
					v.y = stack.pop();
					ctx.cursor().put(v, stack.pop());
					return true;
				}

			case 'Z':
				if(stack.pop() == 0)
					ctx.cursor().position(ctx.cursor().position() + ctx.cursor().direction());
				return true;

			case 'S': 
				{
					String str;
					stack.readString(str);
					writeString(std::cout, str);
					return true;
				}
		}

		return false;
	}

	template<class CellT, int Dimensions>
	IdT Stinkhorn<CellT, Dimensions>::OrthFingerprint::id() {
		return ORTH_FINGERPRINT;
	}

	template<class CellT, int Dimensions>
	char const* Stinkhorn<CellT, Dimensions>::OrthFingerprint::handledInstructions() {
		return "AOEXYVWGPZS";
	}
}

INSTANTIATE(struct, OrthFingerprint);
