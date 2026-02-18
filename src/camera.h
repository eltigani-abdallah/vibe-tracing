#ifndef CAMERA_H
#define CAMERA_H

// ============================================================
//  camera.h  —  Caméra sténopé (pinhole)
//
//  Modèle mathématique :
//
//    1. On construit une base orthonormée (u, v, w) à partir de
//       lookfrom, lookat et vup.
//
//    2. On calcule la fenêtre de visualisation (viewport) en
//       fonction du champ de vision vertical (vfov_deg).
//
//    3. ray_for_pixel(cam, px, py) renvoie le rayon partant de
//       l'œil et traversant le centre du pixel (px, py).
//
//
//  Système de coordonnées écran :
//       (0,0) = coin supérieur gauche
//       x → droite,  y ↓ bas
//
//  L'axe Y est renversé lors de la conversion en coordonnées
//  viewport (où (0,0) est en bas à gauche).
// ============================================================

#include "vec3.h"
#include "ray.h"

// ------------------------------------------------------------
//  Structure Camera
// ------------------------------------------------------------
typedef struct {
    vec3 origin;      // position de l'œil (lookfrom)

    // Coin inférieur gauche du viewport dans l'espace 3D
    vec3 lower_left;

    // Vecteurs couvrant toute la largeur / hauteur du viewport
    vec3 horizontal;  // pointe vers la droite, longueur = largeur viewport
    vec3 vertical;    // pointe vers le haut,   longueur = hauteur viewport

    // Base orthonormée de la caméra
    vec3 u;  // droite
    vec3 v;  // haut (recalculé, pas forcément == vup)
    vec3 w;  // opposé à la direction de visée

    // Dimensions de l'image en pixels
    int img_width;
    int img_height;
} Camera;

// ------------------------------------------------------------
//  Fonctions (implémentées dans camera.c)
// ------------------------------------------------------------

/**
 * Crée et initialise une caméra.
 *
 * @param lookfrom   Position de l'œil
 * @param lookat     Point visé
 * @param vup        Vecteur "haut" de référence (souvent Y+)
 * @param vfov_deg   Champ de vision vertical en degrés
 * @param img_width  Largeur de l'image en pixels
 * @param img_height Hauteur de l'image en pixels
 */
Camera camera_create(vec3 lookfrom, vec3 lookat, vec3 vup,
                     double vfov_deg, int img_width, int img_height);

/**
 * Renvoie le rayon partant de l'œil et passant par le centre
 * du pixel (px, py).
 *
 * @param cam  Caméra initialisée
 * @param px   Colonne du pixel  [0, img_width - 1]
 * @param py   Ligne   du pixel  [0, img_height - 1]  (0 = haut)
 * @return     Rayon normalisé depuis l'œil
 */
Ray ray_for_pixel(const Camera* cam, int px, int py);

/**
 * Rayon pour coordonnées sous-pixel réelles (anti-aliasing MSAA).
 * @param fx, fy  Coordonnées flottantes en pixels (ex: 3.25, 7.75)
 */
Ray ray_for_pixel_sub(const Camera* cam, double fx, double fy);

#endif /* CAMERA_H */
