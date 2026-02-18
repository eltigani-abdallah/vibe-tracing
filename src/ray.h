#ifndef RAY_H
#define RAY_H

// ============================================================
//  ray.h  —  Structure Ray et opérations associées
//
//  Un rayon est défini par :
//      P(t) = origin + t * direction
//
//  La direction est toujours stockée normalisée.
// ============================================================

#include "vec3.h"

// ------------------------------------------------------------
//  Structure
// ------------------------------------------------------------
typedef struct {
    vec3 origin;     // point de départ du rayon
    vec3 direction;  // direction unitaire (normalisée)
} Ray;

// ------------------------------------------------------------
//  Constructeur
//  La direction est normalisée automatiquement.
// ------------------------------------------------------------
static inline Ray ray_create(vec3 origin, vec3 direction) {
    Ray r;
    r.origin    = origin;
    r.direction = vec3_normalize(direction);
    return r;
}

// ------------------------------------------------------------
//  Point sur le rayon au paramètre t
//      P(t) = origin + t * direction
//  t > 0 → devant l'œil
//  t < 0 → derrière l'œil (ignoré en pratique)
// ------------------------------------------------------------
static inline vec3 ray_at(Ray r, double t) {
    return vec3_add(r.origin, vec3_scale(r.direction, t));
}

// ------------------------------------------------------------
//  Debug
// ------------------------------------------------------------
static inline void ray_print(const char* label, Ray r) {
    printf("%s  origin=(%.4f,%.4f,%.4f)  dir=(%.4f,%.4f,%.4f)\n",
           label,
           r.origin.x,    r.origin.y,    r.origin.z,
           r.direction.x, r.direction.y, r.direction.z);
}

#endif /* RAY_H */
