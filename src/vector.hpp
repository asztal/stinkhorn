#ifndef B98_VECTOR_HPP_INCLUDED
#define B98_VECTOR_HPP_INCLUDED

#include <iostream> 

namespace stinkhorn {
	template<class T = int>
	struct vector3 {
		T x,y,z;

		vector3():
		x(), y(), z()
		{}

		vector3(T x, T y, T z):
		x(x), y(y), z(z)
		{}

		vector3<T> operator-() const {
			return vector3<T>(-x,-y,-z);
		}
	};

	template<class T>
	vector3<T> operator +(vector3<T> const& lhs, vector3<T> const& rhs) {
		return vector3<T>(lhs.x + rhs.x,
			lhs.y + rhs.y,
			lhs.z + rhs.z);
	}

	template<class T>
	vector3<T> operator -(vector3<T> const& lhs, vector3<T> const& rhs) {
		return vector3<T>(lhs.x - rhs.x,
			lhs.y - rhs.y,
			lhs.z - rhs.z);
	}

	template<class T>
	vector3<T> operator /(vector3<T> const& lhs, vector3<T> const& rhs) {
		return vector3<T>(lhs.x / rhs.x,
			lhs.y / rhs.y,
			lhs.z / rhs.z);
	}

	template<class T, class U>
	vector3<T> operator /(vector3<T> const& lhs, U t) {
		return vector3<T>(lhs.x / t,
			lhs.y / t,
			lhs.z / t);
	}

	template<class T>
	vector3<T> operator >>(vector3<T> const& lhs, T bits) {
		return vector3<T>(lhs.x >> bits,
			lhs.y >> bits,
			lhs.z >> bits);
	}

	template<class T>
	vector3<T> operator &(vector3<T> const& lhs, T bitmask) {
		return vector3<T>(lhs.x & bitmask,
			lhs.y & bitmask,
			lhs.z & bitmask);
	}

	template<class T>
	vector3<T> operator *(vector3<T> const& lhs, vector3<T> const& rhs) {
		return vector3<T>(lhs.x * rhs.x,
			lhs.y * rhs.y,
			lhs.z * rhs.z);
	}

	template<class T>
	vector3<T> operator *(vector3<T> const& lhs, T rhs) {
		return vector3<T>(lhs.x * rhs,
			lhs.y * rhs,
			lhs.z * rhs);
	}

	template<class T>
	vector3<T> operator *(T lhs, vector3<T> const& rhs) {
		return vector3<T>(lhs * rhs.x,
			lhs * rhs.y,
			lhs * rhs.z);
	}

	template<class T>
	vector3<T>& operator +=(vector3<T>& lhs, vector3<T> const& rhs) {
		lhs.x += rhs.x;
		lhs.y += rhs.y;
		lhs.z += rhs.z;
		return lhs;
	}

	template<class T>
	vector3<T>& operator -=(vector3<T>& lhs, vector3<T> const& rhs) {
		lhs.x -= rhs.x;
		lhs.y -= rhs.y;
		lhs.z -= rhs.z;
		return lhs;
	}

	template<class T>
	vector3<T>& operator /=(vector3<T>& lhs, vector3<T> const& rhs) {
		lhs.x /= rhs.x;
		lhs.y /= rhs.y;
		lhs.z /= rhs.z;
		return lhs;
	}

	template<class T>
	vector3<T>& operator *=(vector3<T>& lhs, vector3<T> const& rhs) {
		lhs.x *= rhs.x;
		lhs.y *= rhs.y;
		lhs.z *= rhs.z;
		return lhs;
	}

	template<class T>
	vector3<T>& operator >>=(vector3<T>& lhs, T bits) {
		lhs.x >>= bits;
		lhs.y >>= bits;
		lhs.z >>= bits;
		return lhs;
	}

	template<class T>
	bool operator ==(vector3<T> const& lhs, vector3<T> const& rhs) {
		return lhs.x == rhs.x &&
			lhs.y == rhs.y &&
			lhs.z == rhs.z;
	}

	template<class T>
	bool operator !=(vector3<T> const& lhs, vector3<T> const& rhs) {
		return lhs.x != rhs.x ||
			lhs.y != rhs.y ||
			lhs.z != rhs.z;
	}

	template<class T, class CharT, class TraitsT>
	std::basic_ostream<CharT, TraitsT>& operator << (std::basic_ostream<CharT, TraitsT>& ostr, vector3<T> const& v) {
		return ostr << "{" << v.x << ", " << v.y << ", " << v.z << "}";
	}
}

#endif
