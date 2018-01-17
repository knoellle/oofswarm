#include <math.h>
#include <stdlib.h>

struct Vector2f
{
	float x;
	float y;
};

Vector2f vec2f(float x, float y)
{
	Vector2f v;
	v.x = x;
	v.y = y;
	return v;
}

// lengths

inline float veclen(Vector2f v)
{
	return sqrt(v.x * v.x + v.y * v.y);
}

inline float veclensqr(Vector2f v)
{
	return (v.x * v.x + v.y * v.y);
}

// arithmetic functions

inline Vector2f vecmult(Vector2f v1, Vector2f v2)
{
	Vector2f v;
	v.x = v1.x * v2.x;
	v.y = v1.y * v2.y;
	return v;
}

inline Vector2f vecadd(Vector2f v1, Vector2f v2)
{
	Vector2f v;
	v.x = v1.x + v2.x;
	v.y = v1.y + v2.y;
	return v;
}

inline Vector2f vecsub(Vector2f v1, Vector2f v2)
{
	Vector2f v;
	v.x = v1.x - v2.x;
	v.y = v1.y - v2.y;
	return v;
} 

inline Vector2f vecscale(Vector2f v1, float f)
{
	Vector2f v;
	v.x = v1.x * f;
	v.y = v1.y * f;
	return v;
}

inline Vector2f normalize(Vector2f v1)
{
	float f = veclen(v1);
	if (f == 0.f)
	{
		return v1;
	}
	else
	{
		return vecscale(v1, 1.f/f);
	}
}

inline float randomFloat()
{
	return (float)rand()/(float)(RAND_MAX);
}

inline Vector2f randomBetween(Vector2f v1, Vector2f v2)
{
	Vector2f v;
	v.x = v1.x + (v.x - v1.x) * randomFloat();
	v.y = v1.y + (v.y - v1.y) * randomFloat();
	return v;
}