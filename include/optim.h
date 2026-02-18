#pragma once

#include <stddef.h>

typedef struct s_mem_pool
{
	char	*buffer;
	size_t	capacity;
	size_t	used;
}	t_mem_pool;

t_mem_pool	*mem_pool_create(size_t capacity);
void		mem_pool_reset(t_mem_pool *pool);
void		mem_pool_destroy(t_mem_pool *pool);
void		*mem_pool_alloc(t_mem_pool *pool, size_t size);

typedef struct s_render_stats
{
	int		rays_traced;
	int		shadow_rays;
	int		hits;
	int		misses;
}	t_render_stats;

extern __thread t_render_stats	g_render_stats;

void	render_stats_reset(void);
void	render_stats_print(void);
