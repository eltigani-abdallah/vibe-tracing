#include "rt.h"
#include "render.h"

static bool	handle_event(SDL_Event *e, bool *running)
{
	if (e->type == SDL_QUIT)
	{
		*running = false;
		return (true);
	}
	if (e->type == SDL_KEYDOWN)
	{
		if (e->key.keysym.sym == SDLK_ESCAPE)
		{
			*running = false;
			return (true);
		}
	}
	return (false);
}

int	main(int argc, char **argv)
{
	t_app	app;
	camera	cam;
	bool	running;

	(void)argc;
	(void)argv;
	if (!app_init(&app, "Ray Tracing (Chihiro)", 800, 600))
		return (1);
	camera_lookat(&cam, v3(0.0, 1.2, -4.0), v3(0.0, 0.8, 3.0), v3(0.0, 1.0, 0.0),
		60.0, (double)app.width / (double)app.height);
	running = true;
	while (running)
	{
		SDL_Event	e;

		while (SDL_PollEvent(&e))
			handle_event(&e, &running);
		render_frame(&app, &cam);
		if (!app_present(&app))
			break ;
		SDL_Delay(1);
	}
	app_destroy(&app);
	return (0);
}
