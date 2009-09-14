#ifndef B98_OCTREE_HPP_INCLUDED
#define B98_OCTREE_HPP_INCLUDED

#include "stinkhorn.hpp"
#include "vector.hpp"

#include <cmath>
#include <cassert>
#include <string>
#include <iostream>
#include <iomanip>
#include <deque>
#include <map>

#ifdef max
#undef max
#endif

#define OCTREE_DEBUG(x) //std::cerr << x << std::endl;

#ifndef OCTREE_PAGE_CACHE_SIZE
#ifdef DEBUG
#define OCTREE_PAGE_CACHE_SIZE 0 //We want to catch broken tree functions
#else
#define OCTREE_PAGE_CACHE_SIZE 32 //256 doesn't actually seem to slow it down much
#endif
#endif

namespace stinkhorn {
	enum FindTypes {
		furthest = 14,
		nearest = 25
	};
    
	enum SearchFor {
		any_instruction = 18,
		non_marker = 113,
		teleport_instruction = 87
	};

	template<class T, int Dimensions>
	struct Stinkhorn<T, Dimensions>::TreePage {
		static const T bits = Dimensions == 2 ? 6 : 3,
			size = 1 << bits,
			mask = size - 1;

		friend class unit_test;

	protected:
		static const T page_area = size * size * (Dimensions==2 ? 1 : size);
	public:

		T data[page_area];

		/**
		 * We use operator new() in this form because the more widely used form
		 * of new[] constructs every element in the array, which would be
		 * redundant, since we need to initialise the array with 32 (' ')
		 */
		TreePage() {
			std::uninitialized_fill(data, data + page_area, T(' '));
		}

		~TreePage() {
		}

		T& get(Vector const& index) {
			assert(Dimensions == 3 || index.z == 0);
			return data[index.x + size * (index.y + size * index.z)];
		}

		T get(Vector const& index) const {
			assert(Dimensions == 3 || index.z == 0);
			return data[index.x + size * (index.y + size * index.z)];
		}

	private:
		//Purposely not instantiated, because these could lead to bad things.
		//Now we can't call these accidentally without getting compile/link 
		//errors.
		TreePage(TreePage const&);
		TreePage& operator =(TreePage const&);
	};

	template<class CellT, int Dimensions>
	struct TreeNodeBase {
		typename Stinkhorn<CellT, Dimensions>::TreePage* getPage() {
			return data;
		}

		TreeNodeBase() : data(0) {}
		~TreeNodeBase() { delete data; }

	protected:
		typename Stinkhorn<CellT, Dimensions>::TreePage* data;
		friend class Tree;
	};

	template<class CellT>
	class TreeNode<CellT, 3> : public TreeNodeBase<CellT, 3> {
		friend class Stinkhorn<CellT, 3>::Tree;

		TreeNode* children[2][2][2];

		TreeNode() {
			children[0][0][0] =
			children[1][0][0] =
			children[0][1][0] =
			children[1][1][0] =
			children[0][0][1] =
			children[1][0][1] =
			children[0][1][1] =
			children[1][1][1] = 0;
		}

		~TreeNode() {
			delete children[0][0][0];
			delete children[1][0][0];
			delete children[0][1][0];
			delete children[1][1][0];
			delete children[0][0][1];
			delete children[1][0][1];
			delete children[0][1][1];
			delete children[1][1][1];
		}

		TreeNode<CellT, 3>*& at(vector3<CellT> const& index) {
			return children[index.x][index.y][index.z];
		}
	};

	template<class CellT>
	class TreeNode<CellT, 2> : public TreeNodeBase<CellT, 2> {
		friend class Stinkhorn<CellT, 2>::Tree;

		TreeNode<CellT, 2>* children[2][2];

		TreeNode() {
			children[0][0] =
			children[1][0] =
			children[0][1] =
			children[1][1] = 0;
		}

		~TreeNode() {
			delete children[0][0];
			delete children[1][0];
			delete children[0][1];
			delete children[1][1];
		}

		TreeNode<CellT, 2>*& at(vector3<CellT> const& index) {
			assert(index.z == 0);
			return children[index.x][index.y];
		}
	};

	/**
	 * The Dimensions parameter is, more than anything, an optimisation. Different
	 * page sizes are needed for different dimensionalities - 2D trees perform best
	 * with a page size of 64x64 (16KB per page in 32-bit environments), whereas 
	 * using 64x64x64 in 3D would more than likely exhaust the machine's memory 
	 * pretty quickly, if the program were to write values at sparse locations.
	 *
	 * Thus, in 3D, we use 8x8x8, for a size in memory of 2 kilobytes per page, and
	 * some additional memory for bookkeeping purposes.
	 */
	template<class T, int Dimensions>
	class Stinkhorn<T, Dimensions>::Tree {
	public:
		Tree();
		~Tree();

	public:
		typedef TreeNode<T, Dimensions> NodeT;
		typedef TreePage PageT;

		struct FileFlags {
			static const int
				binary = 1,
				no_form_feeds = 2; ///<form feeds only do anything in trefunge
		};

#if OCTREE_PAGE_CACHE_SIZE > 0
		static const int EdenSize = OCTREE_PAGE_CACHE_SIZE;
#endif

	public:

		///Easy APIs
		T get(Vector const& location);
		void put(Vector const& location, T value);

		///Goes off positions or what?
		struct instruction_search_results {
			static const int 
				found = 1,
				not_found = 2;
		};

		int find_leaf_instruction_on_line(FindTypes find_type, SearchFor s, Vector const& point, Vector const& direction, NodeT* n, Vector const& lower, Vector const& upper, Vector& result);
		int find_node_instruction_on_line(FindTypes find_type, SearchFor s, Vector const& point, Vector const& direction, NodeT* n, Vector const& lower, Vector const& upper, Vector& result);
		bool find_instruction_on_line(FindTypes find_type, SearchFor s, Vector const& point, Vector const& direction, Vector& result);
		bool read_file_into(Vector const& location, std::istream& stream, int flags, Vector& size);
		bool write_file_from(Vector const& from, Vector const& to, std::ostream& stream, int flags);
		bool advance_cursor(Vector const& current_position, Vector const& current_direction, Vector& new_position, SearchFor s, bool allow_backward = true);
		//void move_row(T y, T dx);
		//void move_column(T x, T dy);

	public:
		PageT* find(Vector const& addr, bool create = false);

		void update_minmax(Vector const& put);
		void get_minmax(Vector& min, Vector& max);

		static T log2(T v);
	    
	protected:
		void increase_depth(T new_depth);
		void expand_to(Vector const& address);
	    
		Vector choose_child(Vector& lower, Vector& upper, Vector const& target);
		void choose_child(Vector const& lower, Vector const& upper, Vector const& index, Vector& new_lower, Vector& new_upper);

	public:
		static const T max_bits = sizeof(T) * 8 - PageT::bits;

	private:
#if OCTREE_PAGE_CACHE_SIZE > 0
		PageT* eden[EdenSize][EdenSize];
		bool inEden(Vector addr) { return addr.z == 0 && addr.x >= 0 && addr.x < EdenSize && addr.y >= 0 && addr.y < EdenSize; }
#endif

		T root_depth;
		NodeT* root;
		friend class unit_test;

		//Upper and lower bounds that have actually been assigned a value
		Vector min_put, max_put;
	};
}

#endif
