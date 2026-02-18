#pragma once

#include <stdbool.h>
#include <stdint.h>

#if defined(__has_include)
#  if __has_include(<SDL2/SDL.h>)
#    include <SDL2/SDL.h>
#  else
#    include <SDL.h>
#  endif
#else
#  include <SDL.h>
#endif

typedef struct s_app
{
	int			width;
	int			height;
	SDL_Window	*window;
	SDL_Renderer	*renderer;
	SDL_Texture	*texture;
	uint32_t		*pixels; /* ARGB8888 */
}	t_app;

bool	app_init(t_app *app, const char *title, int width, int height);
void	app_destroy(t_app *app);
void	app_clear(t_app *app, uint32_t argb);
bool	app_present(t_app *app);
