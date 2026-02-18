#pragma once

#include "ray.h"

typedef struct s_camera
{
	vec3	origin;
	vec3	lower_left;
	vec3	horizontal;
	vec3	vertical;
}	camera;

void	camera_lookat(camera *c, vec3 origin, vec3 target, vec3 world_up,
			double vfov_deg, double aspect);
ray		camera_ray_for_pixel(const camera *c, int x, int y, int width, int height);

