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

// When in hyperspace, the function looks for a semicolon to drop out of hyperspace.
// When not in hyperspace, the function looks for any non-space cell.
template<class CellT, int Dimensions>
bool Stinkhorn<CellT, Dimensions>::Cursor::advance_fast(const Vector& from, Vector& to, bool in_hyperspace, bool can_wrap) {
	Vector pos = from + m_direction;
	PageT* page;
	Vector page_address = pos >> PageT::bits;

	if(page_address == m_page_address) {
		getPage(); 
		page = m_page;
	} else {
		page = m_tree.find(page_address);
	}

	while(page) {
		while(pos >> PageT::bits == page_address) {
			CellT c = page->get(pos & PageT::mask);
			if (in_hyperspace) {
				if (c == ';') {
					to = pos;
					return true;
				}
			} else {
				if (c != ' ') {
					to = pos;
					return true;
				}
			}
			pos += m_direction;
		}
		
		page_address = pos >> PageT::bits;
		page = m_tree.find(page_address);
	}

	// We've hit a gap in funge-space!
	return m_tree.advance_cursor(from, m_direction, to, in_hyperspace ? teleport_instruction : any_instruction, can_wrap);
}

template<class CellT, int Dimensions>
bool Stinkhorn<CellT, Dimensions>::Cursor::advance(bool follow_teleports, bool can_wrap) {
	Vector pos = m_position;
	for(;;) {
		bool s = advance_fast(pos, pos, false, can_wrap);
		if (!s)
			return false;
		
		position(pos);
		CellT c = get(pos);

		if (c != ' ' && c != ';')
			return true;

		if (c == ';') {
			if (!follow_teleports)
				return true;

			Vector from = pos;
			s = advance_fast(pos, pos, true, can_wrap);
			if (!s)
				pos = from;
		}
	}
	return false;
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
CellT Stinkhorn<CellT, Dimensions>::Cursor::get(Vector const& location) {   
	if((location >> PageT::bits) == m_page_address) {
		if(m_page) {
			return m_page->get(location & PageT::mask);
		}
	}

	return m_tree.get(location);
}

template<class CellT, int Dimensions>
void Stinkhorn<CellT, Dimensions>::Cursor::put(CellT value) {
	put(m_position, value);
}

template<class CellT, int Dimensions>
void Stinkhorn<CellT, Dimensions>::Cursor::put(Vector const& location, CellT value) {
	if((location >> PageT::bits) == m_page_address) {
		getPage();

		if(m_page) {
			m_page->get(location & PageT::mask) = value;
			m_tree.update_minmax(location);
			return;
		} else if (value == ' ') {
			return;
		}
	}

	m_tree.put(location, value);
}

template<class CellT, int Dimensions>
CellT Stinkhorn<CellT, Dimensions>::Cursor::currentCharacter() {
	getPage();
	if(m_page)
		return m_page->get(m_position & PageT::mask);
	else
		return ' ';
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

template<class CellT, int Dimensions>
void Stinkhorn<CellT, Dimensions>::Cursor::getPage() {
	if(!m_page)
		m_page = m_tree.find(m_page_address);
}

INSTANTIATE(class, Cursor);
