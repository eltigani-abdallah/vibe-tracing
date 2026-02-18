#pragma once

#include "camera.h"
#include "rt.h"
#include "vec3.h"

void	render_frame(t_app *app, const camera *cam, int scale, double time_s);

typedef struct s_hit
{
	double	t;
	vec3	p;
	vec3	n_unit;
	vec3	albedo;
	bool	mirror;
}	hit;

vec3	trace(ray r, int depth);
void	fill_block(t_app *app, int x0, int y0, int scale, uint32_t argb);
uint32_t	c3_to_argb(vec3 c);
vec3	c3_mul(vec3 a, double s);
vec3	c3(double r, double g, double b);
vec3	v3_add(vec3 a, vec3 b);
vec3	v3_mul(vec3 a, double s);
double	v3_dot(vec3 a, vec3 b);
