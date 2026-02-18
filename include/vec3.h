#pragma once

#include <math.h>

typedef struct s_vec3
{
	double	x;
	double	y;
	double	z;
}	vec3;

static inline vec3	v3(double x, double y, double z)
{
	return ((vec3){x, y, z});
}

static inline vec3	v3_add(vec3 a, vec3 b)
{
	return (v3(a.x + b.x, a.y + b.y, a.z + b.z));
}

static inline vec3	v3_sub(vec3 a, vec3 b)
{
	return (v3(a.x - b.x, a.y - b.y, a.z - b.z));
}

static inline vec3	v3_mul(vec3 a, double s)
{
	return (v3(a.x * s, a.y * s, a.z * s));
}

static inline vec3	v3_div(vec3 a, double s)
{
	return (v3(a.x / s, a.y / s, a.z / s));
}

static inline double	v3_dot(vec3 a, vec3 b)
{
	return (a.x * b.x + a.y * b.y + a.z * b.z);
}

static inline vec3	v3_cross(vec3 a, vec3 b)
{
	return (v3(
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x));
}

static inline double	v3_len2(vec3 a)
{
	return (v3_dot(a, a));
}

static inline double	v3_len(vec3 a)
{
	return (sqrt(v3_len2(a)));
}

static inline vec3	v3_norm(vec3 a)
{
	double	len;

	len = v3_len(a);
	if (len <= 0.0)
		return (v3(0.0, 0.0, 0.0));
	return (v3_div(a, len));
}

static inline vec3	v3_reflect(vec3 v, vec3 n_unit)
{
	return (v3_sub(v, v3_mul(n_unit, 2.0 * v3_dot(v, n_unit))));
}
