#pragma once
#include <math.h>

class Vec3
{
public:
	float x, y, z;
	Vec3(void) {}
	Vec3(float a, float b, float c)
	{
		x = a;
		y = b;
		z = c;
	}
	float Heading(void) const { return atan2(-x, y); }
	float Magnitude(void) const { return sqrt(x * x + y * y + z * z); }
	float MagnitudeSqr(void) const { return x * x + y * y + z * z; }
	float Magnitude2D(void) const { return sqrt(x * x + y * y); }
	float MagnitudeSqr2D(void) const { return x * x + y * y; }
	void Normalise();

	const Vec3& operator+=(Vec3 const& right) {
		x += right.x;
		y += right.y;
		z += right.z;
		return *this;
	}

	const Vec3& operator+=(float const& right) {
		x += right;
		y += right;
		z += right;
		return *this;
	}

	const Vec3& operator-=(Vec3 const& right) {
		x -= right.x;
		y -= right.y;
		z -= right.z;
		return *this;
	}

	const Vec3& operator*=(float right) {
		x *= right;
		y *= right;
		z *= right;
		return *this;
	}

	const Vec3& operator/=(float right) {
		x /= right;
		y /= right;
		z /= right;
		return *this;
	}

	Vec3 operator-() const {
		return Vec3(-x, -y, -z);
	}
};

inline Vec3 operator+(const Vec3& left, const Vec3& right)
{
	return Vec3(left.x + right.x, left.y + right.y, left.z + right.z);
}

inline Vec3 operator-(const Vec3& left, const Vec3& right)
{
	return Vec3(left.x - right.x, left.y - right.y, left.z - right.z);
}

inline Vec3 operator*(const Vec3& left, float right)
{
	return Vec3(left.x * right, left.y * right, left.z * right);
}

inline Vec3 operator*(float left, const Vec3& right)
{
	return Vec3(left * right.x, left * right.y, left * right.z);
}

inline Vec3 operator/(const Vec3& left, float right)
{
	return Vec3(left.x / right, left.y / right, left.z / right);
}

Vec3 CrossProduct(const Vec3& v1, const Vec3& v2);
float DotProduct(const Vec3& v1, const Vec3& v2);