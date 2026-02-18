#include "optim.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

__thread t_render_stats	g_render_stats = {0};

t_mem_pool	*mem_pool_create(size_t capacity)
{
	t_mem_pool	*pool;

	pool = (t_mem_pool *)malloc(sizeof(*pool));
	if (!pool)
		return (NULL);
	pool->buffer = (char *)malloc(capacity);
	if (!pool->buffer)
	{
		free(pool);
		return (NULL);
	}
	pool->capacity = capacity;
	pool->used = 0;
	return (pool);
}

void mem_pool_reset(t_mem_pool *pool)
{
	if (pool)
		pool->used = 0;
}

void mem_pool_destroy(t_mem_pool *pool)
{
	if (!pool)
		return ;
	if (pool->buffer)
		free(pool->buffer);
	free(pool);
}

void *mem_pool_alloc(t_mem_pool *pool, size_t size)
{
	void	*ptr;

	if (!pool || pool->used + size > pool->capacity)
		return (NULL);
	ptr = pool->buffer + pool->used;
	pool->used += size;
	return (ptr);
}

void render_stats_reset(void)
{
	memset(&g_render_stats, 0, sizeof(g_render_stats));
}

void render_stats_print(void)
{
	fprintf(stderr, "[STATS] Rays: %d, Shadow: %d, Hits: %d, Misses: %d\n",
		g_render_stats.rays_traced, g_render_stats.shadow_rays,
		g_render_stats.hits, g_render_stats.misses);
}
