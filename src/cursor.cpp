#include "cursor.hpp"
#include "octree.hpp"

using stinkhorn::Stinkhorn;

template<class CellT, int Dimensions>
Stinkhorn<CellT, Dimensions>::Cursor::Cursor(Tree& tree) :
	m_position(0, 0, 0),
	m_direction(1, 0, 0),
	m_tree(tree)
{
	m_page_address = m_position >> PageT::bits;
	m_page = m_tree.find(m_page_address);
}

//TODO: move teleportation markers into this function and Tree::advance_cursor
template<class CellT, int Dimensions>
bool Stinkhorn<CellT, Dimensions>::Cursor::advance() {
	Vector pos(m_position + m_direction);

	//First, try just iterating through the current m_page, looking for instructions.
	//It's probably faster to do this, especially for #.
	if(m_page) {
		while(pos >> PageT::bits == m_page_address) {
			CellT c = m_page->get(pos & PageT::mask);
			if(c != ' ') {
				m_position = pos;
				return true;
			}

			pos += m_direction;
		}
	}

	//Try the next page before trying the (costly?) finding methods.
	//This results in a small performance gain.
	{
		Vector oldPos = pos;
		Vector nextPageAddress = pos >> PageT::bits;
		PageT* nextPage = m_tree.find(nextPageAddress);

		if(nextPage) {
			while(pos >> PageT::bits == nextPageAddress) {
				CellT c = nextPage->get(pos & PageT::mask);
				if(c != ' ') {
					//We must update all of these, because we have entered a new page.
					m_position = pos;
					m_page = nextPage;
					m_page_address = nextPageAddress;
					return true;
				}

				pos += m_direction;
			}
		}
		pos = oldPos;
	}

	bool success = 
		m_tree.advance_cursor(m_position, m_direction, pos, any_instruction);

	if(success)
		this->position(pos);

	return success;
}

template<class CellT, int Dimensions>
void Stinkhorn<CellT, Dimensions>::Cursor::position(Vector const& new_position) {
	Vector new_page_address = new_position >> PageT::bits;

	if(m_page_address != new_page_address) {
		m_page_address = new_page_address;
		m_page = m_tree.find(m_page_address);
	}

	m_position = new_position;
}

template<class CellT, int Dimensions>
bool Stinkhorn<CellT, Dimensions>::Cursor::jump() {
	Vector pos;
	bool can_advance_forwards = m_tree.advance_cursor(m_position, m_direction,
		pos, any_instruction, false);

	//We don't need the position it gave us. We only wanted to know if we 
	//*could* advance forwards, not where we would advance to.
	if(can_advance_forwards) {
		m_position += m_direction;
		return true;
	} 

	//This could be made faster, I mean, we've already search forwards
	bool can_advance_at_all = m_tree.advance_cursor(m_position, m_direction,
		pos, any_instruction, true);

	if(!can_advance_at_all)
		return false;

	//Advance to the next instruction, which will be skipped over.
	m_position = pos;
	return true;
}

template<class CellT, int Dimensions>
CellT Stinkhorn<CellT, Dimensions>::Cursor::get(Vector const& location) {   
	if((location >> PageT::bits) == m_page_address) {
		if(m_page) {
			return m_page->get(location & PageT::mask);
		}
	}

	return m_tree.get(location);
}

template<class CellT, int Dimensions>
void Stinkhorn<CellT, Dimensions>::Cursor::put(Vector const& location, CellT value) {
	if((location >> PageT::bits) == m_page_address) {
		if(m_page) {
			m_page->get(location & PageT::mask) = value;
			m_tree.update_minmax(location);
			return;
		}
	}

	m_tree.put(location, value);
}

template<class CellT, int Dimensions>
CellT Stinkhorn<CellT, Dimensions>::Cursor::currentCharacter() {
	if(m_page)
		return m_page->get(m_position & PageT::mask);
	else
		return m_tree.get(m_position);
}

template<class CellT, int Dimensions>
void Stinkhorn<CellT, Dimensions>::Cursor::teleport() {
	Vector new_position(m_position + m_direction);
	if(m_page) {
		while(new_position >> PageT::bits == m_page_address) {
			CellT c = m_page->get(new_position & PageT::mask);
			if(c == ';') {
				m_position = new_position;
				return;
			}

			new_position += m_direction;
		}
	}

	bool success = m_tree.advance_cursor(m_position, m_direction, new_position, teleport_instruction);
	if(success)
		m_position = new_position;
}

template<class CellT, int Dimensions>
stinkhorn::vector3<CellT> Stinkhorn<CellT, Dimensions>::Cursor::leftwards90Z() {
	Vector d = direction();
	return Vector(d.y, -d.x, 0);
}

template<class CellT, int Dimensions>
stinkhorn::vector3<CellT> Stinkhorn<CellT, Dimensions>::Cursor::rightwards90Z() {
	Vector d = direction();
	return Vector(-d.y, d.x, 0);
}

INSTANTIATE(class, Cursor);
