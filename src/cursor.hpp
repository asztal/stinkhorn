#ifndef B98_CURSOR_HPP_INCLUDED
#define B98_CURSOR_HPP_INCLUDED

#include "stinkhorn.hpp"

#include "config.hpp"
#include "vector.hpp"

namespace stinkhorn {
	/**
	 * A cursor represents an instruction pointer which is bound to a funge space
	 * and contains a position and a direction. The purpose of this class is to
	 * provide almost-transparent caching ability and to speed up advancing the
	 * cursor, by having the cursor be stateful.

	 * The cursor caches the current page in funge space on which it stands. This
	 * avoids costly tree lookups.
	 */
	template<class CellT, int Dimensions>
	class Stinkhorn<CellT, Dimensions>::Cursor {
		typedef TreePage PageT;
		typedef TreeNode<CellT, Dimensions> NodeT;

	public:
		Cursor(Tree& tree);

		Cursor(Cursor& other) :
			m_tree(other.m_tree),
			m_position(other.m_position),
			m_direction(other.m_direction),
			m_page_address(other.m_page_address),
			m_page(other.m_page)
		{}

		//Getter/setter for the cursor's position. Note that setting the position
		//may invalidate the cursor's cached page.
		void position(Vector const& new_position);

		Vector const& position() const {
			return m_position;
		}

		//Gets/sets the cursor's direction.
		void direction(Vector const& new_direction) {
			m_direction = new_direction;
		}

		Vector const& direction() const {
			return m_direction;
		}

		//Attempts to advance the cursor within the range of current page. If the
		//cursor goes outside the current page, the proper advance_cursor method of
		//octree is called, and the cursor caches the result. Returns false if there
		//are no instructions found in the path of the cursor.
		bool advance(bool follow_teleports = true, bool can_wrap = true);

		//Teleports the cursor to the next ; in the funge-space. If one is not found, 
		//simply arrives at itself, effectively acting as if the instruction was a z.
		void teleport();

		void reflect() {
			direction(-direction());
		}

		Vector leftwards90Z();
		Vector rightwards90Z();

		//Gets the character at the cursor's location in the funge-space. This
		//method cannot fail, it will simply return 32 (space) if there is no data
		//where the cursor is.
		CellT currentCharacter();

		//Attempts to fetch a value from funge-space using the cursor's cached page;
		//if this fails, the get method of octree is called.
		CellT get(Vector const& location);
		void put(Vector const& location, CellT value);
		void put(CellT value);

	protected:
		// Like the funge-space advance_cursor, but using page cache more cleverly.
		bool advance_fast(const Vector& from, Vector& to, bool in_hyperspace, bool can_wrap);

		void getPage();

	private:
		PageT* m_page;
		Vector m_page_address,
			   m_position,
			   m_direction;
		Tree& m_tree;
	};
}

#endif
