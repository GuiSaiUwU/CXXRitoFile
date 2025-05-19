#pragma once  
#include <cstdint>  


namespace RitoFile {
	template<class T>
	struct Container2 {
		T x, y;
		Container2() : x(T()), y(T()) {}
		Container2(const T& x, const T& y) : x(x), y(y) {}
	};

	template<class T>
	struct Container3 {
		T x, y, z;

		Container3() : x(T()), y(T()), z(T()) {}

		Container3(const T& x, const T& y, const T& z) : x(x), y(y), z(z) {}
	};

	template<class T>
	struct Container4 {
		T x, y, z, w;

		Container4() : x(T()), y(T()), z(T()), w(T()) {}

		Container4(const T& x, const T& y, const T& z, const T& w) : x(x), y(y), z(z), w(w) {}
	};

	struct Matrix4 {
		float
			a, b, c, d,
			e, f, g, h,
			i, j, k, l, 
			m, n, o, p;
	};
}