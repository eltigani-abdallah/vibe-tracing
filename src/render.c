#include "render.h"

#include <math.h>

static double	g_time_s = 0.0;

typedef struct s_hit
{
	double	t;
	vec3	p;
	vec3	n_unit;
	vec3	albedo;
	bool	mirror;
}	hit;

typedef struct s_box
{
	vec3	min;
	vec3	max;
	vec3	albedo;
	bool	mirror;
}	box;

typedef struct s_cylinder
{
	vec3	center_xz;
	double	radius;
	double	y0;
	double	y1;
	vec3	albedo;
}	cylinder;

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
	{
		const double	w = 0.10;
		const double	nx = w * sin(out->p.x * 0.55 + g_time_s * 1.7)
			+ (0.5 * w) * sin(out->p.z * 0.22 + g_time_s * 0.9);
		const double	nz = w * sin(out->p.z * 0.48 - g_time_s * 1.3);

		out->n_unit = v3_norm(v3(-nx, 1.0, -nz));
	}
	if (v3_dot(out->n_unit, r.dir) > 0.0)
		out->n_unit = v3_mul(out->n_unit, -1.0);
	out->albedo = c3(0.0, 0.3, 0.6);
	out->mirror = true;
	return (true);
}

static bool	hit_box(ray r, box b, double tmin, double tmax, hit *out)
{
	double	t_enter;
	double	t_exit;
	vec3	n_enter;

	t_enter = tmin;
	t_exit = tmax;
	n_enter = v3(0.0, 1.0, 0.0);
	{
		const double	inv = 1.0 / r.dir.x;
		double			t0 = (b.min.x - r.origin.x) * inv;
		double			t1 = (b.max.x - r.origin.x) * inv;
		vec3			n0 = (inv >= 0.0) ? v3(-1.0, 0.0, 0.0) : v3(1.0, 0.0, 0.0);
		vec3			n1 = (inv >= 0.0) ? v3(1.0, 0.0, 0.0) : v3(-1.0, 0.0, 0.0);

		if (t0 > t1)
		{
			const double	tmp = t0; t0 = t1; t1 = tmp;
			{ const vec3	tn = n0; n0 = n1; n1 = tn; }
		}
		if (t0 > t_enter)
		{
			t_enter = t0;
			n_enter = n0;
		}
		if (t1 < t_exit)
			t_exit = t1;
		if (t_exit <= t_enter)
			return (false);
	}
	{
		const double	inv = 1.0 / r.dir.y;
		double			t0 = (b.min.y - r.origin.y) * inv;
		double			t1 = (b.max.y - r.origin.y) * inv;
		vec3			n0 = (inv >= 0.0) ? v3(0.0, -1.0, 0.0) : v3(0.0, 1.0, 0.0);
		vec3			n1 = (inv >= 0.0) ? v3(0.0, 1.0, 0.0) : v3(0.0, -1.0, 0.0);

		if (t0 > t1)
		{
			const double	tmp = t0; t0 = t1; t1 = tmp;
			{ const vec3	tn = n0; n0 = n1; n1 = tn; }
		}
		if (t0 > t_enter)
		{
			t_enter = t0;
			n_enter = n0;
		}
		if (t1 < t_exit)
			t_exit = t1;
		if (t_exit <= t_enter)
			return (false);
	}
	{
		const double	inv = 1.0 / r.dir.z;
		double			t0 = (b.min.z - r.origin.z) * inv;
		double			t1 = (b.max.z - r.origin.z) * inv;
		vec3			n0 = (inv >= 0.0) ? v3(0.0, 0.0, -1.0) : v3(0.0, 0.0, 1.0);
		vec3			n1 = (inv >= 0.0) ? v3(0.0, 0.0, 1.0) : v3(0.0, 0.0, -1.0);

		if (t0 > t1)
		{
			const double	tmp = t0; t0 = t1; t1 = tmp;
			{ const vec3	tn = n0; n0 = n1; n1 = tn; }
		}
		if (t0 > t_enter)
		{
			t_enter = t0;
			n_enter = n0;
		}
		if (t1 < t_exit)
			t_exit = t1;
		if (t_exit <= t_enter)
			return (false);
	}
	out->t = t_enter;
	out->p = ray_at(r, t_enter);
	out->n_unit = n_enter;
	if (v3_dot(out->n_unit, r.dir) > 0.0)
		out->n_unit = v3_mul(out->n_unit, -1.0);
	out->albedo = b.albedo;
	out->mirror = b.mirror;
	return (true);
}

static bool	hit_cylinder_y(ray r, cylinder c, double tmin, double tmax, hit *out)
{
	const vec3	oc = v3_sub(r.origin, v3(c.center_xz.x, 0.0, c.center_xz.z));
	const double	a = r.dir.x * r.dir.x + r.dir.z * r.dir.z;
	const double	half_b = oc.x * r.dir.x + oc.z * r.dir.z;
	const double	cc = oc.x * oc.x + oc.z * oc.z - c.radius * c.radius;
	const double	discriminant = half_b * half_b - a * cc;

	if (a == 0.0 || discriminant < 0.0)
		return (false);
	{
		const double	sqrtd = sqrt(discriminant);
		double			root = (-half_b - sqrtd) / a;
		double			y;

		y = r.origin.y + root * r.dir.y;
		if (root < tmin || root > tmax || y < c.y0 || y > c.y1)
		{
			root = (-half_b + sqrtd) / a;
			y = r.origin.y + root * r.dir.y;
			if (root < tmin || root > tmax || y < c.y0 || y > c.y1)
				return (false);
		}
		out->t = root;
		out->p = ray_at(r, root);
		out->n_unit = v3_norm(v3(out->p.x - c.center_xz.x, 0.0, out->p.z - c.center_xz.z));
		if (v3_dot(out->n_unit, r.dir) > 0.0)
			out->n_unit = v3_mul(out->n_unit, -1.0);
		out->albedo = c.albedo;
		out->mirror = false;
		return (true);
	}
}

static vec3	sky_color(vec3 dir)
{
	const double	t = 0.5 * (dir.y + 1.0);

	return (c3_lerp(c3(0.3, 0.7, 1.0), c3(0.0, 0.2, 0.8), t));
}

static bool	scene_intersect(ray r, double tmin, double tmax, hit *out)
{
	hit		h;
	bool	any;

	any = false;
	{
		const box	submerged_platform = {
			.min = v3(-8.0, -0.3, 0.0),
			.max = v3(8.0, 0.0, 260.0),
			.albedo = c3(0.40, 0.40, 0.43),
			.mirror = false
		};
		if (hit_box(r, submerged_platform, tmin, tmax, &h))
		{
			*out = h;
			tmax = h.t;
			any = true;
		}
	}
	{
		int	i = 0;
		while (i < 14)
		{
			const double	z = 20.0 * (double)i;
			const cylinder	pole_l = {.center_xz = v3(-6.2, 0.0, z), .radius = 0.18,
				.y0 = 0.0, .y1 = 3.2, .albedo = c3(0.22, 0.18, 0.12)};
			const cylinder	pole_r = {.center_xz = v3(6.2, 0.0, z), .radius = 0.18,
				.y0 = 0.0, .y1 = 3.2, .albedo = c3(0.22, 0.18, 0.12)};

			if (hit_cylinder_y(r, pole_l, tmin, tmax, &h))
			{
				*out = h;
				tmax = h.t;
				any = true;
			}
			if (hit_cylinder_y(r, pole_r, tmin, tmax, &h))
			{
				*out = h;
				tmax = h.t;
				any = true;
			}
			i++;
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
	const vec3	light_dir = v3_norm(v3(-1.0, 2.0, 0.5));
	hit			shadow_test = {0};
	const ray	shadow_ray = {.origin = v3_add(h.p, v3_mul(h.n_unit, 1e-4)),
		.dir = light_dir};
	double		nl;
	double		ambient;
	bool		in_shadow;

	in_shadow = scene_intersect(shadow_ray, 0.001, 1e30, &shadow_test);
	nl = fmax(0.0, v3_dot(h.n_unit, light_dir));
	ambient = 0.22;
	if (in_shadow)
		nl *= 0.3;
	return (c3_mul(h.albedo, ambient + (1.0 - ambient) * nl));
}

static vec3	trace(ray r, int depth)
{
	hit	h = {0};

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

static void	fill_block(t_app *app, int x0, int y0, int scale, uint32_t argb)
{
	int	y;

	y = 0;
	while (y < scale)
	{
		int	x = 0;
		const int	yy = y0 + y;

		if (yy >= app->height)
			break ;
		while (x < scale)
		{
			const int	xx = x0 + x;

			if (xx >= app->width)
				break ;
			app->pixels[yy * app->width + xx] = argb;
			x++;
		}
		y++;
	}
}

void	render_frame(t_app *app, const camera *cam, int scale, double time_s)
{
	int	y;

	g_time_s = time_s;
	if (scale < 1)
		scale = 1;
	y = 0;
	while (y < app->height)
	{
		int	x = 0;
		while (x < app->width)
		{
			vec3	color = v3(0.0, 0.0, 0.0);
			int	samples = (scale > 1) ? 2 : 4;
			int	s = 0;

			while (s < samples)
			{
				int	t = 0;
				while (t < samples)
				{
					const double	offset_x = (double)s / (double)samples + 0.25;
					const double	offset_y = (double)t / (double)samples + 0.25;
					const int		sx = x + (scale / 2) + (int)(offset_x - 0.5);
					const int		sy = y + (scale / 2) + (int)(offset_y - 0.5);
					const ray		r = camera_ray_for_pixel(cam, sx, sy, app->width, app->height);

					color = v3_add(color, trace(r, 3));
					t++;
				}
				s++;
			}
			color = v3_mul(color, 1.0 / (double)(samples * samples));
			fill_block(app, x, y, scale, c3_to_argb(color));
			x += scale;
		}
		y += scale;
	}
}

