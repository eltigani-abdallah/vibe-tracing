#include "rt.h"

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

int	main(void)
{
	t_app	app;
	bool	running;

	if (!app_init(&app, "Ray Tracing (Chihiro)", 800, 600))
		return (1);
	running = true;
	while (running)
	{
		SDL_Event	e;

		while (SDL_PollEvent(&e))
			handle_event(&e, &running);
		app_clear(&app, 0xFF000000u);
		if (!app_present(&app))
			break ;
		SDL_Delay(1);
	}
	app_destroy(&app);
	return (0);
}
