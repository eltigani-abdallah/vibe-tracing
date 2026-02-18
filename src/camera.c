#include "camera.h"

#include <math.h>

void	camera_lookat(camera *c, vec3 origin, vec3 target, vec3 world_up,
			double vfov_deg, double aspect)
{
	const double	pi = 3.14159265358979323846;
	const double	theta = vfov_deg * (pi / 180.0);
	const double	half_h = tan(theta * 0.5);
	const double	viewport_h = 2.0 * half_h;
	const double	viewport_w = aspect * viewport_h;

	const vec3	forward = v3_norm(v3_sub(target, origin));
	const vec3	right = v3_norm(v3_cross(forward, world_up));
	const vec3	up = v3_cross(right, forward);

	c->origin = origin;
	c->horizontal = v3_mul(right, viewport_w);
	c->vertical = v3_mul(up, viewport_h);
	c->lower_left = v3_sub(
			v3_sub(
				v3_add(origin, forward),
				v3_mul(c->horizontal, 0.5)),
			v3_mul(c->vertical, 0.5));
}

ray	camera_ray_for_pixel(const camera *c, int x, int y, int width, int height)
{
	const double	u = ((double)x + 0.5) / (double)width;
	const double	v = 1.0 - (((double)y + 0.5) / (double)height);

	const vec3	p = v3_add(v3_add(c->lower_left, v3_mul(c->horizontal, u)),
			v3_mul(c->vertical, v));
	const vec3	dir = v3_norm(v3_sub(p, c->origin));

	return ((ray){c->origin, dir});
}

