#include "render.h"

#include <math.h>

typedef struct s_hit
{
	double	t;
	vec3	p;
	vec3	n_unit;
	vec3	albedo;
	bool	mirror;
}	hit;

typedef struct s_sphere
{
	vec3		center;
	double		radius;
	vec3		albedo;
}	sphere;

static inline vec3	c3(double r, double g, double b)
{
	return (v3(r, g, b));
}

static inline vec3	c3_mul(vec3 a, double s)
{
	return (v3_mul(a, s));
}

static inline vec3	c3_hadamard(vec3 a, vec3 b)
{
	return (v3(a.x * b.x, a.y * b.y, a.z * b.z));
}

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

static vec3	c3_lerp(vec3 a, vec3 b, double t)
{
	return (v3_add(c3_mul(a, 1.0 - t), c3_mul(b, t)));
}

static uint32_t	c3_to_argb(vec3 c)
{
	const int	r = (int)round(255.0 * c.x);
	const int	g = (int)round(255.0 * c.y);
	const int	b = (int)round(255.0 * c.z);

	return (argb_u8(clampi(r, 0, 255), clampi(g, 0, 255), clampi(b, 0, 255)));
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
		out->albedo = s.albedo;
		out->mirror = false;
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
	out->albedo = c3(0.10, 0.18, 0.25);
	out->mirror = true;
	return (true);
}

static vec3	sky_color(vec3 dir)
{
	const double	t = 0.5 * (dir.y + 1.0);

	return (c3_lerp(c3(1.0, 0.55, 0.35), c3(0.24, 0.08, 0.47), t));
}

static bool	scene_intersect(ray r, double tmin, double tmax, hit *out)
{
	hit		h;
	bool	any;

	any = false;
	{
		const sphere	s = {.center = v3(0.0, 1.0, 3.0), .radius = 1.0,
			.albedo = c3(0.86, 0.18, 0.18)};
		if (hit_sphere(r, s, tmin, tmax, &h))
		{
			*out = h;
			tmax = h.t;
			any = true;
		}
	}
	if (hit_plane_y0(r, tmin, tmax, &h))
	{
		*out = h;
		any = true;
	}
	return (any);
}

static vec3	shade_diffuse(hit h)
{
	const vec3	light_dir = v3_norm(v3(0.6, 1.0, -0.2));
	const double	nl = fmax(0.0, v3_dot(h.n_unit, light_dir));
	const double	ambient = 0.22;

	return (c3_mul(h.albedo, ambient + (1.0 - ambient) * nl));
}

static vec3	trace(ray r, int depth)
{
	hit	h;

	if (depth <= 0)
		return (c3(0.0, 0.0, 0.0));
	if (scene_intersect(r, 0.001, 1e30, &h))
	{
		if (h.mirror)
		{
			const vec3	ref_dir = v3_norm(v3_reflect(r.dir, h.n_unit));
			const ray	ref = {.origin = v3_add(h.p, v3_mul(h.n_unit, 1e-4)),
				.dir = ref_dir};
			return (c3_hadamard(h.albedo, trace(ref, depth - 1)));
		}
		return (shade_diffuse(h));
	}
	return (sky_color(r.dir));
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
			app->pixels[y * app->width + x] = c3_to_argb(trace(r, 3));
			x++;
		}
		y++;
	}
}

