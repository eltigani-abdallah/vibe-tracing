#include "gpu.h"
#include "render.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct s_thread_work
{
	t_app		*app;
	const camera	*cam;
	double		time_s;
	int			start_y;
	int			end_y;
	int			scale;
}	t_thread_work;

static void *render_thread(void *arg)
{
	t_thread_work	*work = (t_thread_work *)arg;
	int				y;

	y = work->start_y;
	while (y < work->end_y)
	{
		int	x = 0;
		while (x < work->app->width)
		{
			const ray	r = camera_ray_for_pixel(work->cam, x, y, 
				work->app->width, work->app->height);
			const uint32_t	argb = c3_to_argb(trace(r, 3));

			fill_block(work->app, x, y, work->scale, argb);
			x += work->scale;
		}
		y += work->scale;
	}
	return (NULL);
}

bool gpu_init(t_gpu_ctx *ctx, int width, int height)
{
	if (!ctx)
		return (false);
	ctx->width = width;
	ctx->height = height;
	ctx->num_threads = 4;
	fprintf(stderr, "Multi-threaded CPU rendering initialized (%d threads)\n", ctx->num_threads);
	return (true);
}

bool gpu_render(t_gpu_ctx *ctx, const camera *cam, uint32_t *pixels, double time_s)
{
	pthread_t		threads[16];
	t_thread_work	work[16];
	t_app			app;
	int				i;
	int				stripe_height;

	if (!ctx || !cam || !pixels)
		return (false);
	
	app.width = ctx->width;
	app.height = ctx->height;
	app.pixels = pixels;
	
	stripe_height = ctx->height / ctx->num_threads;
	if (stripe_height < 8)
		stripe_height = 8;
	
	for (i = 0; i < ctx->num_threads; i++)
	{
		work[i].app = &app;
		work[i].cam = cam;
		work[i].time_s = time_s;
		work[i].start_y = i * stripe_height;
		work[i].end_y = (i == ctx->num_threads - 1) ? ctx->height : (i + 1) * stripe_height;
		work[i].scale = 1;
		
		if (pthread_create(&threads[i], NULL, render_thread, &work[i]) != 0)
		{
			fprintf(stderr, "Failed to create thread %d\n", i);
			return (false);
		}
	}
	
	for (i = 0; i < ctx->num_threads; i++)
	{
		if (pthread_join(threads[i], NULL) != 0)
		{
			fprintf(stderr, "Failed to join thread %d\n", i);
			return (false);
		}
	}
	
	return (true);
}

void gpu_destroy(t_gpu_ctx *ctx)
{
	if (ctx)
		memset(ctx, 0, sizeof(*ctx));
}
