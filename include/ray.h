#pragma once

#include "vec3.h"

typedef struct s_ray
{
	vec3	origin;
	vec3	dir;
}	ray;

static inline vec3	ray_at(ray r, double t)
{
	return (v3_add(r.origin, v3_mul(r.dir, t)));
}

