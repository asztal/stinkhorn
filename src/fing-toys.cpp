#include "fingerprint.hpp"
#include "cursor.hpp"
#include "context.hpp"

#include <cstddef>

namespace stinkhorn { namespace {
	template<class T, int D>
	void debug_funge_space_area_numeric(typename Stinkhorn<T, D>::Tree& fs, vector3<T> const& from, vector3<T> const& size) {
		typename Stinkhorn<T, D>::Cursor cr(fs);

		T x, y, z;
		for(z = 0; z < size.z; ++z) {
			for(y = 0; y < size.y; ++y) {
				for(x = 0; x < size.x; ++x) {
					vector3<T> r(x, y, z);

					cr.position(r + from);					
					std::cerr << cr.currentCharacter() << ' ';
				}

				std::cerr << '\n';
			}
		}
	}
} }

namespace stinkhorn {
	template<class CellT, int Dimensions>
	IdT Stinkhorn<CellT, Dimensions>::ToysFingerprint::id() {
		return TOYS_FINGERPRINT;
	}


	template<class CellT, int Dimensions>
	void Stinkhorn<CellT, Dimensions>::ToysFingerprint::copy(Context& ctx, bool low_order, bool erase) 
	{
		const Vector to = ctx.stack().popVector(Dimensions);
		Vector size = ctx.stack().popVector(Dimensions);
		const Vector from = ctx.stack().popVector(Dimensions);

		if(Dimensions == 2)
			size.z = 1;

		//Probably a programmer error
		if(size.x < 0 || size.y < 0 || size.z < 0) {
			ctx.cursor().reflect();
			return;
		}

		if(size.x == 0 || size.y == 0 || size.z == 0)
			return; //Nothing to do

		Cursor source_cursor(ctx.cursor());
		Cursor dest_cursor(source_cursor);

		CellT x, y, z;
		for(z = 0; z < size.z; ++z) {
			for(y = 0; y < size.y; ++y) {
				for(x = 0; x < size.x; ++x) {
					Vector offset(x, y, z);
					if(!low_order)
						offset = size - Vector(1,1,1) - offset;

					source_cursor.position(from + offset);
					dest_cursor.position(to + offset);
					
					CellT temp = source_cursor.get(from + offset);
					dest_cursor.put(to + offset, temp);
					if(erase)
						source_cursor.put(from + offset, ' ');
				}
			}
		}
	}

	//This could probably be trivially optimised by splitting into pages and using std::fill
	//on each row of each page, clipped with the rectangle.
	//However, it's best to do that once everything is actually working.
	template<class CellT, int Dimensions>
	void Stinkhorn<CellT, Dimensions>::ToysFingerprint::chicane(Context& ctx) {
		const Vector from = ctx.stack().popVector(Dimensions);
		Vector size = ctx.stack().popVector(Dimensions);
		Cursor dest_cursor(ctx.cursor());

		CellT value = ctx.stack().pop();

		if(Dimensions == 2)
			size.z = 1;

		//Probably a programmer error
		if(size.x < 0 || size.y < 0 || size.z < 0) {
			ctx.cursor().reflect();
			return;
		}

		if(size.x == 0 || size.y == 0 || size.z == 0)
			return; //Nothing to do

		CellT x, y, z;
		for(z = 0; z < size.z; ++z) {
			for(y = 0; y < size.y; ++y) {
				for(x = 0; x < size.x; ++x) {
					Vector pos = Vector(x, y, z) + from;
					dest_cursor.position(pos);
					dest_cursor.put(pos, value);
				}
			}
		}
	}

	//Left to implement: J, O
	template<class CellT, int Dimensions>
	bool Stinkhorn<CellT, Dimensions>::ToysFingerprint::handleInstruction(CellT instruction, Context& ctx) {
		StackStackT& stack = ctx.stack();
		Cursor& cr = ctx.cursor();

		switch(instruction) {
			case 'C':
				copy(ctx, true, false);
				return true;

			case 'K':
				copy(ctx, false, false);
				return true;

			case 'M':
				copy(ctx, true, true);
				return true;

			case 'V':
				copy(ctx, false, true);
				return true;

			case 'S':
				chicane(ctx);
				return true;

			case 'L':
				{
					stack.push(cr.get(cr.position() + cr.leftwards90Z()));
					return true;
				}

			case 'R': 
				{
					stack.push(cr.get(cr.position() + cr.rightwards90Z()));
					return true;
				}

			case 'I':
				stack.push(stack.pop() + 1);
				return true;

			case 'D':
				stack.push(stack.pop() - 1);
				return true;

			case 'N':
				stack.push(-stack.pop());
				return true;

			//SPEC: I'm assuming it's meant to push a back onto the stack, even though
			//the documentation didn't say so.
			case 'H':
				{
					CellT a, b;
					b = stack.pop();
					a = stack.pop();
					if(b >= 0)
						a <<= b;
					else
						a >>= -b;
					stack.push(a);
					return true;
				}

			case 'A':
				{
					CellT n = stack.pop();
					CellT value = stack.pop();
					if(n < 0)
						cr.reflect();
					else
						while(n--)
							stack.push(value);
					return true;
				}

			case 'B':
				{
					CellT a, b;
					b = stack.pop();
					a = stack.pop();
					stack.push(a+b);
					stack.push(a-b);
					return true;
				}

			case 'E':
				{
					CellT sum = 0;
					while(stack.topStackSize())
						sum += stack.pop();
					stack.push(sum);
					return true;
				}

			case 'P':
				{
					CellT sum = 1;
					while(stack.topStackSize())
						sum *= stack.pop();
					stack.push(sum);
					return true;
				}

			//SPEC: I interpreted the spec as saying that j should be implicitly taken from 
			//the size of the stack... but what do I know?
			//Doing what ccbi does here anyway.
			case 'F':
				{
					Vector least = stack.popVector(Dimensions);
					Cursor matrix_cursor(cr);
					CellT j = stack.pop();
					CellT i = stack.pop();
					if(i <= 0) {
						cr.reflect();
						return true;
					}

					CellT y = 0;
					while(y < j) { 
						for(CellT x = 0; x < i; ++x) {
							matrix_cursor.position(least + Vector(x, y, 0));
							matrix_cursor.put(matrix_cursor.position(), stack.pop());
						}
						++y;
					}

					return true;
				}

				//SPEC: Location of j chosen for compatibility with ccbi
				//(Obviously, j can't be implicit here, unless you terminated on an empty row, but that
				//would be silly)
			case 'G':
				{
					Vector least = stack.popVector(Dimensions);
					Cursor matrix_cursor(cr);
					CellT j = stack.pop();
					CellT i = stack.pop();
					if(i <= 0) {
						cr.reflect();
						return true;
					}

					//SPEC: CCBI does it this way (ie., backwards so that F then G preserves the stack), 
					//but it seems undefined to me.
					for(CellT y = j - 1; y >= 0; --y) {
						for(CellT x = i - 1; x >= 0; --x) {
							matrix_cursor.position(least + Vector(x, y, 0));
							stack.push(matrix_cursor.currentCharacter());
						}
					}

					return true;
				}

			case 'Q':
				cr.put(cr.position() - cr.direction(), stack.pop());
				return true;

			//SPEC: TODO: These should respect hover mode, once it's here...
			case 'T': 
				{
					CellT d = stack.pop();
					Vector current;
					/*if(hover)
						current = cr.direction();*/
					switch(d) {
						case 0: cr.direction(current + (stack.pop() ? Vector(-1, 0, 0) : Vector(1, 0, 0))); break;
						case 1: cr.direction(current + (stack.pop() ? Vector(0, -1, 0) : Vector(0, 1, 0))); break;
						case 2: 
							if(Dimensions > 2)
								cr.direction(current + (stack.pop() ? Vector(0, 0, 1) : Vector(0, 0, -1)));
							else
								cr.reflect();
							break;
						default:
							cr.reflect();
					}

					return true;
				}

				//TODO: Low bits of rand() are the least reliable... Should be changed?
			case 'U':
				{
					assert(Dimensions <= 3);
					CellT result = "<>^vhl"[rand() % (Dimensions * 2)];
					cr.put(cr.position(), result);
					return true;
				}

			case 'W':
				{
					Vector pos = stack.popVector(Dimensions);
					CellT value = stack.pop();
					CellT actual = cr.get(pos + ctx.storageOffset());
					if(actual < value) {
						stack.push(value);

						if(Dimensions > 2)
							stack.push(pos.z);
						stack.push(pos.y);
						stack.push(pos.x);

						cr.position(cr.position() - cr.direction());
					} else if(actual > value) {
						cr.reflect();
					}
					return true;
				}

			case 'X':
				cr.position(cr.position() + Vector(1, 0, 0));
				return true;
				
			case 'Y':
				cr.position(cr.position() + Vector(0, 1, 0));
				return true;

			case 'Z':
				if(Dimensions > 2)
					cr.position(cr.position() + Vector(0, 0, 1));
				else
					cr.reflect();
				return true;

			case 'O': case 'J':
				if(ctx.owner().interpreter().warnings())
					std::cerr << "Warning: Instruction '" << char(instruction) << "' is not yet implemented" << std::endl;
				cr.reflect();
				return true;
		}

		return false;
	}

	template<class CellT, int Dimensions>
	char const* Stinkhorn<CellT, Dimensions>::ToysFingerprint::handledInstructions() {
		return "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	}
}

INSTANTIATE(struct, ToysFingerprint);
