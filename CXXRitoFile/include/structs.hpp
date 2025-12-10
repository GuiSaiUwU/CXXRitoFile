#pragma once  
#include <cstdint>
#include <cmath>

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


	struct Quaternion {
		float x, y, z, w;

		Quaternion operator*(const Quaternion& other) const {
			return Quaternion{
				w * other.x + x * other.w + y * other.z - z * other.y,
				w * other.y - x * other.z + y * other.w + z * other.x,
				w * other.z + x * other.y - y * other.x + z * other.w,
				w * other.w - x * other.x - y * other.y - z * other.z
			};
		}

		Quaternion& operator*(const float scalar) {
			this->x = this->x * scalar;
			this->y = this->y * scalar;
			this->z = this->z * scalar;
			this->w = this->w * scalar;
			return *this;
		}

		constexpr static float epsilon = 1e-6;
		constexpr static float epsilon_subtracted = (1.0 - epsilon);

		Quaternion slerp(const Quaternion& quat1, const Quaternion& quat2, float weight) {
			float cos_omega = quat1.x * quat2.x + quat1.y * quat2.y +
				quat1.z * quat2.z + quat1.w * quat2.w;

			bool flip = false;
			if (cos_omega < 0.0) {
				flip = true;
				cos_omega = -cos_omega;
			}

			float s1, s2;

			if (cos_omega > epsilon_subtracted) { // Linear interpolation for very small angles
				s1 = 1.0 - weight;
				s2 = flip ? -weight : weight;
			}
			else {
				float omega = std::acos(cos_omega);
				float sin_omega = std::sin(omega);

				// Avoid division by zero
				if (std::fabs(sin_omega) < epsilon) {
					s1 = 1.0 - weight;
					s2 = flip ? -weight : weight;
				}
				else {
					float inv_sin_omega = 1.0 / sin_omega;
					s1 = std::sin((1.0 - weight) * omega) * inv_sin_omega;
					s2 = flip ? -std::sin(weight * omega) * inv_sin_omega
						: std::sin(weight * omega) * inv_sin_omega;
				}
			}

			return Quaternion(
				s1 * quat1.x + s2 * quat2.x,
				s1 * quat1.y + s2 * quat2.y,
				s1 * quat1.z + s2 * quat2.z,
				s1 * quat1.w + s2 * quat2.w
			);
		}

	};
}