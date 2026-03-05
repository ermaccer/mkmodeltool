#include "Vector.h"

inline float RecipSqrt(float x, float y) { return x / sqrt(y); }
inline float RecipSqrt(float x) { return RecipSqrt(1.0f, x); }

void Vec3::Normalise()
{
	float sq = MagnitudeSqr();
	if (sq > 0.0f) {
		float invsqrt = RecipSqrt(sq);
		x *= invsqrt;
		y *= invsqrt;
		z *= invsqrt;
	}
	else
		x = 1.0f;
}


Vec3 CrossProduct(const Vec3& v1, const Vec3& v2)
{
	return Vec3(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
}

float DotProduct(const Vec3& v1, const Vec3& v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}
