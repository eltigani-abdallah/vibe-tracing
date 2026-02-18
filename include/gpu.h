#pragma once

#include "rt.h"
#include "camera.h"

typedef struct s_gpu_ctx
{
	int	width;
	int	height;
	int	num_threads;
}	t_gpu_ctx;

bool	gpu_init(t_gpu_ctx *ctx, int width, int height);
bool	gpu_render(t_gpu_ctx *ctx, const camera *cam, uint32_t *pixels, double time_s);
void	gpu_destroy(t_gpu_ctx *ctx);
