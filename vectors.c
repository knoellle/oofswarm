#include <math.h>
#include <stdlib.h>

struct Vectorf
{
	float x;
	float y;
	float z;
	float w;
};

Vectorf vecf(float x, float y, float z = 0.f, float w = 0.f)
{
	Vectorf v;
	v.x = x;
	v.y = y;
	v.z = z;
	v.w = w;
	return v;
}

// lengths

inline float veclen(Vectorf v)
{
	return sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

inline float veclensqr(Vectorf v)
{
	return (v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

// arithmetic functions

inline Vectorf vecmult(Vectorf v1, Vectorf v2)
{
	Vectorf v;
	v.x = v1.x * v2.x;
	v.y = v1.y * v2.y;
	v.z = v1.z * v2.z;
	v.w = v1.w * v2.w;
	return v;
}

inline Vectorf vecadd(Vectorf v1, Vectorf v2)
{
	Vectorf v;
	v.x = v1.x + v2.x;
	v.y = v1.y + v2.y;
	v.z = v1.z + v2.z;
	v.w = v1.w + v2.w;
	return v;
}

inline Vectorf vecsub(Vectorf v1, Vectorf v2)
{
	Vectorf v;
	v.x = v1.x - v2.x;
	v.y = v1.y - v2.y;
	v.z = v1.z - v2.z;
	v.w = v1.w - v2.w;
	return v;
} 

inline Vectorf vecscale(Vectorf v1, float f)
{
	Vectorf v;
	v.x = v1.x * f;
	v.y = v1.y * f;
	v.z = v1.z * f;
	v.w = v1.w * f;
	return v;
}

inline Vectorf normalize(Vectorf v1)
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

inline Vectorf randomBetween(Vectorf v1, Vectorf v2)
{
	Vectorf v;
	v.x = v1.x + (v2.x - v1.x) * randomFloat();
	v.y = v1.y + (v2.y - v1.y) * randomFloat();
	v.z = v1.z + (v2.z - v1.z) * randomFloat();
	v.w = v1.w + (v2.w - v1.w) * randomFloat();
	return v;
}