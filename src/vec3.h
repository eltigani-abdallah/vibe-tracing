#ifndef VEC3_H
#define VEC3_H

#include <math.h>
#include <stdio.h>

// ============================================================
//  Structure vec3 - Vecteur 3D
//  Utilisé pour : positions, directions, couleurs RGB
// ============================================================
typedef struct {
    double x, y, z;
} vec3;

// ============================================================
//  CONSTRUCTEURS
// ============================================================

// Créer un vecteur
static inline vec3 vec3_create(double x, double y, double z) {
    vec3 v = {x, y, z};
    return v;
}

// Vecteur zéro (0, 0, 0)
static inline vec3 vec3_zero(void) {
    vec3 v = {0.0, 0.0, 0.0};
    return v;
}

// Vecteur (1, 1, 1)
static inline vec3 vec3_one(void) {
    vec3 v = {1.0, 1.0, 1.0};
    return v;
}

// ============================================================
//  OPÉRATIONS DE BASE
// ============================================================

// Addition : a + b
static inline vec3 vec3_add(vec3 a, vec3 b) {
    vec3 r = {a.x + b.x, a.y + b.y, a.z + b.z};
    return r;
}

// Soustraction : a - b
static inline vec3 vec3_sub(vec3 a, vec3 b) {
    vec3 r = {a.x - b.x, a.y - b.y, a.z - b.z};
    return r;
}

// Négation : -v
static inline vec3 vec3_neg(vec3 v) {
    vec3 r = {-v.x, -v.y, -v.z};
    return r;
}

// Multiplication scalaire : v * t
static inline vec3 vec3_scale(vec3 v, double t) {
    vec3 r = {v.x * t, v.y * t, v.z * t};
    return r;
}

// Division scalaire : v / t
static inline vec3 vec3_div(vec3 v, double t) {
    vec3 r = {v.x / t, v.y / t, v.z / t};
    return r;
}

// Multiplication composante par composante : a * b (utile pour les couleurs)
static inline vec3 vec3_mul(vec3 a, vec3 b) {
    vec3 r = {a.x * b.x, a.y * b.y, a.z * b.z};
    return r;
}

// ============================================================
//  PRODUITS
// ============================================================

// Produit scalaire (dot product) : a · b
static inline double vec3_dot(vec3 a, vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Produit vectoriel (cross product) : a × b
static inline vec3 vec3_cross(vec3 a, vec3 b) {
    vec3 r = {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
    return r;
}

// ============================================================
//  LONGUEUR ET NORMALISATION
// ============================================================

// Longueur au carré : |v|² (évite sqrt quand possible)
static inline double vec3_length_sq(vec3 v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

// Longueur : |v|
static inline double vec3_length(vec3 v) {
    return sqrt(vec3_length_sq(v));
}

// Normalisation : v / |v| (vecteur unitaire)
static inline vec3 vec3_normalize(vec3 v) {
    double len = vec3_length(v);
    if (len < 1e-8) return vec3_zero(); // Éviter division par zéro
    return vec3_div(v, len);
}

// ============================================================
//  UTILITAIRES POUR LE RAYTRACER
// ============================================================

// Distance entre deux points
static inline double vec3_dist(vec3 a, vec3 b) {
    return vec3_length(vec3_sub(a, b));
}

// Interpolation linéaire : lerp(a, b, t) = a + t*(b-a)
static inline vec3 vec3_lerp(vec3 a, vec3 b, double t) {
    return vec3_add(vec3_scale(a, 1.0 - t), vec3_scale(b, t));
}

// Réflexion d'un rayon incident sur une normale : r = v - 2*(v·n)*n
static inline vec3 vec3_reflect(vec3 v, vec3 n) {
    return vec3_sub(v, vec3_scale(n, 2.0 * vec3_dot(v, n)));
}

// Clamp chaque composante entre min et max
static inline vec3 vec3_clamp(vec3 v, double min, double max) {
    vec3 r = {
        v.x < min ? min : (v.x > max ? max : v.x),
        v.y < min ? min : (v.y > max ? max : v.y),
        v.z < min ? min : (v.z > max ? max : v.z)
    };
    return r;
}

// Vérifie si un vecteur est proche de zéro (évite les dégénérescences)
static inline int vec3_near_zero(vec3 v) {
    const double eps = 1e-8;
    return (fabs(v.x) < eps) && (fabs(v.y) < eps) && (fabs(v.z) < eps);
}

// ============================================================
//  COULEURS (alias pour vec3, composantes RGB dans [0, 1])
// ============================================================

typedef vec3 color;

static inline color color_create(double r, double g, double b) {
    return vec3_create(r, g, b);
}

// Conversion couleur [0,1] → entier [0,255] pour SDL
static inline int color_to_byte(double c) {
    if (c < 0.0) c = 0.0;
    if (c > 1.0) c = 1.0;
    return (int)(255.0 * c);
}

// Conversion couleur vec3 → pixel ARGB (format SDL ARGB8888)
static inline unsigned int color_to_argb(color c) {
    int r = color_to_byte(c.x);
    int g = color_to_byte(c.y);
    int b = color_to_byte(c.z);
    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}

// Tone mapping ACES filmic (approx. S-curve) + gamma 2.2
// Transforme les valeurs HDR en [0,1] avec un rendu "cinématique"
static inline double aces_channel(double x) {
    double a = 2.51, b = 0.03, c2 = 2.43, d = 0.59, e = 0.14;
    double t = (x * (a * x + b)) / (x * (c2 * x + d) + e);
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return t;
}
static inline unsigned int color_tonemap_argb(color col) {
    /* Exposition légère : +20% pour  des couleurs game plus vives */
    col.x *= 1.20;  col.y *= 1.20;  col.z *= 1.20;
    /* ACES filmic tone map */
    col.x = aces_channel(col.x);
    col.y = aces_channel(col.y);
    col.z = aces_channel(col.z);
    /* Gamma 2.2 */
    double inv_g = 1.0 / 2.2;
    col.x = (col.x > 0.0) ? pow(col.x, inv_g) : 0.0;
    col.y = (col.y > 0.0) ? pow(col.y, inv_g) : 0.0;
    col.z = (col.z > 0.0) ? pow(col.z, inv_g) : 0.0;
    int r = color_to_byte(col.x);
    int g = color_to_byte(col.y);
    int b = color_to_byte(col.z);
    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}

// ============================================================
//  DEBUG
// ============================================================

// Afficher un vec3
static inline void vec3_print(const char* label, vec3 v) {
    printf("%s: (%.4f, %.4f, %.4f)\n", label, v.x, v.y, v.z);
}

#endif // VEC3_H