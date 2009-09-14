#include "fingerprint.hpp"
#include "cursor.hpp"
#include "context.hpp"
#include "octree.hpp"

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

	// There are two cursors, the get cursor and the put cursor.  They travel from one 
	// side of the funge-space to the other, with the put cursor lagging behind the get cursor 
	// somewhat.
	// If the cells are to be moved eastwards, we start at the most eastwards position and go west,
	// moving cells eastwards until we run out of cells.
	template<class CellT, int Dimensions>
	void Stinkhorn<CellT, Dimensions>::ToysFingerprint::move_line(Context& ctx, const Vector& movement_direction, CellT magnitude) {
		if (magnitude == 0)
			return;

		Cursor get(ctx.cursor()), put(ctx.cursor());

		// Figure out where to start.
		{
			Vector min, max;
			ctx.fungeSpace().get_minmax(min, max);
			Vector initial = ctx.cursor().position();

			// It is assumed that the movement direction is cardinal and normalized.
			if(movement_direction.x) {
				initial.x = movement_direction.x > 0 ? max.x : min.x;
			} else if(movement_direction.y) {
				initial.y = movement_direction.y > 0 ? max.y : min.y;
			} else if(movement_direction.z) {
				initial.z = movement_direction.z > 0 ? max.z : min.z;
			} else {
				if(ctx.interpreter().warnings()) {
					std::cerr << "TOYS: J or O instruction passed invalid movement direction " << movement_direction << std::endl;
				}
				ctx.cursor().reflect();
				return;
			}

			put.position(initial + movement_direction * magnitude);
			get.position(initial);
		}

		get.direction(-movement_direction);
		put.direction(-movement_direction);

		while(true) {
			put.put(get.currentCharacter());

			CellT gx0 = dot(get.position(), movement_direction);
			bool gsucc = get.advance(false, false);
			CellT gx1 = dot(get.position(), movement_direction);
		
			CellT px0 = dot(put.position(), movement_direction), px1 = px0;

			if(!gsucc) {
				// No more cells to move. Clear the rest of the put cursor's line -- i.e. clear 
				// those cells which the get cursor didn't clear.
				while(put.advance(false, false));
					put.put(' ');
				break;
			}

			// If the put cursor didn't travel as far as the get cursor, we have to erase the things
			// it lands on until it catches up.
			while(abs(px1 - px0) < abs(gx1 - gx0)) {
				if(px1 != px0) // Don't overwrite the non-space character we've just written.
					put.put(' ');
				bool psucc = put.advance(false, false);
				if(!psucc)
					break;
				px1 = dot(put.position(), movement_direction);
			}

			put.position(get.position() + movement_direction * magnitude);
		}
	}

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
				{
					CellT delta = stack.pop();
					if (delta == 0)
						return true;

					CellT d = delta > 0 ? 1 : -1;
					move_line(ctx, instruction == 'O' ? Vector(d, 0, 0) : Vector(0, d, 0), abs(delta));
					return true;
				}
		}

		return false;
	}

	template<class CellT, int Dimensions>
	char const* Stinkhorn<CellT, Dimensions>::ToysFingerprint::handledInstructions() {
		return "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	}
}

INSTANTIATE(struct, ToysFingerprint);
