#include "rt.h"

#include <stdlib.h>
#include <string.h>

static void	app_zero(t_app *app)
{
	memset(app, 0, sizeof(*app));
}

bool	app_init(t_app *app, const char *title, int width, int height)
{
	app_zero(app);
	app->width = width;
	app->height = height;
	SDL_SetMainReady();
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
		return (false);
	app->window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
	if (!app->window)
		return (false);
	app->renderer = SDL_CreateRenderer(app->window, -1,
			SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!app->renderer)
		return (false);
	app->texture = SDL_CreateTexture(app->renderer, SDL_PIXELFORMAT_ARGB8888,
			SDL_TEXTUREACCESS_STREAMING, width, height);
	if (!app->texture)
		return (false);
	app->pixels = (uint32_t *)malloc((size_t)width * (size_t)height
			* sizeof(uint32_t));
	if (!app->pixels)
		return (false);
	app_clear(app, 0xFF000000u);
	return (true);
}

void	app_destroy(t_app *app)
{
	if (!app)
		return ;
	if (app->pixels)
		free(app->pixels);
	if (app->texture)
		SDL_DestroyTexture(app->texture);
	if (app->renderer)
		SDL_DestroyRenderer(app->renderer);
	if (app->window)
		SDL_DestroyWindow(app->window);
	SDL_Quit();
	app_zero(app);
}

void	app_clear(t_app *app, uint32_t argb)
{
	int	i;
	int	count;

	count = app->width * app->height;
	i = 0;
	while (i < count)
	{
		app->pixels[i] = argb;
		i++;
	}
}

bool	app_present(t_app *app)
{
	void	*pixels;
	int		pitch;

	if (SDL_LockTexture(app->texture, NULL, &pixels, &pitch) != 0)
		return (false);
	memcpy(pixels, app->pixels, (size_t)app->width * (size_t)app->height
		* sizeof(uint32_t));
	SDL_UnlockTexture(app->texture);
	SDL_RenderClear(app->renderer);
	if (SDL_RenderCopy(app->renderer, app->texture, NULL, NULL) != 0)
		return (false);
	SDL_RenderPresent(app->renderer);
	return (true);
}
