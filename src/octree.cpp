#include "octree.hpp"
#include <climits>
#include <vector>
#include <algorithm>

namespace stinkhorn {
	template<class T, int D>
	Stinkhorn<T, D>::Tree::Tree() {
		root_depth = 1; //TODO: Make higher in release mode?
		root = new NodeT();

		max_put = min_put = Vector();

#if OCTREE_PAGE_CACHE_SIZE > 0
		std::uninitialized_fill_n(&eden[0][0], EdenSize * EdenSize, (PageT*)0);
#endif
	}

	template<class T, int D>
	Stinkhorn<T, D>::Tree::~Tree() {
		delete root;
	}

	/**
	 * Calculate the log-base-2 of an integer in a fairly low amount of operations.
	 * Works up to 64-bit integers (and will complain if given anything else).
	 *
	 * Note that the some of uint64 literals are truncated to 0 if sizeof(v) < 8, but 
	 * for those ones v & b[i] fails and the bit in the output isn't set.
	 */
	template<class T, int D>
	T Stinkhorn<T, D>::Tree::log2(T v) {
		T v_ = v;
		typedef typename unsigned_of<T>::type uint_t;
		STATIC_ASSERT(sizeof(v) <= 8);
		STATIC_ASSERT(sizeof(v) == sizeof(uint_t));
		
#pragma warning( push )
#pragma warning( disable: 4305 ) //Truncation from 'unsigned __uint64' to 'const uint_t'
#pragma warning( disable: 4309 ) //Truncation of constant value
		const uint_t b[] = {
			B98_UINT64_LITERAL(0x2), B98_UINT64_LITERAL(0xC), 
			B98_UINT64_LITERAL(0xF0), B98_UINT64_LITERAL(0xFF00), 
			B98_UINT64_LITERAL(0xFFFF0000), B98_UINT64_LITERAL(0xFFFFFFFF00000000)
		};
#pragma warning( pop )
		const uint_t S[] = { 1ul, 2ul, 4ul, 8ul, 16ul, 32ul };

		register unsigned int r = 0; // result of log2(v) will go here
		for (int i = sizeof(S)/sizeof(S[0]) - 1; i >= 0; i--) // unroll for speed...
		{
			if (v & b[i])
			{
				v >>= S[i];
				r |= S[i];
			} 
		}

		//log2 rounds down! If v isn't a power of 2, then it's rounded down, so we
		//need to round up
		if( (v_ & (v_ - 1)) != 0)
			r++;

		return r;
	}

	template<class T, int D>
	void Stinkhorn<T, D>::Tree::update_minmax(Vector const& addr) {
		max_put.x = std::max(addr.x, max_put.x);
		max_put.y = std::max(addr.y, max_put.y);
		max_put.z = std::max(addr.z, max_put.z);
		
		min_put.x = std::min(addr.x, min_put.x);
		min_put.y = std::min(addr.y, min_put.y);
		min_put.z = std::min(addr.z, min_put.z);
	}
	
	template<class T, int D>
	void Stinkhorn<T, D>::Tree::get_minmax(Vector& min, Vector& max) {
		min = min_put;
		max = max_put;
	}

	template<class T, int D>
	typename Stinkhorn<T, D>::Tree::PageT* Stinkhorn<T, D>::Tree::find(Vector const& addr, bool create) {
#if OCTREE_PAGE_CACHE_SIZE > 0
		if(inEden(addr)) {
			PageT* p = eden[addr.y][addr.x];
			if(p || !create) {
				return p;
			}
			//If it's not found, we still need to create it, which currently still involves dealing with the tree structure.
		}
#endif
		//std::cerr << "find(" << addr << ", " << std::boolalpha << create << ");\n";

		//Find the current bounds of the tree
		assert(root_depth <= sizeof(T) * CHAR_BIT); //Otherwise, bad things will happen (malloc loop)
		T tree_max = T(1) << root_depth;
	    
		//If the tree isn't yet deep enough to contain this address, return 0 if we
		//don't need to create it or deepen the tree and create the node otherwise
		if(root_depth < sizeof(T) * CHAR_BIT - 1) {
			if(addr.x >= tree_max || addr.y >= tree_max || addr.z >= tree_max ||
				addr.x < -tree_max || addr.y < -tree_max || addr.z < -tree_max) {
				if(!create)
					return 0;

				expand_to(addr);
			}
		}

		tree_max = T(1) << root_depth;
		Vector lower(-tree_max, -tree_max, -tree_max),
		       upper(tree_max, tree_max, tree_max);
	    
		T depth = root_depth;
		NodeT *parent = 0, *n = root;
		Vector index;

#ifdef DEBUG
		std::vector<NodeT*> path;
		path.reserve(depth);
#endif

		while(true) {
#ifdef DEBUG
			path.push_back(n);
#endif
			index = choose_child(lower, upper, addr);

			parent = n;
			n = n->at(index);

			if(!n)
				break;
			else if(depth == 0) {
				assert(n && n->data);
				return n->data;
			}
			
			depth--;
		}

		assert(parent);
		assert(depth >= 0 || n);
		assert(!n || n->data);

		//If not found but we know where it should be, insert it.
		//Currently, parent is the deepest existing node on the correct path.
		if(!n) {
			if(!create)
				return 0;

			//idx_ is the index that was used to get to the current node
			//confusingly, idx is the index for the next node... I think
			assert(depth >= 0);
			while(depth >= 0) {
				NodeT*& child = parent->at(index); 
				assert(child == 0);

				child = new NodeT();
				//std::cerr << "  Creating node at " << addr << ": node = 0x" << child << ", parent = 0x" << parent << "\n";
				parent = child;
				
				index = choose_child(lower, upper, addr);
				depth--;
			}
			assert(depth == -1);
			n = parent;

			assert(n);
			n->data = new PageT();
			//std::cerr << "  Creating  0x" << n->data << " for " << addr << "\n";
			
#if OCTREE_PAGE_CACHE_SIZE > 0
			if(inEden(addr))
				eden[addr.y][addr.x] = n->data;
#endif
		}

		assert(n->data);
		assert(reinterpret_cast<uint64>(n->data) > 0x100);
		//std::cerr << "  Returning 0x" << n->data << " for " << addr << "\n";
		return n->data;
	}

	template<class T, int D>
	inline typename Stinkhorn<T, D>::Vector Stinkhorn<T, D>::Tree::choose_child(Vector& lower, Vector& upper, Vector const& addr)
	{
		assert(addr.x >= lower.x);
		assert(addr.y >= lower.y);
		assert(addr.z >= lower.z);
		assert(addr.x < upper.x);
		assert(addr.y < upper.y);
		assert(addr.z < upper.z);

		Vector idx(0, 0, 0);

		Vector middle = (lower + upper) / 2;

		if(addr.x >= middle.x) {
			lower.x = middle.x;
			idx.x = 1;
		} else {
			upper.x = middle.x;
		}

		if(addr.y >= middle.y) {
			lower.y = middle.y;
			idx.y = 1;
		} else {
			upper.y = middle.y;
		}

		//TODO: Only if D == 3?
		if(addr.y >= middle.y) {
			lower.y = middle.y;
			idx.y = 1;
		} else {
			upper.y = middle.y;
		}

		return idx;
	}

	namespace {
#ifdef DEBUG
		template<class T>
		void ensure_cubic(vector3<T> const& lower, vector3<T> const& upper) {
			vector3<T> size = upper - lower;
			assert(size.x == size.y && size.y == size.z);
		}
#else
		template<class T>
		void ensure_cubic(vector3<T> const& lower, vector3<T> const& upper) { }
#endif
	}

	template<class T, int D>
	inline void Stinkhorn<T, D>::Tree::choose_child(Vector const& lower, Vector const& upper, Vector const& index, Vector& new_lower, Vector& new_upper)
	{
		Vector middle = (lower + upper) / 2;

		new_lower.x = index.x ? middle.x : lower.x;
		new_lower.y = index.y ? middle.y : lower.y;
		new_lower.z = index.z ? middle.z : lower.z;

		new_upper.x = !index.x ? middle.x : upper.x;
		new_upper.y = !index.y ? middle.y : upper.y;
		new_upper.z = !index.z ? middle.z : upper.z;

		ensure_cubic(new_lower, new_upper);
	}

	template<class T, int D>
	void Stinkhorn<T, D>::Tree::increase_depth(T new_depth) {
		//std::cerr << "  increase_depth(" << new_depth << ")\n";
		assert(new_depth < max_bits && new_depth > 0);

		while(this->root_depth < new_depth) {
			//Go through each existing child of the root node, and insert a node
			//between it and the root.
			//Set the "inside" (closer to {0,0,0}) child of the new node to point 
			//to the previous child, which then won't have moved.
			
			for(T k = 0; k < D - 1; ++k) {
				for(T j = 0; j < 2; ++j) {
					for(T i = 0; i < 2; ++i) {
						//n is the current child of the root node, m is the node we
						//are shoving between n and root of course, if there is no
						//node here, we don't need to do anything.
						Vector v(i, j, k);
						NodeT* n = root->at(v);
						if(n) {
							NodeT* m = root->at(v) = new NodeT();
							//std::cerr << "    Creating node at " << v << ": node = 0x" << n << "\n";

							Vector opposite = Vector(1, 1, 1) - v;
							if(D == 2) {
								assert(v.z == 0);
								opposite.z = 0;
							}

							m->at(opposite) = n;
						}
					}
				}
			}

			root_depth++;
		}
	}

	//This is for expand_to.
	//If largest > 0, we want it so that largest < tree_max, not largest <= tree_max (because the range is
	//-2^n <= x < 2^n). In that case, we add 1 when getting the log2 so that the tree expands correctly.)
	namespace {
		template<class T>
		T abs1(T x) {
			if(x < 0)
				return -x;
			return x + 1;
		}
	}

	//Expands the tree so that it can contain the specified page address. If the specified address can't
	//currently be contained, new roots are added until that is the case.
	//We take the largest of the address components. Positive values have 1 added to them, see abs1 for
	//rationale.
	template<class T, int D>
	void Stinkhorn<T, D>::Tree::expand_to(Vector const& addr) {
		T largest = std::max<T>(std::max<T>(abs1(addr.x), abs1(addr.y)), abs1(addr.z) );
		T bits = log2(largest);
		increase_depth(bits);
	}

	//These are the "easy" versions of the functions which are expected to be used
	//when there is no cached page data. These will be much slower than Cursor::get
	//if the page address is not in the initial page cache (eden).
	template<class T, int D>
	T Stinkhorn<T, D>::Tree::get(Vector const& location) {
		Vector address = location >> PageT::bits;

		PageT* p = find(address);
		if(!p)
			return 32;

		return p->get(location & PageT::mask);
	}

	template<class T, int D>
	void Stinkhorn<T, D>::Tree::put(Vector const& location, T value) {
		update_minmax(location);

		Vector address = location >> PageT::bits;

		PageT* p = find(address, true);
		assert(p);

		p->get(location & PageT::mask) = value;
	}

	/**
	 * This function is used for two purposes. It handles the initial loading of the
	 * file into funge space, but it also handles the read calls from the i instruction.
	 *
	 * Still left to do:
	 * - test loading files with form feeds
	 * - add binary reading (no parsing of crlf etc)
	 * - try caching in 3 dimensions
	 *
	 * Let's assume the stream has been opened with the correct flags for now. It's
	 * probably trivial to verify this, and can be done at a later date.
	 */
	template<class T, int D>
	bool Stinkhorn<T, D>::Tree::read_file_into(Vector const& location, std::istream& stream, int flags, Vector& size)
	{
		bool binary = flags & FileFlags::binary;
		bool ff = ((flags & FileFlags::no_form_feeds) == 0) && !binary;
		bool lf = true && !binary;
	   
		size = Vector(0, 0, 0);
		Vector maxput(0, 0, 0);
	   
		if(true) { //will be used for different code path for binary files. Not yet though.
			PageT* current_page = 0;
			Vector page_base = location >> PageT::bits,
						page_offset(0, 0, 0);
			std::deque<PageT*> cache;

			//A distance in cells from the first cell of page_base
	        
			//XXX won't work with negative numbers... or WILL it? I think it does
			//actually
			Vector r0 = location & PageT::mask, r = r0; 
	        
			char c;
			PageT* p = 0;

			try {
				stream.exceptions(std::ios::failbit | std::ios::badbit 
								  | std::ios::eofbit);

				while(1) {
					c = stream.get();

					//The current page pointer was invalidated because we moved out
					//of the page. Try to find the page in cache before trying an
					//expensive lookup.
					if(!p) {
						T x = page_offset.x;
						if(x >= T(cache.size())) {
							p = this->find(page_base + page_offset, true);
							cache.push_back( p );
							//std::cerr << "Adding page 0x" << p << " to cache" <<
							//std::endl;
						} else {
							assert(x >= 0);
							p = cache[static_cast<std::size_t>(x)];
						}
					}
					assert(p);

					//HERE BE DRAGONS
					if(c == '\f' && ff) {
						//We're now going on to the next level in depth. Clear the cache
						//since it's easier (for now). Increase the page_offset's z, but
						//reset the x and y.

						//Update the size if necessary.
						if(r.x + 1 - r0.x + page_offset.x * PageT::size > size.x)
							size.x = r.x + 1 - r0.x + page_offset.x * PageT::size;
						if(r.y + 1 - r0.y + page_offset.y * PageT::size > size.y)
							size.x = r.y + 1 - r0.y + page_offset.y * PageT::size;

						r = Vector(r0.x, r0.y, r.z + 1);

						if(r.z == PageT::size) {
							page_offset.z++;
							r.z = 0;
						}

						page_offset.x = page_offset.y = 0;
						//TODO: We really only need to clear the cache if we've gone outside of the original 
						//XY plane of pages. And since pages are 8x8x8 by default, this won't happen every time.
						cache.clear();
						p = 0;
					}

					else if((c == '\r' || c == '\n') && lf) {
						//If it's CR, look for a following LF.
						if(c == '\r') {
							if(stream.peek() == '\n')
								stream.get();
						}

						//We're now going on to the next line.
						//Reset page_offset's x, increase its y, and leave the z alone.

						//Update the size if necessary.
						if(r.x + 1 - r0.x + page_offset.x * PageT::size > size.x)
							size.x = r.x + 1 - r0.x + page_offset.x * PageT::size;

						r = Vector(r0.x, r.y + 1, r.z);
						assert(r.x < PageT::size && r.y <= PageT::size && r.z < PageT::size);

						//This should happen only when we need to refresh the cache
						if(r.y == PageT::size) {
							page_offset.y++;
							r.y = 0;
							cache.clear();
						}

						p = 0;
						page_offset.x = 0;
					}

					else {
						assert(p);
						assert(r.x < PageT::size && r.y < PageT::size && r.z < PageT::size);
						assert((!lf || c != '\n') && (!ff || c != '\f'));
						if(c != ' ')
							p->get(r) = c;

						maxput.x = std::max<T>(maxput.x, r.x + page_offset.x * PageT::size);
						maxput.y = std::max<T>(maxput.y, r.y + page_offset.y * PageT::size);
						maxput.z = std::max<T>(maxput.z, r.z + page_offset.z * PageT::size);

						++r.x;
						if(r.x == PageT::size) {
							//std::cerr << "We've reached the end of a page!" <<
							//std::endl; std::cerr << "page_offset = " <<
							//page_offset << std::endl;
							p = 0;
							r.x = 0;
							page_offset.x++;
						}
					}
				}

			} catch(std::ios_base::failure const&) {
				if(!stream.eof())
					return false;
			}

			if(r.x - r0.x + 1 + page_offset.x * PageT::size > size.x)
				size.x = r.x + 1 - r0.x + page_offset.x * PageT::size;
			if(r.y - r0.y + 1 + page_offset.y * PageT::size > size.y)
				size.y = r.y + 1 - r0.y + page_offset.y * PageT::size;
			if(r.z - r0.z + 1 + page_offset.z * PageT::size > size.z)
				size.z = r.z + 1 - r0.z + page_offset.z * PageT::size;
		}
	    
		if(size.y == 0)
			size.y = 1;
		if(size.z == 0)
			size.z = 1;

		update_minmax(location);
		update_minmax(maxput);
		//update_minmax(location + size - Vector(1,1,1));

		return true;
	}

	namespace {
		template<class PageT>
		void debug_page_contents(PageT* p) {
			for(int y = 0; y < PageT::size; y++) {
				for(int x = 0; x < PageT::size; x++) {
					int c = p->get(vector3<int>(x,y,0));
					if(c <= 32) c = '.';
					if(c > 255 || c < 0) c = '.';
					std::cerr.put(char(c));
				}
				std::cerr << std::endl;
			}
		}
	}

	template<class T, int D>
	bool Stinkhorn<T, D>::Tree::write_file_from(Vector const& from, Vector const& original_to, std::ostream& stream, int flags)
	{
		bool linear = flags & 0x1;

		Vector r, r0;
		r0 = from & PageT::mask;
		r = r0;
	    
		Vector to = original_to;
		if(to.z == from.z)
			to.z++;
		
		bool is2d = to.z - from.z <= 1;
	    
		std::string spaces;
		std::deque<PageT*> pages;

		Vector page_offset(0, 0, 0), page_base = from >> PageT::bits, max_offset = (to - from) >> PageT::bits;
		PageT* p = find(page_base);
		pages.push_back(p);
	    
		try {

			while(1) {
				if((page_offset.x > max_offset.x) || (page_offset.x >= max_offset.x && (r.x >= (to.x & PageT::mask)))) 
				{
					r.x = r0.x;
					r.y++;
					page_offset.x = 0;
					p = 0;
	                
					//In linear mode, spaces before EOL should not be written out.
					//Instead, just remove all spaces, leaving \n and \f characters intact.
					if(linear) {
						std::string::iterator itr = std::remove(spaces.begin(), spaces.end(), ' ');
						spaces.erase(itr, spaces.end());
					}

					spaces += '\n';
				}
	            
				if(r.y == PageT::size) {
					r.y = 0;
					page_offset.y++;
					p = 0;
					pages.clear();
				}
	            
				if(page_offset.y >= max_offset.y && (r.y >= (to.y & PageT::mask))) {
					r.y = r0.y;
					r.z++;
					r.x = r0.x;
					p = 0;
					page_offset.x = 0;
					page_offset.y = 0;
	                
					//Should we be doing the same thing as with EOL?
					if(!is2d)
						spaces += "\f";
				}
	            
				if(r.z == PageT::size) {
					page_offset.z++;
					r.z = 0;
					p = 0;
					pages.clear();
				}
	            
				if(page_offset.z >= max_offset.z && (r.z >= (to.z & PageT::mask))) {
					break;
				}
	            
				if(p == 0) {
					if(static_cast<std::size_t>(page_offset.x) < pages.size()) {
						p = pages[static_cast<std::size_t>(page_offset.x)];
					} else {
						p = find(page_base + page_offset);
						//The same page shouldn't show up in the cache twice.
						assert(!p || std::find(pages.begin(), pages.end(), p) == pages.end());
						pages.push_back(p);
					}
	                
					if(p == 0) {
						page_offset.x++;
						r.x = 0;
					}
				}
	            
				if(p) {
					char c = char(p->get(r));
					if(c == ' ' && linear) {
						spaces += c;
					} else {
						stream << spaces;
						spaces.clear();
						stream.put(c);
						stream.flush();//temp
					}
				} else {
					T spc = PageT::size;
					if(page_offset == max_offset)
						spc = std::min<T>(spc, to.x & PageT::mask);

					assert(spc <= PageT::size);
					spaces += std::string(static_cast<std::size_t>(spc), ' ');
				}
	            
				r.x++;
				if(r.x == PageT::size) {
					page_offset.x++;
					p = 0;
					r.x = 0;
				}
	            
			}
	    
		} catch (std::ios_base::failure&) {
			return false;
		}
	    
		if(!linear) {
			stream << spaces;
		}
	    
		return true;
	}

	///Helper functions for Tree::find_line_end
	namespace {
		/**
		 * Finds the intersection between a line (specified by a point and a
		 * direction) and a cube (specified a "top left" point (also lower Z value)
		 * and a "bottom right" point (higher Z).
		 * 
		 * returns @true if the ray intersects the cube. The location of the
		 * intersection is stored in @location, and @distance will contain the
		 * number of steps from the point on the ray to the point of intersection.
		 *
		 * Note that this function only cares about cubes that contain a point on
		 * the line with an integral parameter. That is, at least one of the "steps"
		 * on the line must lie within the cube, otherwise the function will return
		 * @false.
		 *
		 * FT is a template parameter so that we don't need to know the type of the
		 * Tree.
		 */
		template<class T>
		bool intersection(FindTypes find_type, int dimensions, vector3<T> const& ray_point, 
			vector3<T> const& ray_direction, vector3<T> const& cube_lower, vector3<T> const& cube_upper, 
			vector3<T>& location, T& distance)
		{
			ensure_cubic(cube_lower, cube_upper);

			//If the point is inside the cube, and we are finding the nearest bit,
			//advance the point along the ray and return that, if it's still inside
			//the cube.
			if(find_type == nearest) {
				vector3<T> p(ray_point + ray_direction);
				if(p.x >= cube_lower.x && p.y >= cube_lower.y && p.z >= cube_lower.z
				   && p.x < cube_upper.x && p.y < cube_upper.y && p.z < cube_upper.z)
				{
					distance = 1;
					location = p;
					return true;
				}
			}

			//Number of steps along the ray needed to get to the point of
			//intersection
			T current_steps = 0; 

			//whether or not an intersection has been found yet
			bool found = false; 

			//For each face, we only check for intersections if:
			// - It is definitely the furthest face from the start if |find_type ==
			//   furthest|, or the nearest from ray_point if |find_type == nearest|.
			// - It isn't the nearest/furthest (delete where appropriate), but the
			//   ray_point is inside the cube
			struct face_checker { 
				enum face_side {
					lower_face = 0,
					upper_face = 1
				};

				T& current_steps;
				bool& found;
				FindTypes find_type;
				vector3<T> const& cube_lower,
								& cube_upper,
								& ray_point,
								& ray_direction;

				void check_face(face_side face, T lower, T upper, 
								T point, T direction)
				{ 
					if(direction == 0)
						return;

					T steps;
					if(face == lower_face)
						steps = (lower - point) / direction;
					else
						steps = (upper - point - 1) / direction;
	                
					//Make sure it really is intersecting, and not some other cube
					//off to the side of the ray
					vector3<T> ep = ray_point + steps * ray_direction;
					if(ep.x <  cube_lower.x || ep.y <  cube_lower.y || ep.z <  cube_lower.z ||
					   ep.x >= cube_upper.x || ep.y >= cube_upper.y || ep.z >= cube_upper.z)
					{
						return;
					}
	                
					if(steps > 0) {
						if((steps < current_steps || current_steps == 0) && find_type == nearest)
							current_steps = steps;
						if((steps > current_steps) 
						   && find_type == furthest)
						{
							current_steps = steps;
						}
	                    
						found = true;
					}
				}

				face_checker(FindTypes find_type, T& current_steps, bool& found,
							 vector3<T> const& cube_lower, vector3<T> const& cube_upper,
							 vector3<T> const& ray_point, vector3<T> const& ray_direction):
					find_type(find_type), current_steps(current_steps), found(found), 
					cube_lower(cube_lower), cube_upper(cube_upper),
					ray_point(ray_point), ray_direction(ray_direction)
					{}
			};

			face_checker fc(find_type, current_steps, found, cube_lower,
							cube_upper, ray_point, ray_direction);

			fc.check_face(face_checker::lower_face, cube_lower.x, 
						  cube_upper.x, ray_point.x, ray_direction.x); 
			fc.check_face(face_checker::upper_face, cube_lower.x,
						  cube_upper.x, ray_point.x, ray_direction.x);
			fc.check_face(face_checker::lower_face, cube_lower.y, 
						  cube_upper.y, ray_point.y, ray_direction.y);
			fc.check_face(face_checker::upper_face, cube_lower.y, 
						  cube_upper.y, ray_point.y, ray_direction.y);

			//TODO: Verify that this check does not cause regressions.
			//As far as I ca see, it's impossible to hit these faces in 2D mode, ever.
			//Perhaps a check on the ray's direction would be appropriate.
			if(dimensions == 3) {
				fc.check_face(face_checker::lower_face, cube_lower.z, 
							  cube_upper.z, ray_point.z, ray_direction.z);
				fc.check_face(face_checker::upper_face, cube_lower.z, 
							  cube_upper.z, ray_point.z, ray_direction.z);
			}
	        
			if(found) {
				location = ray_point + ray_direction * current_steps;
				distance = current_steps;
			}

			return found;
		}

	}

	namespace {
		//A simple structure to throw around in standard containers. It doesn't
		//"own" the node it references, so the default constructors will do.
		template<class T, int D>
		struct cubedef {
			typename Stinkhorn<T, D>::Tree::NodeT* node;
			vector3<T> lower, upper, p;

			cubedef(): node(0) {}

			cubedef(typename Stinkhorn<T, D>::Tree::NodeT* node, vector3<T> const& lower, vector3<T> const& upper)
				: node(node), lower(lower), upper(upper) 
			{}

			cubedef(cubedef const& other) 
				: node(other.node), lower(other.lower), upper(other.upper), p(other.p)
			{}
		};
	}

	/**
	 * Look in a leaf for an instruction (any non-space character). We backtrack
	 * along the line given, trying to find instructions. Return not_found if none
	 * found, otherwise return found.
	 *
	 * @param point 
	 *   The furthest point on the line that is in the node's cube.
	 * @param direction 
	 *   The direction that the IP goes in
	 * @param upper 
	 *   Upper bounds of the cube
	 * @param lower
	 *   Lower bounds of the cube
	 * @param n 
	 *   The node
	 * @param @out result 
	 *   The result of the test. Undefined if the function returns false.
	 */
	template<class T, int D>
	int Stinkhorn<T, D>::Tree::find_leaf_instruction_on_line(FindTypes find_type, SearchFor searching_for,
		Vector const& point, Vector const& direction, NodeT* n, 
		Vector const& lower, Vector const& upper, Vector& result)
	{
		//std::cerr << "Looking for " << (find_type==furthest?"furthest":"nearest") 
		//          << " instruction in leaf at " << lower << " to " << upper 
		//          << std::endl;

		Vector r = point;

		bool found = false;
		while(r.x >= lower.x && r.y >= lower.y && r.z >= lower.z && 
			r.x < upper.x && r.y < upper.y && r.z < upper.z)
		{
			//Convert from the position in space to an index into the vector, and
			//make absolutely sure that it's valid index. (Shouldn't this be done in
			//the getter anyway?)
			Vector index = r - lower;
			assert(n->data);
			assert(index.x < PageT::size && index.y < PageT::size 
				  && index.z < PageT::size);
	        
			bool found;
			char c = static_cast<char>(n->data->get(index));
			if(searching_for == any_instruction) //for wrapping
				found = c != ' ';
			else if(searching_for == non_marker) //for k
				found = c != ';' && c != ' ';
			else if(searching_for == teleport_instruction) //for ;
				found = c == ';';
			else {
				assert(0 && "searching_for what?");
			}
	        
			if(found) {
				//We're going backwards along the line, so the furthest points will
				//in fact be reached first
				if(find_type == furthest) {
					result = r;
					return instruction_search_results::found;
				} else {
					result = r;
					found = true;
					return instruction_search_results::found;
				}
			}

			if(find_type == furthest)
				r -= direction;
			else
				r += direction;
		}

		if(found)
			return instruction_search_results::found;

		return instruction_search_results::not_found;
	}

	/**
	 * Basic algorithm here is to order the non-null child nodes by distance from
	 * the point (returned by @intersection) and go through them (recursing),
	 * furthest to closest. When we find an instruction, return @found.
	 *
	 * Generalise this to find the nearest instruction as well.
	 */
	template<class T, int D>
	int Stinkhorn<T, D>::Tree::find_node_instruction_on_line(FindTypes find_type, SearchFor searching_for,
		Vector const& point, Vector const& direction, NodeT* n,
		Vector const& lower, Vector const& upper, Vector& result)
	{
		//std::cerr << "Looking for " << (find_type==furthest?"furthest":"nearest")
		//          << " instruction in node at " << lower << " to " << upper
		//          << std::endl;

		//As far as I am aware, there is no way that 2 cubes could result in the
		//same "distance" parameter, so this should do.
		//TODO: Replace with something more lightweight.
		typedef cubedef<T,D> CubeT;
		std::map<T, CubeT> kids;

		//Find the non-null children of n
		//TODO: Only check one of the two Z-halves in 2D.
		for(int k = 0; k < 2; ++k) {
			for(int i = 0; i < 2; ++i) {
				for(int j = 0; j < 2; ++j) {
					Vector idx(i, j, k);
					Vector idx_fixed(idx); //We don't want the Z-less index to actually be used for choose_child
					if(D == 2) //HACK: It should work, but is kind of inefficient since we're checking twice as many cubes.
						idx_fixed.z = 0;

					NodeT* child = n->at(idx_fixed);
					if(!child)
						continue;

					CubeT cube(child, Vector(), Vector());
					choose_child(lower, upper, idx, /*out*/ cube.lower, /*out*/ cube.upper);

					T distance;
					if(!intersection(find_type, D, point, direction, cube.lower, cube.upper, /*out*/ cube.p, /*out*/ distance))
						continue;

					if(distance > 0) {
						//see notes on the map
						assert(kids.find(distance) == kids.end());
						kids[distance] = cube;
					}
				}
			}
		}

		//Can probably fix this tragedy with some sort of generalised iterator-taking function.
		//Or by switching to a vector and sort()ing it.
		if(find_type == furthest) {
			//Loop *backwards* through the list. We want the furthest ones. You
			//haven't been listening at all, have you?
			for(typename std::map<T, CubeT>::const_reverse_iterator i = kids.rbegin();
				i != kids.rend(); ++i)
			{
				CubeT const& cube = i->second;
				if(cube.node->data) {
					if(this->find_leaf_instruction_on_line(find_type, searching_for, cube.p,
						direction, cube.node, cube.lower, cube.upper, result) == instruction_search_results::found)
					{
						return instruction_search_results::found;
					}
				} else {
					if(this->find_node_instruction_on_line(find_type, searching_for, point,
						direction, cube.node, cube.lower, cube.upper, result) == instruction_search_results::found)
					{
						return instruction_search_results::found;
					}
				}
			}
		} else {
			for(typename std::map<T, CubeT>::const_iterator i = kids.begin();
				i != kids.end(); ++i)
			{
				CubeT const& cube = i->second;
				if(cube.node->data) {
					if(this->find_leaf_instruction_on_line(find_type, searching_for, cube.p, 
						direction, cube.node, cube.lower, cube.upper, result) == instruction_search_results::found)
					{
						return instruction_search_results::found;
					}
				} else {
					if(this->find_node_instruction_on_line(find_type, searching_for, point, 
						direction, cube.node, cube.lower, cube.upper, result) == instruction_search_results::found)
					{
						return instruction_search_results::found;
					}
				}
			}
		}

		return instruction_search_results::not_found;
	}

	/**
	 * The purpose of this function is for line wrapping in Funge-98's Lahey-space
	 * model. The first two vector3 parameters, @point and @direction, form a line
	 * in 3D space. The function determines the first non-space character in the
	 * funge-space which lies on a step on the line.
	 *
	 * These vectors refer to cells, not pages.
	 */
	template<class T, int D>
	bool Stinkhorn<T, D>::Tree::find_instruction_on_line(FindTypes find_type, SearchFor searching_for,
		Vector const& point, Vector const& direction, Vector& result)
	{
		//If the dimension is 2, we can't have 3D vectors. That would just be silly.
		assert( !(D == 2 && direction.z != 0) );
	    
		T half_width = 1 << root_depth;
		half_width <<= PageT::bits;

		Vector lower(-half_width, -half_width, -half_width),
		        upper(half_width, half_width, half_width);
	    
		int found = find_node_instruction_on_line(find_type, searching_for, point, direction, root, lower, upper, result);
		return found == instruction_search_results::found;
	}

	/**
	 * Advance the instruction pointer along the line with the point
	 * @current_position and the direction @current_direction, returning the next
	 * non-blank instruction. If none are found, reverse the line's direction and
	 * start looking for the furthest non-blank instruction.
	 *
	 * Return @false if no instructions are found, @true if an instruction is found.
	 * When the function returns false, the value of @new_position is undefined.
	 */
	template<class T, int D>
	bool Stinkhorn<T, D>::Tree::advance_cursor(Vector const& current_position, Vector const& current_direction,
		Vector& new_position, SearchFor searching_for, bool allow_backward)
	{
		//if the IP is stuck, we're probably on an instruction (unless another thread has
		//been modifying the code). If we're on an instruction, we can just stay on that 
		//instruction for as long as we like.
		//If we're not on an instruction, then there is clearly no path to an
		//instruction, so we go into an infinite loop. This is handled by the
		//calling function.
		if(current_direction == Vector(0, 0, 0)) {
			if(this->get(current_position) != ' ')
				return true;
			return false;
		}

		bool forward = find_instruction_on_line(nearest, searching_for, current_position,
												current_direction, new_position); 

		if(forward) {
			//new_position is already populated with the correct value
			return true;
		} else {
			if(allow_backward) {
				bool backward = 
					find_instruction_on_line(furthest, searching_for, current_position, 
											 -current_direction, new_position);
				if(backward) {
					return true;
				} else {
					//We didn't find an instruction forwards, and we didn't find one
					//looking backwards.
					//But we may or may not be standing on one. Let's look.
					if(this->get(current_position) != ' ') {
						new_position = current_position;
						return true;
					}

					//Not found! Infinite loop time! Execution of said infinite loop is
					//left to the caller.
					return false;
				}
			} else {
				return false;
			}
		}
	}
}

INSTANTIATE(class, Tree);
