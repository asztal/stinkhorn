#ifndef B98_CONFIG_HPP_INCLUDED
#define B98_CONFIG_HPP_INCLUDED

#ifndef __cplusplus
#error "Please compile this as C++"
#endif

#ifdef _MSC_VER
typedef signed char int8;
typedef short int int16;
typedef int int32;
typedef __int64 int64;

typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned int uint32;
typedef unsigned __int64 uint64;

#define B98_UINT64_LITERAL(x) x##ui64
#else
typedef signed char int8;
typedef short int int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
#define B98_UINT64_LITERAL(x) x##ull
#endif

#define B98_STRINGIFY_(x) #x
#define B98_STRINGIFY(x) B98_STRINGIFY_(x)

#if defined(WIN64) || defined(_WIN64)
#	define B98_PLATFORM "win64"
#	define B98_WINDOWS
#elif defined(WIN32) || defined(_WIN32)
#	ifndef WIN32
#		define WIN32
#	endif
#	define B98_PLATFORM "win32"
#	define B98_WINDOWS
#else
#endif

#if defined(_DEBUG) && !defined(DEBUG)
#	define DEBUG
#endif

#if defined(_M_IX86) || (defined(__SIZEOF_POINTER__) && __SIZEOF_POINTER__ == 4)
#	define B98_ARCH "x86"
#elif defined(_M_X64) || (defined(__SIZEOF_POINTER__) && __SIZEOF_POINTER__ == 8)
#	define B98_ARCH "x64"
#endif

#if defined(_MSC_VER)
#	if _MSC_VER >= 1500
#		define B98_COMPILER "msvc-9.0"
#	elif _MSC_VER >= 1400
#		define B98_COMPILER "msvc-8.0"
#	elif _MSC_VER >= 1310
#		define B98_COMPILER "msvc-7.1"
#	elif _MSC_VER >= 1300
#		define B98_COMPILER "msvc-7.0"
#	elif _MSC_VER >= 1200
#		define B98_COMPILER "msvc-6.0"
#		pragma message("Compiling with Visual C++ 6.0 is not recommended.")
#	elif _MSC_VER >= 1200
#		define B98_COMPILER "msvc-5.0"
#		pragma message("Compiling with Visual C++ 5.0... you need help.")
#	else
#		define B98_COMPILER "msvc"
#	endif
#	define B98_MSVC
#elif defined(__GNUC__)
#	if __GNUC__ >= 3
#		define B98_COMPILER "gcc-" B98_STRINGIFY(__GNUC__) "." B98_STRINGIFY(__GNUC_MINOR__) "." B98_STRINGIFY(__GNUC_PATCHLEVEL__)
#	elif __GNUC__ >= 2
#		define B98_COMPILER "gcc-" B98_STRINGIFY(__GNUC__) "." B98_STRINGIFY(__GNUC_MINOR__)
#	else
#		define B98_COMPILER "gcc"
#	endif
#	define B98_GCC
#endif

template<class T>
struct unsigned_of;

template<> struct unsigned_of<int8> { typedef uint8 type; };
template<> struct unsigned_of<int16> { typedef uint16 type; };
template<> struct unsigned_of<int32> { typedef uint32 type; };
template<> struct unsigned_of<int64> { typedef uint64 type; };

namespace stinkhorn {
	namespace detail { 
		namespace {
			template<bool False>
			struct static_assert;

			template<>
			struct static_assert<true> {
			};
		} 
	}
}

#define B98_JOIN_(x,y) x##y
#define B98_JOIN(x,y) B98_JOIN_(x,y)
#define STATIC_ASSERT(expr) typedef ::stinkhorn::detail::static_assert<sizeof(::stinkhorn::detail::static_assert<bool(expr)>)> B98_JOIN(_static_assert_for_line_,__LINE__)

//Various options available (oh, the choice):

//Define this to not use QueryPerformanceCounter for the HRTI fingerprint on Windows.
//Why you'd want to give up microsecond-accurate timing is beyond me, unless
//you're using windows 3.1 or it breaks your program...
//#define B98_WIN32_DONT_USE_QPC

//Define this to not use boost::pool_allocator for the stack stack.
//#define B98_NO_STACK_POOL

#ifdef _MSC_VER
inline int64 abs(int64 a) {
	return a >= int64() ? a : -a;
}
#endif

#define B98_MAJOR_VERSION 0
#define B98_MINOR_VERSION 0
#define B98_REVISION 54

#ifdef B98_NO_TREFUNGE
# ifdef B98_NO_64BIT_CELLS
#  define INSTANTIATE(aggregateType, name) \
	template aggregateType stinkhorn::Stinkhorn<int32, 2>::name;
# else
#  define INSTANTIATE(aggregateType, name) \
	template aggregateType stinkhorn::Stinkhorn<int32, 2>::name; \
	template aggregateType stinkhorn::Stinkhorn<int64, 2>::name;
# endif
#else
# ifdef B98_NO_64BIT_CELLS
#  define INSTANTIATE(aggregateType, name) \
	template aggregateType stinkhorn::Stinkhorn<int32, 2>::name; \
	template aggregateType stinkhorn::Stinkhorn<int32, 3>::name;
# else
#  define INSTANTIATE(aggregateType, name) \
	template aggregateType stinkhorn::Stinkhorn<int32, 2>::name; \
	template aggregateType stinkhorn::Stinkhorn<int64, 2>::name; \
	template aggregateType stinkhorn::Stinkhorn<int32, 3>::name; \
	template aggregateType stinkhorn::Stinkhorn<int64, 3>::name;
# endif
#endif

#ifndef _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#endif

#ifndef DEBUG
#define _SCL_SECURE 0
#else
#define _SCL_SECURE 1
#endif

#endif
