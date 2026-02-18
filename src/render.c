#include "render.h"

#include <math.h>

typedef struct s_hit
{
	double	t;
	vec3	p;
	vec3	n_unit;
	uint32_t	color;
}	hit;

typedef struct s_sphere
{
	vec3		center;
	double		radius;
	uint32_t	color;
}	sphere;

static inline uint32_t	argb_u8(int r, int g, int b)
{
	return (0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b);
}

static inline int	clampi(int v, int lo, int hi)
{
	if (v < lo)
		return (lo);
	if (v > hi)
		return (hi);
	return (v);
}

static uint32_t	color_lerp(uint32_t a, uint32_t b, double t)
{
	const int	ar = (int)((a >> 16) & 0xFF);
	const int	ag = (int)((a >> 8) & 0xFF);
	const int	ab = (int)(a & 0xFF);
	const int	br = (int)((b >> 16) & 0xFF);
	const int	bg = (int)((b >> 8) & 0xFF);
	const int	bb = (int)(b & 0xFF);

	const int	r = (int)round((1.0 - t) * (double)ar + t * (double)br);
	const int	g = (int)round((1.0 - t) * (double)ag + t * (double)bg);
	const int	bc = (int)round((1.0 - t) * (double)ab + t * (double)bb);

	return (argb_u8(clampi(r, 0, 255), clampi(g, 0, 255), clampi(bc, 0, 255)));
}

static bool	hit_sphere(ray r, sphere s, double tmin, double tmax, hit *out)
{
	const vec3	oc = v3_sub(r.origin, s.center);
	const double	a = v3_dot(r.dir, r.dir);
	const double	half_b = v3_dot(oc, r.dir);
	const double	c = v3_dot(oc, oc) - s.radius * s.radius;
	const double	discriminant = half_b * half_b - a * c;

	if (discriminant < 0.0)
		return (false);
	{
		const double	sqrtd = sqrt(discriminant);
		double			root = (-half_b - sqrtd) / a;

		if (root < tmin || root > tmax)
		{
			root = (-half_b + sqrtd) / a;
			if (root < tmin || root > tmax)
				return (false);
		}
		out->t = root;
		out->p = ray_at(r, root);
		out->n_unit = v3_norm(v3_div(v3_sub(out->p, s.center), s.radius));
		out->color = s.color;
		return (true);
	}
}

static bool	hit_plane_y0(ray r, double tmin, double tmax, hit *out)
{
	const double	eps = 1e-9;
	double			t;

	if (fabs(r.dir.y) < eps)
		return (false);
	t = (0.0 - r.origin.y) / r.dir.y;
	if (t < tmin || t > tmax)
		return (false);
	out->t = t;
	out->p = ray_at(r, t);
	out->n_unit = v3(0.0, 1.0, 0.0);
	if (v3_dot(out->n_unit, r.dir) > 0.0)
		out->n_unit = v3_mul(out->n_unit, -1.0);
	out->color = argb_u8(20, 80, 110);
	return (true);
}

static uint32_t	trace_primary(ray r)
{
	hit		h;
	hit		best;
	bool	any;
	double	tmax;

	any = false;
	tmax = 1e30;
	{
		const sphere	s = {.center = v3(0.0, 1.0, 3.0), .radius = 1.0,
			.color = argb_u8(220, 40, 40)};
		if (hit_sphere(r, s, 0.001, tmax, &h))
		{
			best = h;
			tmax = h.t;
			any = true;
		}
	}
	if (hit_plane_y0(r, 0.001, tmax, &h))
	{
		best = h;
		any = true;
	}
	if (any)
		return (best.color);
	{
		const double	t = 0.5 * (r.dir.y + 1.0);
		return (color_lerp(argb_u8(20, 40, 120), argb_u8(120, 170, 255), t));
	}
}

void	render_frame(t_app *app, const camera *cam)
{
	int	y;

	y = 0;
	while (y < app->height)
	{
		int	x = 0;
		while (x < app->width)
		{
			const ray		r = camera_ray_for_pixel(cam, x, y, app->width, app->height);
			app->pixels[y * app->width + x] = trace_primary(r);
			x++;
		}
		y++;
	}
}

