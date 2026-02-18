#include "material.h"
#include <stdlib.h>
#include <math.h>

// Fonction utilitaire pour générer un vecteur aléatoire dans une sphère unitaire
static Vec3 random_in_unit_sphere(void) {
    Vec3 p;
    do {
        p = vec3_create(
            2.0 * ((double)rand() / RAND_MAX) - 1.0,
            2.0 * ((double)rand() / RAND_MAX) - 1.0,
            2.0 * ((double)rand() / RAND_MAX) - 1.0
        );
    } while (vec3_length_squared(p) >= 1.0);
    return p;
}

// Fonction utilitaire pour la réflexion de Schlick approximation
static double schlick_reflectance(double cosine, double ref_idx) {
    double r0 = (1 - ref_idx) / (1 + ref_idx);
    r0 = r0 * r0;
    return r0 + (1 - r0) * pow((1 - cosine), 5);
}

// Création des matériaux
Material material_lambertian(Color albedo) {
    Material mat;
    mat.type = MATERIAL_LAMBERTIAN;
    mat.albedo = albedo;
    mat.roughness = 1.0;
    mat.refraction_index = 1.0;
    mat.emission = color_black();
    mat.emission_strength = 0.0;
    return mat;
}

Material material_metal(Color albedo, double roughness) {
    Material mat;
    mat.type = MATERIAL_METAL;
    mat.albedo = albedo;
    mat.roughness = roughness < 0 ? 0 : (roughness > 1 ? 1 : roughness);
    mat.refraction_index = 1.0;
    mat.emission = color_black();
    mat.emission_strength = 0.0;
    return mat;
}

Material material_dielectric(double refraction_index) {
    Material mat;
    mat.type = MATERIAL_DIELECTRIC;
    mat.albedo = color_white();
    mat.roughness = 0.0;
    mat.refraction_index = refraction_index;
    mat.emission = color_black();
    mat.emission_strength = 0.0;
    return mat;
}

Material material_emissive(Color emission, double strength) {
    Material mat;
    mat.type = MATERIAL_EMISSIVE;
    mat.albedo = emission;
    mat.roughness = 0.0;
    mat.refraction_index = 1.0;
    mat.emission = emission;
    mat.emission_strength = strength;
    return mat;
}

Material material_water(Color tint, double roughness) {
    Material mat;
    mat.type = MATERIAL_WATER;
    mat.albedo = tint;
    mat.roughness = roughness;
    mat.refraction_index = 1.33; // Indice de réfraction de l'eau
    mat.emission = color_black();
    mat.emission_strength = 0.0;
    return mat;
}

// Fonction principale de scattering
ScatterResult material_scatter(Material* material, Ray ray_in, HitRecord* hit) {
    ScatterResult result;
    result.scattered_ray = 0;
    result.attenuation = color_black();
    
    switch (material->type) {
        case MATERIAL_LAMBERTIAN: {
            Vec3 scatter_direction = vec3_add(hit->normal, vec3_normalize(random_in_unit_sphere()));
            
            // Éviter les directions très petites qui causent des problèmes numériques
            if (vec3_length_squared(scatter_direction) < 1e-8) {
                scatter_direction = hit->normal;
            }
            
            result.scattered = ray_create(hit->point, scatter_direction);
            result.attenuation = material->albedo;
            result.scattered_ray = 1;
            break;
        }
        
        case MATERIAL_METAL: {
            Vec3 reflected = vec3_reflect(vec3_normalize(ray_in.direction), hit->normal);
            
            // Ajouter de la rugosité si nécessaire
            if (material->roughness > 0.0) {
                Vec3 fuzz = vec3_mul(random_in_unit_sphere(), material->roughness);
                reflected = vec3_add(reflected, fuzz);
            }
            
            result.scattered = ray_create(hit->point, reflected);
            result.attenuation = material->albedo;
            result.scattered_ray = vec3_dot(reflected, hit->normal) > 0;
            break;
        }
        
        case MATERIAL_DIELECTRIC: {
            result.attenuation = color_white();
            double refraction_ratio = hit->front_face ? 
                (1.0 / material->refraction_index) : material->refraction_index;
            
            Vec3 unit_direction = vec3_normalize(ray_in.direction);
            double cos_theta = fmin(vec3_dot(vec3_neg(unit_direction), hit->normal), 1.0);
            double sin_theta = sqrt(1.0 - cos_theta * cos_theta);
            
            int cannot_refract = refraction_ratio * sin_theta > 1.0;
            Vec3 direction;
            
            if (cannot_refract || schlick_reflectance(cos_theta, refraction_ratio) > ((double)rand() / RAND_MAX)) {
                // Réflexion
                direction = vec3_reflect(unit_direction, hit->normal);
            } else {
                // Réfraction (formule de Snell)
                Vec3 r_out_perp = vec3_mul(vec3_add(unit_direction, vec3_mul(hit->normal, cos_theta)), refraction_ratio);
                Vec3 r_out_parallel = vec3_mul(hit->normal, -sqrt(fabs(1.0 - vec3_length_squared(r_out_perp))));
                direction = vec3_add(r_out_perp, r_out_parallel);
            }
            
            result.scattered = ray_create(hit->point, direction);
            result.scattered_ray = 1;
            break;
        }
        
        case MATERIAL_WATER: {
            // Combinaison de réflexion et de réfraction pour l'eau
            double refraction_ratio = hit->front_face ? (1.0 / material->refraction_index) : material->refraction_index;
            Vec3 unit_direction = vec3_normalize(ray_in.direction);
            double cos_theta = fmin(vec3_dot(vec3_neg(unit_direction), hit->normal), 1.0);
            
            // L'eau a plus de chance de réfléchir à des angles rasants
            double reflect_prob = schlick_reflectance(cos_theta, material->refraction_index);
            
            if (((double)rand() / RAND_MAX) < reflect_prob) {
                // Réflexion avec un peu de rugosité
                Vec3 reflected = vec3_reflect(unit_direction, hit->normal);
                if (material->roughness > 0.0) {
                    Vec3 fuzz = vec3_mul(random_in_unit_sphere(), material->roughness * 0.1);
                    reflected = vec3_add(reflected, fuzz);
                }
                result.scattered = ray_create(hit->point, reflected);
            } else {
                // Réfraction
                double sin_theta = sqrt(1.0 - cos_theta * cos_theta);
                if (refraction_ratio * sin_theta <= 1.0) {
                    Vec3 r_out_perp = vec3_mul(vec3_add(unit_direction, vec3_mul(hit->normal, cos_theta)), refraction_ratio);
                    Vec3 r_out_parallel = vec3_mul(hit->normal, -sqrt(fabs(1.0 - vec3_length_squared(r_out_perp))));
                    Vec3 direction = vec3_add(r_out_perp, r_out_parallel);
                    result.scattered = ray_create(hit->point, direction);
                } else {
                    // Réflexion totale interne
                    Vec3 reflected = vec3_reflect(unit_direction, hit->normal);
                    result.scattered = ray_create(hit->point, reflected);
                }
            }
            
            result.attenuation = material->albedo;
            result.scattered_ray = 1;
            break;
        }
        
        case MATERIAL_EMISSIVE: {
            // Les matériaux émissifs n'émettent pas de rayons scattered
            result.scattered_ray = 0;
            result.attenuation = material->emission;
            break;
        }
    }
    
    return result;
}

// Matériaux prédéfinis pour la scène de Chihiro
Material chihiro_train_body(void) {
    // Corps métallique du train - couleur plus lumineuse
    return material_metal(color_create(0.5, 0.6, 0.3), 0.3);
}

Material chihiro_train_window(void) {
    // Fenêtres légèrement teintées
    return material_dielectric(1.5);
}

Material chihiro_station_floor(void) {
    // Sol en béton grisâtre - plus lumineux
    return material_lambertian(color_create(0.6, 0.6, 0.65));
}

Material chihiro_station_walls(void) {
    // Murs de la gare - tons plus lumineux
    return material_lambertian(color_create(0.8, 0.7, 0.6));
}

Material chihiro_water(void) {
    // Eau sombre avec reflections
    return material_water(color_create(0.1, 0.2, 0.3), 0.05);
}

Material chihiro_lamplight(void) {
    // Lumière chaude des lampadaires - plus intense
    return material_emissive(color_create(1.0, 0.8, 0.4), 8.0);
}

Material chihiro_sky(void) {
    // Ciel brumeux nocturne
    return material_lambertian(color_create(0.3, 0.3, 0.4));
}