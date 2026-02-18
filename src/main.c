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
	bool		mouse_look;
	vec3		cam_pos;
	vec3		cam_target;
	double		time_s;
	uint32_t	last_ticks;
	double		yaw;
	double		pitch;

	(void)argc;
	(void)argv;
	if (!app_init(&app, "Ray Tracing (Chihiro) - Multi-threaded", 800, 600))
		return (1);
	
	use_gpu = gpu_init(&gpu, app.width, app.height);
	
	auto_mode = true;
	mouse_look = false;
	cam_pos = v3(0.0, 1.2, -4.0);
	yaw = 0.0;
	pitch = 0.0;
	time_s = 0.0;
	last_ticks = SDL_GetTicks();
	running = true;
	SDL_SetRelativeMouseMode(SDL_FALSE);
	while (running)
	{
		SDL_Event	e;
		bool		toggle_auto;
		uint32_t	now;
		double		dt;
		const uint8_t	*keys;
		double		dx;
		double		dz;
		int			mouse_x;
		int			mouse_y;
		uint32_t	mouse_state;

		toggle_auto = false;
		while (SDL_PollEvent(&e))
		{
			handle_event(&e, &running, &toggle_auto);
			if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT)
				mouse_look = true;
			if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_RIGHT)
				mouse_look = false;
		}
		if (toggle_auto)
			auto_mode = !auto_mode;
		
		mouse_state = SDL_GetMouseState(&mouse_x, &mouse_y);
		if (mouse_look && (mouse_state & SDL_BUTTON_RMASK))
		{
			static int last_mouse_x = 0;
			static int last_mouse_y = 0;
			int delta_x = mouse_x - last_mouse_x;
			int delta_y = mouse_y - last_mouse_y;
			last_mouse_x = mouse_x;
			last_mouse_y = mouse_y;
			
			yaw += delta_x * 0.005;
			pitch -= delta_y * 0.005;
			if (pitch > 1.57)
				pitch = 1.57;
			if (pitch < -1.57)
				pitch = -1.57;
		}
		
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
			yaw = 0.0;
			pitch = 0.0;
		}
		else
		{
			const double	speed = 6.0;

			dx = (keys[SDL_SCANCODE_A] ? 1.0 : 0.0) - (keys[SDL_SCANCODE_D] ? 1.0 : 0.0);
			dz = (keys[SDL_SCANCODE_W] ? 1.0 : 0.0) - (keys[SDL_SCANCODE_S] ? 1.0 : 0.0);
			cam_pos.x += dx * speed * dt;
			cam_pos.z += dz * speed * dt;
		}
		
		double	cos_yaw = cos(yaw);
		double	sin_yaw = sin(yaw);
		double	cos_pitch = cos(pitch);
		double	sin_pitch = sin(pitch);
		
		cam_target = v3_add(cam_pos, v3(
			sin_yaw * cos_pitch,
			sin_pitch,
			cos_yaw * cos_pitch
		));
		
		camera_lookat(&cam, cam_pos, cam_target,
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


