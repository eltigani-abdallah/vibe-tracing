#include "display.h"
#include "camera.h"
#include "geometry.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

// Structure interne pour SDL
struct SDLDisplay {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    DisplayConfig config;
    int should_quit;
};

// Initialisation de SDL et création de la fenêtre
SDLDisplay* display_create(DisplayConfig config) {
    SDLDisplay* display = malloc(sizeof(SDLDisplay));
    if (!display) {
        printf("❌ Erreur: Allocation mémoire pour l'affichage\n");
        return NULL;
    }
    
    display->config = config;
    display->should_quit = 0;
    
    // Initialisation SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("❌ Erreur SDL_Init: %s\n", SDL_GetError());
        free(display);
        return NULL;
    }
    
    // Création de la fenêtre
    Uint32 window_flags = SDL_WINDOW_SHOWN;
    if (config.fullscreen) {
        window_flags |= SDL_WINDOW_FULLSCREEN;
    }
    
    display->window = SDL_CreateWindow(
        config.title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        config.window_width,
        config.window_height,
        window_flags
    );
    
    if (!display->window) {
        printf("❌ Erreur création fenêtre: %s\n", SDL_GetError());
        SDL_Quit();
        free(display);
        return NULL;
    }
    
    // Création du renderer
    display->renderer = SDL_CreateRenderer(display->window, -1, 
                                         SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!display->renderer) {
        printf("❌ Erreur création renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(display->window);
        SDL_Quit();
        free(display);
        return NULL;
    }
    
    // Création de la texture pour le rendu
    display->texture = SDL_CreateTexture(display->renderer,
                                       SDL_PIXELFORMAT_RGB24,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       config.render_width,
                                       config.render_height);
    
    if (!display->texture) {
        printf("❌ Erreur création texture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(display->renderer);
        SDL_DestroyWindow(display->window);
        SDL_Quit();
        free(display);
        return NULL;
    }
    
    printf("✅ Affichage SDL initialisé: %dx%d\n", config.window_width, config.window_height);
    return display;
}

// Destruction de l'affichage
void display_destroy(SDLDisplay* display) {
    if (!display) return;
    
    if (display->texture) SDL_DestroyTexture(display->texture);
    if (display->renderer) SDL_DestroyRenderer(display->renderer);
    if (display->window) SDL_DestroyWindow(display->window);
    SDL_Quit();
    free(display);
}

// Affichage d'une image dans la fenêtre SDL
int display_show_image(SDLDisplay* display, Image* image) {
    if (!display || !image) return 0;
    
    // Conversion de l'image en format SDL
    int pitch;
    void* pixels;
    
    if (SDL_LockTexture(display->texture, NULL, &pixels, &pitch) != 0) {
        printf("❌ Erreur SDL_LockTexture: %s\n", SDL_GetError());
        return 0;
    }
    
    Uint8* pixel_buffer = (Uint8*)pixels;
    
    // Copie pixel par pixel avec correction gamma
    for (int y = 0; y < image->height; y++) {
        for (int x = 0; x < image->width; x++) {
            Color color = image_get_pixel(image, x, y);
            color = gamma_correct(color, 2.2);
            
            int index = y * pitch + x * 3;
            pixel_buffer[index + 0] = (Uint8)color_to_int(color, 0); // Rouge
            pixel_buffer[index + 1] = (Uint8)color_to_int(color, 1); // Vert  
            pixel_buffer[index + 2] = (Uint8)color_to_int(color, 2); // Bleu
        }
    }
    
    SDL_UnlockTexture(display->texture);
    return 1;
}

// Mise à jour de l'affichage
int display_update(SDLDisplay* display) {
    if (!display) return 0;
    
    SDL_SetRenderDrawColor(display->renderer, 0, 0, 0, 255);
    SDL_RenderClear(display->renderer);
    
    // Affichage de la texture avec mise à l'échelle
    SDL_Rect dst_rect = {
        0, 0,
        display->config.window_width,
        display->config.window_height
    };
    
    SDL_RenderCopy(display->renderer, display->texture, NULL, &dst_rect);
    SDL_RenderPresent(display->renderer);
    
    return 1;
}

// Gestion des événements SDL
int display_poll_events(SDLDisplay* display) {
    if (!display) return 0;
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                display->should_quit = 1;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                    case SDLK_q:
                        display->should_quit = 1;
                        break;
                    case SDLK_f:
                        // Toggle fullscreen
                        if (display->config.fullscreen) {
                            SDL_SetWindowFullscreen(display->window, 0);
                            display->config.fullscreen = 0;
                        } else {
                            SDL_SetWindowFullscreen(display->window, SDL_WINDOW_FULLSCREEN);
                            display->config.fullscreen = 1;
                        }
                        break;
                }
                break;
        }
    }
    
    return 1;
}

// Vérifier si on doit quitter
int display_should_quit(SDLDisplay* display) {
    return display ? display->should_quit : 1;
}

// Mode cinématique temps réel pour présentation
int display_cinematic_mode(SDLDisplay* display, Camera* camera, Scene* scene, RenderConfig* config) {
    if (!display || !camera || !scene || !config) return 0;
    
    printf("🎬 Démarrage du mode cinématique temps réel...\n");
    printf("💡 Contrôles: ESC/Q = Quitter, F = Plein écran\n\n");
    
    // Configuration pour rendu temps réel (équilibre qualité/vitesse)
    RenderConfig real_time_config = *config;
    real_time_config.width = display->config.render_width;
    real_time_config.height = display->config.render_height;
    real_time_config.samples_per_pixel = 12;  // Plus d'échantillons pour moins de bruit
    real_time_config.max_depth = 8;
    
    Image* image = image_create(real_time_config.width, real_time_config.height);
    if (!image) {
        printf("❌ Erreur: Impossible de créer l'image\n");
        return 0;
    }
    
    camera_set_mode(camera, CAMERA_CINEMATIC);
    
    Uint32 last_time = SDL_GetTicks();
    int frame_count = 0;
    
    while (!display_should_quit(display)) {
        Uint32 current_time = SDL_GetTicks();
        float delta_time = (current_time - last_time) / 1000.0f;
        last_time = current_time;
        
        // Mise à jour de la caméra
        camera_update(camera, delta_time);
        
        // Rendu de la frame
        render_image(camera, scene, &real_time_config, image);
        
        // Affichage
        display_show_image(display, image);
        display_update(display);
        
        // Gestion des événements
        display_poll_events(display);
        
        frame_count++;
        if (frame_count % 30 == 0) {
            float fps = 30.0f / ((SDL_GetTicks() - current_time + 30 * (1000.0f / 30.0f)) / 1000.0f);
            printf("\r🎬 Frame %d - FPS: %.1f", frame_count, fps);
            fflush(stdout);
        }
    }
    
    printf("\n✅ Mode cinématique terminé. %d frames rendues.\n", frame_count);
    image_free(image);
    return 1;
}

// Configurations prédéfinies
DisplayConfig display_config_presentation(void) {
    DisplayConfig config;
    config.window_width = 1024;
    config.window_height = 768;
    config.render_width = 1024;    // Résolution native - pas de mise à l'échelle
    config.render_height = 768;
    config.fullscreen = 0;
    config.title = "🌟 Spirited Away Ray Tracer - Sixième Station 🚊";
    return config;
}

DisplayConfig display_config_fullscreen(void) {
    DisplayConfig config;
    config.window_width = 1920;
    config.window_height = 1080;
    config.render_width = 1920;   // Résolution native HD
    config.render_height = 1080;
    config.fullscreen = 1;
    config.title = "🌟 Spirited Away Ray Tracer - Sixième Station 🚊";
    return config;
}