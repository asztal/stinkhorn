#ifndef B98_STINKHORN_HPP_INCLUDED
#define B98_STINKHORN_HPP_INCLUDED

#include "config.hpp"
#include <string>
#include <ostream>

namespace stinkhorn {
	template<class CellT>
	class StackStack;

	template<class CellT>
	struct vector3;

	typedef uint64 IdT;

	//These need to specialize on Dimensions.
	template<class CellT, int Dimensions>
	class TreeNode;
	template<class CellT, int Dimensions>
	struct TreeNodeBase;

	//Overloading operator<< causes all sorts of compiler troubles, and generally isn't worth the effort.
	template<class CellT>
	inline void writeString(std::ostream& stream, std::basic_string<CellT> const& str) {
		for(std::basic_string<int16>::size_type i = 0; i < str.length(); ++i)
			stream.put(str[i]);
	}

	template<class CellT, int Dimensions>
	struct Stinkhorn {
		typedef vector3<CellT> Vector;
		typedef StackStack<CellT> StackStackT;
		typedef typename unsigned_of<CellT>::type UCell;

		typedef std::basic_string<CellT> String;

		struct TreePage;
		class Tree;

		class Context;
		class Cursor;
		class DebugInterpreter;
		struct IFingerprint;
		struct IFingerprintState;
		struct IFingerprintSource;
		class FingerprintStack;
		class FingerprintRegistry;
		class DefaultFingerprintSource;
		struct Befunge93Fingerprint;
		struct Befunge98Fingerprint;
		struct TrefungeFingerprint;
		class Interpreter;
		class Thread;

		struct NullFingerprint;
		struct RomaFingerprint;
		struct TimerFingerprint;
		struct ModuFingerprint;
		struct OrthFingerprint;
		struct BoolFingerprint;
		struct RefcFingerprint;
		struct ToysFingerprint;
		struct SockFingerprint;
		struct StrnFingerprint;
	};
}

#endif
