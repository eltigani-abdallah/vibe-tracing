#include "renderer.h"
#include "material.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// Création d'une image
Image* image_create(int width, int height) {
    Image* image = malloc(sizeof(Image));
    if (!image) return NULL;
    
    image->width = width;
    image->height = height;
    image->pixels = malloc(sizeof(Color) * width * height);
    
    if (!image->pixels) {
        free(image);
        return NULL;
    }
    
    // Initialiser avec du noir
    for (int i = 0; i < width * height; i++) {
        image->pixels[i] = color_black();
    }
    
    return image;
}

// Libération d'une image
void image_free(Image* image) {
    if (image) {
        if (image->pixels) free(image->pixels);
        free(image);
    }
}

// Définir un pixel
void image_set_pixel(Image* image, int x, int y, Color color) {
    if (x < 0 || x >= image->width || y < 0 || y >= image->height) return;
    image->pixels[y * image->width + x] = color;
}

// Obtenir un pixel
Color image_get_pixel(Image* image, int x, int y) {
    if (x < 0 || x >= image->width || y < 0 || y >= image->height) return color_black();
    return image->pixels[y * image->width + x];
}

// Sauvegarde au format PPM
int image_save_ppm(Image* image, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) return 0;
    
    fprintf(file, "P3\n%d %d\n255\n", image->width, image->height);
    
    for (int y = 0; y < image->height; y++) {
        for (int x = 0; x < image->width; x++) {
            Color pixel = image_get_pixel(image, x, y);
            pixel = gamma_correct(pixel, 2.2); // Correction gamma
            
            int r = color_to_int(pixel, 0);
            int g = color_to_int(pixel, 1);
            int b = color_to_int(pixel, 2);
            
            fprintf(file, "%d %d %d ", r, g, b);
        }
        fprintf(file, "\n");
    }
    
    fclose(file);
    return 1;
}

// Fonction utilitaire de clamp
double clamp(double value, double min, double max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Correction gamma
Color gamma_correct(Color color, double gamma) {
    return color_create(
        pow(clamp(color.x, 0.0, 1.0), 1.0 / gamma),
        pow(clamp(color.y, 0.0, 1.0), 1.0 / gamma),
        pow(clamp(color.z, 0.0, 1.0), 1.0 / gamma)
    );
}

// Calcul de la couleur d'un rayon (fonction récursive principale)
Color ray_color(Ray ray, Scene* scene, int depth, Color background) {
    // Condition d'arrêt
    if (depth <= 0) {
        return color_black();
    }
    
    HitRecord hit;
    if (scene_hit(scene, ray, 0.001, INFINITY, &hit)) {
        // Si on a touché un objet
        Color emission = color_black();
        
        // Vérifier si le matériau émet de la lumière
        if (hit.material->type == MATERIAL_EMISSIVE) {
            emission = color_mul(hit.material->emission, hit.material->emission_strength);
        }
        
        // Calculer le scattering
        ScatterResult scatter = material_scatter(hit.material, ray, &hit);
        
        if (scatter.scattered_ray) {
            Color scattered_color = ray_color(scatter.scattered, scene, depth - 1, background);
            Color result = color_add(emission, 
                                   color_create(
                                       scatter.attenuation.x * scattered_color.x,
                                       scatter.attenuation.y * scattered_color.y,
                                       scatter.attenuation.z * scattered_color.z
                                   ));
            return result;
        } else {
            // Le rayon a été absorbé, retourner seulement l'émission
            return emission;
        }
    } else {
        // Pas d'intersection, retourner l'arrière-plan
        return background;
    }
}

// Échantillonnage d'un pixel avec anti-aliasing
Color sample_pixel(Camera* camera, Scene* scene, RenderConfig* config, int x, int y) {
    Color color = color_black();
    
    for (int s = 0; s < config->samples_per_pixel; s++) {
        // Jitter aléatoire pour l'anti-aliasing
        double u = ((double)x + ((double)rand() / RAND_MAX)) / (double)config->width;
        double v = ((double)y + ((double)rand() / RAND_MAX)) / (double)config->height;
        
        // Inverser v car l'image PPM commence par le haut
        v = 1.0 - v;
        
        Ray ray = camera_get_ray(camera, u, v);
        Color sample_color = ray_color(ray, scene, config->max_depth, config->background);
        
        color = color_add(color, sample_color);
    }
    
    // Moyenne des échantillons
    color = color_mul(color, 1.0 / (double)config->samples_per_pixel);
    
    return color;
}

// Rendu d'une image complète
void render_image(Camera* camera, Scene* scene, RenderConfig* config, Image* image) {
    render_image_with_progress(camera, scene, config, image, NULL, NULL);
}

// Rendu avec callback de progrès
void render_image_with_progress(Camera* camera, Scene* scene, RenderConfig* config, 
                               Image* image, ProgressCallback callback, void* user_data) {
    int total_pixels = config->width * config->height;
    int completed_pixels = 0;
    
    for (int y = 0; y < config->height; y++) {
        for (int x = 0; x < config->width; x++) {
            Color pixel_color = sample_pixel(camera, scene, config, x, y);
            image_set_pixel(image, x, y, pixel_color);
            
            completed_pixels++;
            if (callback) {
                callback(completed_pixels, total_pixels, user_data);
            }
        }
    }
}

// Configurations prédéfinies
RenderConfig render_config_preview(void) {
    RenderConfig config;
    config.width = 512;   // Résolution réduite pour aperçu
    config.height = 384;
    config.samples_per_pixel = 15;  // Plus d'échantillons
    config.max_depth = 8;
    config.background = color_create(0.3, 0.3, 0.4); // Ciel brumeux visible
    return config;
}

RenderConfig render_config_quality(void) {
    RenderConfig config;
    config.width = 800;
    config.height = 600;
    config.samples_per_pixel = 50;
    config.max_depth = 10;
    config.background = color_create(0.3, 0.3, 0.4);
    return config;
}

RenderConfig render_config_production(void) {
    RenderConfig config;
    config.width = 1920;
    config.height = 1080;
    config.samples_per_pixel = 200;
    config.max_depth = 20;
    config.background = color_create(0.3, 0.3, 0.4);
    return config;
}