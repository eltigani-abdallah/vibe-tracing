#include "rt.h"
#include "render.h"
#include "gpu.h"

#include <math.h>

static void	handle_event(SDL_Event *e, bool *running, bool *toggle_auto)
{
	if (e->type == SDL_QUIT)
	{
		*running = false;
		return ;
	}
	if (e->type == SDL_KEYDOWN)
	{
		if (e->key.keysym.sym == SDLK_ESCAPE)
		{
			*running = false;
			return ;
		}
		if (e->key.keysym.sym == SDLK_SPACE)
			*toggle_auto = true;
	}
}

int	main(int argc, char **argv)
{
	t_app		app;
	t_gpu_ctx	gpu;
	camera		cam;
	bool		running;
	bool		auto_mode;
	bool		use_gpu;
	vec3		cam_pos;
	double		time_s;
	uint32_t	last_ticks;

	(void)argc;
	(void)argv;
	if (!app_init(&app, "Ray Tracing (Chihiro) - Multi-threaded", 800, 600))
		return (1);
	
	use_gpu = gpu_init(&gpu, app.width, app.height);
	
	auto_mode = true;
	cam_pos = v3(0.0, 1.2, -4.0);
	time_s = 0.0;
	last_ticks = SDL_GetTicks();
	running = true;
	while (running)
	{
		SDL_Event	e;
		bool		toggle_auto;
		uint32_t	now;
		double		dt;
		const uint8_t	*keys;
		double		dx;
		double		dz;

		toggle_auto = false;
		while (SDL_PollEvent(&e))
			handle_event(&e, &running, &toggle_auto);
		if (toggle_auto)
			auto_mode = !auto_mode;
		now = SDL_GetTicks();
		dt = (double)(now - last_ticks) / 1000.0;
		last_ticks = now;
		if (dt > 0.1)
			dt = 0.1;
		time_s += dt;
		keys = SDL_GetKeyboardState(NULL);
		dx = 0.0;
		dz = 0.0;
		if (auto_mode)
		{
			cam_pos.x = 0.0;
			cam_pos.y = 1.2 + sin(time_s * 0.8) * 0.15;
			cam_pos.z += 2.0 * dt;
		}
		else
		{
			const double	speed = 6.0;

			dx = (keys[SDL_SCANCODE_A] ? 1.0 : 0.0) - (keys[SDL_SCANCODE_D] ? 1.0 : 0.0);
			dz = (keys[SDL_SCANCODE_W] ? 1.0 : 0.0) - (keys[SDL_SCANCODE_S] ? 1.0 : 0.0);
			cam_pos.x += dx * speed * dt;
			cam_pos.z += dz * speed * dt;
		}
		camera_lookat(&cam, cam_pos, v3_add(cam_pos, v3(0.0, -0.05, 1.0)),
			v3(0.0, 1.0, 0.0), 60.0, (double)app.width / (double)app.height);
		
		if (use_gpu)
		{
			if (!gpu_render(&gpu, &cam, app.pixels, time_s))
				use_gpu = false;
		}
		
		if (!app_present(&app))
			break ;
		SDL_Delay(1);
	}
	
	gpu_destroy(&gpu);
	app_destroy(&app);
	return (0);
}


