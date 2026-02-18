#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* stb_image : chargement PNG/JPG header-only */
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "vec3.h"
#include "ray.h"
#include "camera.h"
#include "geometry.h"

/* --- Déclarations anticipées --- */
static double smoothstep(double edge0, double edge1, double x);
static double g_time = 0.0;  /* horloge globale pour les ripples */

// ============================================================
//  Système de textures (conservé pour usage futur)
// ============================================================
typedef struct {
    unsigned char *data;
    int w, h, ch;
} Texture;

/* Texture bois procédurale (poteaux uniquement) */
static color wood_texture(double u, double v) {
    double grain = sin((v * 40.0) + sin(u * 8.0) * 2.0 + cos(v * 3.7) * 1.5) * 0.5 + 0.5;
    double plank  = fmod(u * 5.0, 1.0);
    double edge   = (plank < 0.04 || plank > 0.96) ? 0.6 : 1.0;
    double knot_u = fmod(u * 1.7 + 0.3, 1.0);
    double knot_v = fmod(v * 2.3 + 0.7, 1.0);
    double knot   = 1.0 - smoothstep(0.0, 0.08,
                    sqrt((knot_u-0.5)*(knot_u-0.5)+(knot_v-0.5)*(knot_v-0.5)));
    knot = knot * knot * 0.4;
    color base = vec3_create(0.62 + grain*0.15, 0.42 + grain*0.08, 0.22 + grain*0.04);
    base = vec3_scale(base, edge);
    base.x -= knot * 0.25;  base.y -= knot * 0.15;  base.z -= knot * 0.05;
    return vec3_clamp(base, 0.05, 1.0);
}

// ============================================================
//  Texture rails Ghibli — traverses en bois + rails métalliques
//  Appelée depuis ray_color pour la Box (quai → voie ferrée)
// ============================================================
static color rail_texture(double wx, double wz) {
    /* wx = position monde X, wz = position monde Z
       On veut des traverses perpendiculaires = bandes en Z,
       et deux rails parallèles en X ≈ ±1.1 (espace entre les poteaux) */

    /* --- Traverses : bandes de bois sombres tous les 0.8 unités --- */
    double tie_period = 0.80;
    double tie_phase  = fmod(fabs(wz), tie_period) / tie_period;
    /* tie_phase ∈ [0,1] ; bande bois si phase < 0.45 */
    double is_tie     = (tie_phase < 0.45) ? 1.0 : 0.0;
    /* adoucissement du bord traverses (cel-shading : bord net) */
    double tie_edge   = smoothstep(0.40, 0.45, tie_phase);
    is_tie = is_tie * (1.0 - tie_edge);

    /* Variation grain de bois traverses */
    double grain = sin(wz * 18.0 + sin(wx * 4.0) * 1.2) * 0.5 + 0.5;
    color  c_tie = vec3_create(0.32 + grain*0.10, 0.22 + grain*0.06, 0.12 + grain*0.03);
    /* Ballast (caillou gris-bleuté) entre les traverses */
    double bal_noise = sin(wx * 7.3 + wz * 11.7) * 0.5 + 0.5;
    double bal2      = sin(wx * 3.1 - wz * 5.9)  * 0.5 + 0.5;
    double bal_v     = (bal_noise + bal2) * 0.5;
    color  c_ballast = vec3_create(0.38 + bal_v * 0.12,
                                   0.36 + bal_v * 0.10,
                                   0.42 + bal_v * 0.08);

    /* --- Rails : deux bandes métalliques en X --- */
    /* Rail gauche centré en wx ≈ -1.14, rail droit ≈ +1.14 */
    double rail_hw  = 0.09;   /* demi-largeur rail */
    double left_d   = fabs(wx + 1.14);
    double right_d  = fabs(wx - 1.14);
    double on_rail  = (left_d < rail_hw || right_d < rail_hw) ? 1.0 : 0.0;
    /* Reflet métallique côté illuminé */
    double rail_x   = (left_d < rail_hw) ? (wx + 1.14) : (wx - 1.14);
    double highlight = smoothstep(0.03, 0.0, fabs(rail_x - rail_hw * 0.35));
    color  c_rail = vec3_create(0.58 + highlight * 0.35,
                                0.58 + highlight * 0.32,
                                0.62 + highlight * 0.25);
    /* Rouille légère sous rail */
    double rust = sin(wz * 5.7 + wx * 2.2) * 0.5 + 0.5;
    c_rail.x += rust * 0.06;
    c_rail.y -= rust * 0.02;

    /* --- Composition finale --- */
    color base;
    if (on_rail > 0.0) {
        base = c_rail;
    } else if (is_tie > 0.5) {
        base = c_tie;
    } else {
        /* Transition nette traverses/ballast (cel) */
        double blend = smoothstep(0.3, 0.7, is_tie);
        base = vec3_lerp(c_ballast, c_tie, blend);
    }
    return vec3_clamp(base, 0.0, 1.0);
}

    /* Texture eau : perturbation des normales (ripples) */
static vec3 water_normal(double wx, double wz, double time) {
    /* Superposition de 3 ondes sinusoïdales */
    double h = sin(wx * 2.1 + time * 1.3) * 0.06
             + sin(wz * 1.7 + time * 1.0) * 0.05
             + sin((wx + wz) * 3.2 + time * 2.1) * 0.025;
    double eps = 0.01;
    double hx  = sin((wx+eps)*2.1 + time*1.3)*0.06 + sin(wz*1.7+time*1.0)*0.05
               + sin(((wx+eps)+wz)*3.2+time*2.1)*0.025;
    double hz  = sin(wx*2.1+time*1.3)*0.06 + sin((wz+eps)*1.7+time*1.0)*0.05
               + sin((wx+(wz+eps))*3.2+time*2.1)*0.025;
    (void)h;
    return vec3_normalize(vec3_create(-(hx - h) / eps,
                                      1.0,
                                      -(hz - h) / eps));
}

// ============================================================
//  Constantes fenêtre
// ============================================================
#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720
#define WINDOW_TITLE  "Chihiro — Sixième Station"

// ============================================================
//  Constantes de rendu
// ============================================================
#define T_MIN          1e-4
#define T_MAX          1e10
#define MAX_BOUNCES    4      /* profondeur max récursion */
#define AA_SAMPLES     4      /* anti-aliasing : 4 rayons par pixel (grille 2×2) */

// Plan de l'eau
#define WATER_LEVEL    0.0

// Réflectivité de l'eau : 0=opaque, 1=miroir parfait
// On garde 1.0 (miroir parfait) comme demandé
#define WATER_REFLECT  1.0

// ============================================================
//  Scène Chihiro — Sixième Station
//
//  Objets :
//    - Plan  y=0           : surface de l'eau (miroir Fresnel)
//    - Sphères             : lanternes en suspens
//    - Cylindres verticaux : poteaux du quai tous les 20u en Z
//    - Box (AABB)          : quai de béton à fleur d'eau
// ============================================================

/* ---------- Sphères (lanternes + lune) ---------- */
#define N_SPHERES  5

typedef struct {
    vec3   center;
    double radius;
    color  col;
    double emit;
} Sphere;

static Sphere g_spheres[N_SPHERES];

/* ---------- Cylindres (poteaux) ---------- */
/* 2 rangées (x = ±POLE_X) × N_POLE_Z positions = 2*N_POLE_Z cylindres */
#define POLE_X        3.2
#define POLE_RADIUS   0.22
#define POLE_HEIGHT   4.5
#define POLE_SPACING  10.0   /* espacement serré pour look défilement */
#define POLE_Z_START  5.0
#define N_POLE_Z      9
#define N_CYLINDERS   (2 * N_POLE_Z)

typedef struct {
    double cx, cz;    /* base au centre, au niveau y=0  */
    double y_min;
    double y_max;
    double radius;
    color  col;
} Cylinder;

static Cylinder g_cylinders[N_CYLINDERS];

/* ---------- Box (quai + train) ---------- */
typedef struct {
    vec3  bmin, bmax;
    color col;
    double ghost;   /* 0=solide, 1=fantôme translucide */
} Box;

#define N_BOXES_QUAI  1
/* Train : 1 locomotive (3 box) + 3 wagons (2 box chacun) + détails = 16 box max */
#define N_BOXES_TRAIN 16
#define N_BOXES       (N_BOXES_QUAI + N_BOXES_TRAIN)
static Box g_boxes[N_BOXES];

/* Position Z de la tête du train (avance avec le temps) */
static double g_train_z = 30.0;   /* démarre loin, arrive vers cam */

/* -------------------------------------------------- */
static void build_scene(void) {

    /* --- Lanternes --- */
    /* Lanterne centrale proche */
    g_spheres[0].center = vec3_create( 0.0, 2.2,  6.0);
    g_spheres[0].radius = 0.46;
    g_spheres[0].col    = vec3_create(1.00, 0.50, 0.08);
    g_spheres[0].emit   = 3.0;

    /* Lanternes latérales proches */
    g_spheres[1].center = vec3_create(-3.2, 2.0,  7.0);
    g_spheres[1].radius = 0.36;
    g_spheres[1].col    = vec3_create(1.00, 0.60, 0.12);
    g_spheres[1].emit   = 2.4;

    g_spheres[2].center = vec3_create( 3.2, 2.0,  7.0);
    g_spheres[2].radius = 0.36;
    g_spheres[2].col    = vec3_create(1.00, 0.60, 0.12);
    g_spheres[2].emit   = 2.4;

    /* Lune / soleil couchant  */
    g_spheres[3].center = vec3_create( 5.0, 6.5, 40.0);
    g_spheres[3].radius = 2.6;
    g_spheres[3].col    = vec3_create(1.00, 0.92, 0.65);
    g_spheres[3].emit   = 5.5;

    /* Lanterne lointaine */
    g_spheres[4].center = vec3_create(-1.2, 1.9, 18.0);
    g_spheres[4].radius = 0.28;
    g_spheres[4].col    = vec3_create(1.00, 0.55, 0.10);
    g_spheres[4].emit   = 2.0;

    /* --- Poteaux --- */
    color col_pole = vec3_create(0.40, 0.30, 0.18);
    for (int k = 0; k < N_POLE_Z; k++) {
        double z = POLE_Z_START + k * POLE_SPACING;
        g_cylinders[2*k  ].cx = -POLE_X; g_cylinders[2*k  ].cz = z;
        g_cylinders[2*k  ].y_min = -0.12; g_cylinders[2*k  ].y_max = POLE_HEIGHT;
        g_cylinders[2*k  ].radius = POLE_RADIUS; g_cylinders[2*k  ].col = col_pole;
        g_cylinders[2*k+1].cx =  POLE_X; g_cylinders[2*k+1].cz = z;
        g_cylinders[2*k+1].y_min = -0.12; g_cylinders[2*k+1].y_max = POLE_HEIGHT;
        g_cylinders[2*k+1].radius = POLE_RADIUS; g_cylinders[2*k+1].col = col_pole;
    }

    /* --- Quai large --- */
    g_boxes[0].bmin  = vec3_create(-3.8, -0.15, 2.0);
    g_boxes[0].bmax  = vec3_create( 3.8,  0.08, 100.0);
    g_boxes[0].col   = vec3_create(0.58, 0.40, 0.22);
    g_boxes[0].ghost = 0.0;

    /* Le train est reconstruit chaque frame via update_train() */
}

// ============================================================
//  update_train — construit le train à partir de g_train_z
//  Appelé depuis update_pixels après avoir mis à jour g_train_z
// ============================================================
static void update_train(void) {
    double z0 = g_train_z;   /* nez de la locomotive */

    /* Couleur fantôme Ghibli : bleu-blanc translucide */
    color gc  = vec3_create(0.72, 0.88, 1.00);
    color gc2 = vec3_create(0.55, 0.70, 0.90);
    color gc3 = vec3_create(0.20, 0.35, 0.60);   /* vitre sombre */
    color gc4 = vec3_create(1.00, 0.92, 0.60);   /* phare chaud  */

    int b = N_BOXES_QUAI;   /* index de départ dans g_boxes */

    /* === LOCOMOTIVE === */
    /* Corps principal : large + haut */
    g_boxes[b].bmin  = vec3_create(-1.05, 0.05, z0);
    g_boxes[b].bmax  = vec3_create( 1.05, 2.20, z0 + 5.5);
    g_boxes[b].col   = gc;
    g_boxes[b].ghost = 0.72;
    b++;

    /* Cheminée (box fine) */
    g_boxes[b].bmin  = vec3_create(-0.20, 2.20, z0 + 1.0);
    g_boxes[b].bmax  = vec3_create( 0.20, 3.20, z0 + 1.6);
    g_boxes[b].col   = gc2;
    g_boxes[b].ghost = 0.72;
    b++;

    /* Cabine conducteur (dessus arrière) */
    g_boxes[b].bmin  = vec3_create(-0.90, 2.20, z0 + 2.8);
    g_boxes[b].bmax  = vec3_create( 0.90, 3.30, z0 + 5.5);
    g_boxes[b].col   = gc2;
    g_boxes[b].ghost = 0.68;
    b++;

    /* Pare-brise avant (box mince, sombre) */
    g_boxes[b].bmin  = vec3_create(-0.88, 1.20, z0 - 0.05);
    g_boxes[b].bmax  = vec3_create( 0.88, 2.10, z0 + 0.08);
    g_boxes[b].col   = gc3;
    g_boxes[b].ghost = 0.85;
    b++;

    /* Phare gauche */
    g_boxes[b].bmin  = vec3_create(-0.88, 0.70, z0 - 0.06);
    g_boxes[b].bmax  = vec3_create(-0.52, 1.05, z0 + 0.06);
    g_boxes[b].col   = gc4;
    g_boxes[b].ghost = 0.0;   /* phare = émissif opaque */
    b++;

    /* Phare droit */
    g_boxes[b].bmin  = vec3_create( 0.52, 0.70, z0 - 0.06);
    g_boxes[b].bmax  = vec3_create( 0.88, 1.05, z0 + 0.06);
    g_boxes[b].col   = gc4;
    g_boxes[b].ghost = 0.0;
    b++;

    /* === WAGON 1 === */
    double w1 = z0 + 6.5;
    g_boxes[b].bmin  = vec3_create(-1.00, 0.05, w1);
    g_boxes[b].bmax  = vec3_create( 1.00, 2.00, w1 + 7.0);
    g_boxes[b].col   = gc;
    g_boxes[b].ghost = 0.78;
    b++;

    /* Fenêtres wagon 1 — rangée gauche (3 fenêtres) */
    for (int fw = 0; fw < 3 && b < N_BOXES_QUAI + N_BOXES_TRAIN - 2; fw++) {
        double fz = w1 + 0.8 + fw * 2.0;
        g_boxes[b].bmin  = vec3_create(-1.02, 0.75, fz);
        g_boxes[b].bmax  = vec3_create(-0.98, 1.55, fz + 1.2);
        g_boxes[b].col   = vec3_create(0.90, 0.78, 0.55);   /* lumière chaude intérieure */
        g_boxes[b].ghost = 0.88;
        b++;
    }

    /* === WAGON 2 === */
    double w2 = w1 + 8.2;
    g_boxes[b].bmin  = vec3_create(-1.00, 0.05, w2);
    g_boxes[b].bmax  = vec3_create( 1.00, 2.00, w2 + 7.0);
    g_boxes[b].col   = gc2;
    g_boxes[b].ghost = 0.80;
    b++;

    /* Rembourrage : s'assurer que les box inutilisées sont hors champ */
    while (b < N_BOXES_QUAI + N_BOXES_TRAIN) {
        g_boxes[b].bmin  = vec3_create(9999, 9999, 9999);
        g_boxes[b].bmax  = vec3_create(9999, 9999, 9999);
        g_boxes[b].col   = vec3_create(0,0,0);
        g_boxes[b].ghost = 0.0;
        b++;
    }
}

// ============================================================
//  Palette ciel — ambiance Voyage de Chihiro / Sixième Station
//
//  Trois zones verticales interpolées :
//
//   ZÉNITH    dir.y ≈ +1   Violet profond      #1A0A2E
//   CIEL      dir.y ≈  0   Bleu-rose crépuscule #6B3FA0 → #D4608A
//   HORIZON   dir.y ≈ -0.1 Orange brûlé        #F07030
//   SOUS-HORIZ dir.y < -0.1 Jaune soleil bas    #FFB347
//
//  On utilise un Lerp à 4 clés (smooth-step entre zones).
// ============================================================

/* smooth-step */
static double smoothstep(double edge0, double edge1, double x) {
    double t = (x - edge0) / (edge1 - edge0);
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return t * t * (3.0 - 2.0 * t);
}

// ============================================================
//  Ciel Ghibli — nuages style cel-shading, crépuscule doré
// ============================================================

/* Bruit FBM simple 2D (sans librairie externe) */
static double hash2(double x, double y) {
    double v = sin(x * 127.1 + y * 311.7) * 43758.5453;
    return v - floor(v);
}
static double noise2(double x, double y) {
    double ix = floor(x), iy = floor(y);
    double fx = x - ix,   fy = y - iy;
    /* Smoothstep hermite */
    double ux = fx*fx*(3.0-2.0*fx), uy = fy*fy*(3.0-2.0*fy);
    double a = hash2(ix,   iy  );
    double b = hash2(ix+1, iy  );
    double c2= hash2(ix,   iy+1);
    double d = hash2(ix+1, iy+1);
    return a + (b-a)*ux + (c2-a)*uy + (b-a+a-b+d-c2)*ux*uy;
                         /* = a*(1-ux)*(1-uy) + b*ux*(1-uy)
                              + c*(1-ux)*uy   + d*ux*uy */
}
static double fbm(double x, double y) {
    double v = 0.0, amp = 0.50;
    for (int i = 0; i < 5; i++) {
        v   += amp * noise2(x, y);
        amp *= 0.50;
        x   *= 2.0;  y *= 2.0;
    }
    return v; /* ∈ [0, ~1] */
}

static color get_sky_color(vec3 dir) {
    double y = dir.y;

    /* ---- Gradient crépuscule 4-stops ---- */
    color C_ZEN  = vec3_create(0.08, 0.05, 0.22); /* nuit indigo      */
    color C_HIGH = vec3_create(0.28, 0.10, 0.52); /* mauve profond    */
    color C_MID  = vec3_create(0.90, 0.28, 0.38); /* rose Ghibli vif  */
    color C_HOR  = vec3_create(1.00, 0.60, 0.20); /* orange horizon   */
    color C_GLOW = vec3_create(1.00, 0.88, 0.45); /* jaune soleil     */

    color sky;
    if (y >= 0.0) {
        double t1 = smoothstep(0.00, 0.14, y);
        double t2 = smoothstep(0.12, 0.45, y);
        double t3 = smoothstep(0.40, 1.00, y);
        color  b1 = vec3_lerp(C_HOR,  C_MID,  t1);
        color  b2 = vec3_lerp(C_MID,  C_HIGH, t2);
        color  b3 = vec3_lerp(C_HIGH, C_ZEN,  t3);
        double w1 = 1.0 - smoothstep(0.0, 0.45, y);
        double w3 = smoothstep(0.40, 1.0, y);
        double w2 = 1.0 - w1 - w3; if (w2 < 0.0) w2 = 0.0;
        sky = vec3_add(vec3_add(vec3_scale(b1,w1),vec3_scale(b2,w2)),vec3_scale(b3,w3));
    } else {
        double t = smoothstep(-0.25, 0.0, y);
        sky = vec3_lerp(C_GLOW, C_HOR, t);
    }

    /* ---- Disque solaire + corona ---- */
    vec3 sun_dir = vec3_normalize(vec3_create(0.55, 0.12, 0.90));
    double dot_s = vec3_dot(dir, sun_dir);
    if (dot_s > 0.9965) {
        sky = vec3_lerp(sky, vec3_create(1.8, 1.7, 1.1),
                        smoothstep(0.9965, 1.00, dot_s));
    }
    double corona = smoothstep(0.982, 0.9965, dot_s);
    sky = vec3_add(sky, vec3_scale(vec3_create(1.0, 0.52, 0.12), corona * 0.55));
    double halo   = smoothstep(0.80,  0.982,  dot_s);
    sky = vec3_add(sky, vec3_scale(vec3_create(0.80, 0.28, 0.08), halo * 0.18));

    /* ---- Nuages Ghibli ---- */
    /* Projection sphérique → coordonnées nuages */
    if (y > -0.05) {
        /* Plan de nuages à hauteur h_cloud */
        double safe_y = (y < 0.02) ? 0.02 : y;
        double cloud_u = dir.x / safe_y * 0.35 + g_time * 0.006;
        double cloud_v = dir.z / safe_y * 0.35;

        /* FBM pour forme des nuages */
        double density = fbm(cloud_u * 2.2, cloud_v * 2.2);
        density = density - 0.38;  /* seuil → zones claires/sombres */

        /* Cel-shading : 3 niveaux de blanc (pas de gradient continu) */
        double cel;
        if      (density > 0.22) cel = 1.00;  /* cœur blanc pur        */
        else if (density > 0.08) cel = 0.72;  /* corps demi-ombre       */
        else if (density > 0.00) cel = 0.42;  /* bord sombre (outline)  */
        else                     cel = -1.0;  /* pas de nuage           */

        if (cel >= 0.0) {
            /* Teinte nuage : blanc chaud côté soleil, lilas côté ombre */
            double sun_cloud = smoothstep(-0.2, 0.2,
                                dot_s * 2.0 - 1.0);  /* 0=ombre, 1=soleil */
            color c_cloud_lit  = vec3_create(1.00, 0.95, 0.82); /* blanc chaud  */
            color c_cloud_shad = vec3_create(0.55, 0.38, 0.65); /* lilas-mauve  */
            color c_cloud = vec3_lerp(c_cloud_shad, c_cloud_lit, sun_cloud);
            c_cloud = vec3_scale(c_cloud, cel);

            /* Fondu doux en altitude (nuages moins denses vers zénith) */
            double fade = smoothstep(1.0, 0.08, y) * smoothstep(-0.02, 0.05, y);
            /* Contour noir Ghibli : outline foncé sur les bords */
            double outline = (density > 0.0 && density < 0.05) ? 1.0 : 0.0;
            c_cloud = vec3_lerp(c_cloud, vec3_create(0.08, 0.05, 0.12), outline * 0.7);

            sky = vec3_lerp(sky, c_cloud, cel * fade * 0.92);
        }
    }

    return vec3_clamp(sky, 0.0, 3.5);
}

static color sky_color(vec3 dir) { return get_sky_color(dir); }

// ============================================================
//  Fog atmosphérique — brume perspective Ghibli
//  Plus l'objet est loin, plus il se fond dans la couleur
//  de l'horizon (dégradé pêche-rose clair).
// ============================================================
#define FOG_START  8.0
#define FOG_END   55.0

static color apply_fog(color c, double t_hit) {
    double fog_t = (t_hit - FOG_START) / (FOG_END - FOG_START);
    if (fog_t < 0.0) fog_t = 0.0;
    if (fog_t > 1.0) fog_t = 1.0;
    fog_t = fog_t * fog_t * fog_t;  /* cubique : brume graduelle */
    /* Couleur brume : rose-lilas crépuscule */
    color fog_col = vec3_create(0.72, 0.42, 0.55);
    return vec3_lerp(c, fog_col, fog_t * 0.80);
}

// ============================================================
//  shade_surface — 3 sources : soleil + fill + rimlight
// ============================================================
static color shade_surface(color base_col, double emit, HitRecord rec) {
    /* Soleil bas à droite (aligné avec le disque du ciel) */
    vec3 sun  = vec3_normalize(vec3_create( 0.55, 0.60, 0.50));
    /* Fill doux depuis la gauche (ciel crep. mauve) */
    vec3 fill = vec3_normalize(vec3_create(-0.70, 0.40, 0.30));
    /* Rim froid depuis derrière (profondeur) */
    vec3 rim  = vec3_normalize(vec3_create( 0.10, 0.20,-1.00));

    double d_sun  = vec3_dot(rec.normal, sun);   if (d_sun  < 0) d_sun  = 0;
    double d_fill = vec3_dot(rec.normal, fill);  if (d_fill < 0) d_fill = 0;
    double d_rim  = vec3_dot(rec.normal, rim);   if (d_rim  < 0) d_rim  = 0;
    d_rim = d_rim * d_rim * 0.25; /* rim quadratique doux */

    color C_sun  = vec3_create(1.00, 0.78, 0.48);
    color C_fill = vec3_create(0.55, 0.30, 0.60);
    color C_rim  = vec3_create(0.30, 0.38, 0.70);
    color C_amb  = vec3_create(0.20, 0.12, 0.22);

    color lit = vec3_add(vec3_add(vec3_add(
        vec3_scale(C_amb,  0.30),
        vec3_scale(C_sun,  d_sun  * 0.85)),
        vec3_scale(C_fill, d_fill * 0.35)),
        vec3_scale(C_rim,  d_rim));

    color c = vec3_mul(base_col, lit);

    if (emit > 0.0)
        c = vec3_add(c, vec3_scale(base_col, emit * 0.80));

    return c;
}

// ============================================================
//  ObjectType — identifiant du type d'objet touché
// ============================================================
typedef enum {
    OBJ_NONE     = -1,
    OBJ_WATER    =  0,   /* plan eau          */
    OBJ_SPHERE   =  1,   /* index dans g_spheres   */
    OBJ_CYLINDER =  2,   /* index dans g_cylinders */
    OBJ_BOX      =  3    /* index dans g_boxes     */
} ObjType;

typedef struct {
    ObjType type;
    int     idx;
} ObjId;

// ============================================================
//  scene_intersect — teste TOUS les objets de la scène,
//  retourne le HitRecord le plus proche et l'identifiant
//  de l'objet touché.
// ============================================================
static HitRecord scene_intersect(Ray r, ObjId *out_id) {
    HitRecord best = hit_miss();
    out_id->type = OBJ_NONE;
    out_id->idx  = -1;

    double t_cur = T_MAX;   /* plafond décroissant pour chaque test */

    /* --- Sphères --- */
    for (int i = 0; i < N_SPHERES; i++) {
        HitRecord h = hit_sphere(r, g_spheres[i].center,
                                    g_spheres[i].radius,
                                    T_MIN, t_cur);
        if (h.hit) {
            best      = h;
            t_cur     = h.t;
            out_id->type = OBJ_SPHERE;
            out_id->idx  = i;
        }
    }

    /* --- Cylindres (poteaux) --- */
    for (int i = 0; i < N_CYLINDERS; i++) {
        Cylinder *cy = &g_cylinders[i];
        HitRecord h  = hit_cylinder_v(r,
                            cy->cx, cy->y_min, cy->cz,
                            cy->radius, cy->y_max,
                            T_MIN, t_cur);
        if (h.hit) {
            best      = h;
            t_cur     = h.t;
            out_id->type = OBJ_CYLINDER;
            out_id->idx  = i;
        }
    }

    /* --- Boîtes (quai + train) --- */
    for (int i = 0; i < N_BOXES; i++) {
        HitRecord h = hit_box(r, g_boxes[i].bmin, g_boxes[i].bmax,
                              T_MIN, t_cur);
        if (h.hit) {
            best      = h;
            t_cur     = h.t;
            out_id->type = OBJ_BOX;
            out_id->idx  = i;
        }
    }

    /* --- Plan eau ---
       Ne tester que si aucun objet solide n'est déjà plus proche.
       Le quai repose sur l'eau : on le teste AVANT le plan pour
       qu'il masque correctement la surface. */
    {
        HitRecord hp = hit_plane(r, WATER_LEVEL, T_MIN, t_cur);
        if (hp.hit) {
            best      = hp;
            out_id->type = OBJ_WATER;
            out_id->idx  = -1;
        }
    }

    return best;
}

// ============================================================
//  ray_color — récursif, profondeur max MAX_BOUNCES
// ============================================================
static color ray_color(Ray r, int depth) {
    if (depth > MAX_BOUNCES) return sky_color(r.direction);

    ObjId id;
    HitRecord rec = scene_intersect(r, &id);

    if (!rec.hit) return sky_color(r.direction);

    /* --- Plan eau : Fresnel + ripples --- */
    if (id.type == OBJ_WATER) {
        /* Perturbation de la normale par les ripples */
        double t_anim = g_time;
        vec3 ripple_n = water_normal(rec.point.x, rec.point.z, t_anim);
        /* On blend normal plate + ripple selon profondeur d'angle */
        double blend_r = 0.55;
        vec3 perturbed = vec3_normalize(vec3_add(
                            vec3_scale(vec3_create(0,1,0), 1.0 - blend_r),
                            vec3_scale(ripple_n, blend_r)));

        vec3  refl_dir  = vec3_reflect(r.direction, perturbed);
        Ray   refl_ray  = ray_create(rec.point, refl_dir);
        color reflected = ray_color(refl_ray, depth + 1);

        /* Teinte eau : turquoise profond façon Ghibli */
        color water_tint = vec3_create(0.03, 0.18, 0.28);

        double cos_theta = -vec3_dot(r.direction, perturbed);
        if (cos_theta < 0.0) cos_theta = 0.0;
        double one_min = 1.0 - cos_theta;
        double fres = 0.06 + 0.94 *
                      (one_min*one_min*one_min*one_min*one_min);

        color warm_tint   = vec3_create(1.02, 0.97, 0.92);
        color tinted_refl = vec3_mul(reflected, warm_tint);

        color c = vec3_add(vec3_scale(water_tint, 1.0 - fres),
                           vec3_scale(tinted_refl, fres));
        return apply_fog(c, rec.t);
    }

    /* --- Sphère (lanterne / lune) --- */
    if (id.type == OBJ_SPHERE) {
        Sphere *s = &g_spheres[id.idx];
        color c = shade_surface(s->col, s->emit, rec);
        return apply_fog(c, rec.t);
    }

    /* --- Cylindre (poteau) : texture bois procédurale --- */
    if (id.type == OBJ_CYLINDER) {
        Cylinder *cy = &g_cylinders[id.idx];
        /* UV : U = angle autour de l'axe, V = hauteur */
        double u_cyl = atan2(rec.point.x - cy->cx, rec.point.z - cy->cz)
                       / (2.0 * M_PI) + 0.5;
        double v_cyl = (rec.point.y - cy->y_min) / (cy->y_max - cy->y_min);
        color tex_col = wood_texture(u_cyl * 3.0, v_cyl * 2.0);
        color c = shade_surface(tex_col, 0.0, rec);
        return apply_fog(c, rec.t);
    }

    /* --- Box (quai ou train) --- */
    if (id.type == OBJ_BOX) {
        Box *bx = &g_boxes[id.idx];

        /* Phares de la locomotive : émissifs */
        if (bx->ghost == 0.0 && bx->col.x > 0.9 && bx->col.z < 0.7) {
            /* Phare chaud : couleur émissive directe */
            color c = vec3_scale(bx->col, 3.5);
            return apply_fog(c, rec.t);
        }

        /* Objets solides (quai) */
        if (bx->ghost <= 0.0) {
            color tex_col = (id.idx == 0)
                ? rail_texture(rec.point.x, rec.point.z)
                : shade_surface(bx->col, 0.0, rec);
            return apply_fog(tex_col, rec.t);
        }

        /* --- RENDU FANTÔME --- */
        /* 1. Couleur de base éclairée */
        color solid = shade_surface(bx->col, 0.0, rec);

        /* 2. Lueur intérieure — les fenêtres émettent chaleur */
        double win_emit = (bx->col.x > 0.85 && bx->col.z < 0.70) ? 1.8 : 0.0;
        if (win_emit > 0.0)
            solid = vec3_add(solid, vec3_scale(bx->col, win_emit));

        /* 3. Transparence : on voit derrière le train (rayon continu) */
        Ray   behind_ray = ray_create(rec.point, r.direction);
        color behind     = ray_color(behind_ray, depth + 1);

        /* 4. Fresnel sur les bords : plus opaque de face, transparent en biais */
        double cos_t = fabs(vec3_dot(r.direction, rec.normal));
        /* bord du train = presque transparent, face = semi-opaque */
        double alpha = bx->ghost * (1.0 - cos_t * 0.6);
        if (alpha > 1.0) alpha = 1.0;

        /* 5. Halo bleuté autour du fantôme (outline glow) */
        double rim = 1.0 - cos_t;
        color glow = vec3_scale(vec3_create(0.50, 0.75, 1.00), rim * rim * 0.80);

        color c = vec3_lerp(behind, vec3_add(solid, glow), alpha);
        return apply_fog(c, rec.t);
    }

    return sky_color(r.direction);
}

// ============================================================
//  Constantes caméra
// ============================================================
#define CAM_SPEED_DEMO  2.2    /* unités/s  — avance automatique démo     */
#define CAM_SPEED_JEU   4.0    /* unités/s  — déplacement joueur          */
#define CAM_Y           1.2    /* hauteur de l'œil — caméra basse        */
#define CAM_LOOKAT_DZ   7.0    /* look-ahead en mode démo                 */
#define CAM_Z_START     1.5    /* position Z de départ (dans la scène)    */
#define CAM_BREATH_AMP  0.10   /* amplitude oscillation Y (respiration)   */
#define CAM_BREATH_FREQ 0.04   /* fréquence oscillation (rad / frame)     */
#define LINES_PER_FRAME 6      /* lignes rendues par frame                */

// ============================================================
//  Structure application
// ============================================================
typedef struct {
    SDL_Window*   window;
    SDL_Renderer* renderer;
    SDL_Texture*  texture;
    Uint32*       pixel_buffer;
    int           running;
    Camera        cam;

    /* --- position FPS --- */
    vec3   pos;           /* position de l'œil dans le monde             */
    vec3   dir;           /* vecteur direction (normalisé)               */
    vec3   right;         /* vecteur droite   (normalisé, perp à dir)    */
    double yaw;           /* rotation horizontale (degrés)               */
    double pitch;         /* rotation verticale   (degrés, clamped)      */

    /* --- dual-mode --- */
    int    is_demo_mode;  /* 1 = cinématique, 0 = jeu                    */
    Uint32 frame_count;   /* compteur de frames (pour la respiration)    */

    /* --- clavier (état continu) --- */
    int    key_w;         /* Z ou W : avance                             */
    int    key_s;         /* S     : recule                              */
    int    key_a;         /* Q ou A : strafe gauche                      */
    int    key_d;         /* D     : strafe droite                       */
    int    key_up;        /* ↑ : tourner vue vers le haut               */
    int    key_down;      /* ↓ : tourner vue vers le bas                */
    int    key_left;      /* ← : pivoter à gauche                       */
    int    key_right;     /* → : pivoter à droite                      */

    /* --- souris --- */
    int    mouse_captured; /* 1 = souris capturée (mode jeu)             */

    /* --- book-keeping --- */
    int           current_line;
    Uint32        last_ticks;
    double        time;
} App;


// ============================================================
//  init_app
// ============================================================
int init_app(App* app) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Erreur SDL_Init: %s\n", SDL_GetError());
        return 0;
    }

    app->window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN);
    if (!app->window) {
        printf("Erreur fenetre: %s\n", SDL_GetError());
        SDL_Quit();
        return 0;
    }

    app->renderer = SDL_CreateRenderer(app->window, -1, SDL_RENDERER_ACCELERATED);
    if (!app->renderer) {
        printf("Erreur renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(app->window);
        SDL_Quit();
        return 0;
    }

    app->texture = SDL_CreateTexture(
        app->renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!app->texture) {
        printf("Erreur texture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(app->renderer);
        SDL_DestroyWindow(app->window);
        SDL_Quit();
        return 0;
    }

    app->pixel_buffer = (Uint32*)malloc(WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(Uint32));
    if (!app->pixel_buffer) {
        printf("Erreur malloc pixel buffer\n");
        SDL_DestroyTexture(app->texture);
        SDL_DestroyRenderer(app->renderer);
        SDL_DestroyWindow(app->window);
        SDL_Quit();
        return 0;
    }

    // Noir au départ
    for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
        app->pixel_buffer[i] = 0xFF000000;

    // ----------------------------------------------------------
    //  Caméra — position et orientation initiales
    // ----------------------------------------------------------
    app->pos   = vec3_create(0.0, CAM_Y, CAM_Z_START);
    app->yaw   = 90.0;
    app->pitch = -8.0;           /* angle légèrement bas, quai visible */
    {
        double yr  = app->yaw   * M_PI / 180.0;
        double pr  = app->pitch * M_PI / 180.0;
        app->dir   = vec3_normalize(vec3_create(
                         cos(pr)*cos(yr), sin(pr), cos(pr)*sin(yr)));
        vec3 vup   = vec3_create(0.0, 1.0, 0.0);
        app->right = vec3_normalize(vec3_cross(app->dir, vup));
    }
    {
        vec3 lookat = vec3_add(app->pos, app->dir);
        vec3 vup    = vec3_create(0.0, 1.0, 0.0);
        app->cam = camera_create(app->pos, lookat, vup,
                                 68.0, WINDOW_WIDTH, WINDOW_HEIGHT); /* fov large */
    }

    app->is_demo_mode = 1;
    app->frame_count  = 0;
    app->key_w = app->key_s = app->key_a = app->key_d = 0;
    app->key_up = app->key_down = app->key_left = app->key_right = 0;
    app->mouse_captured = 0;
    app->running      = 1;
    app->current_line = 0;
    app->last_ticks   = SDL_GetTicks();
    app->time         = 0.0;
    g_time            = 0.0;

    build_scene();


    printf("SDL OK  \u2014  %dx%d  \u2014  ESC quitter | SPACE toggle Demo/Jeu\n",
           WINDOW_WIDTH, WINDOW_HEIGHT);
    printf("Mode: Demo\n");
    return 1;
}

// ============================================================
//  handle_events  —  état continu du clavier + toggle mode
// ============================================================
void handle_events(App* app) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) { app->running = 0; return; }

        /* Mouvement souris relatif (mode jeu capturé) */
        if (event.type == SDL_MOUSEMOTION && app->mouse_captured) {
            double sens = 0.12;   /* degrés par pixel */
            app->yaw   += event.motion.xrel * sens;
            app->pitch -= event.motion.yrel * sens;
            if (app->pitch >  85.0) app->pitch =  85.0;
            if (app->pitch < -85.0) app->pitch = -85.0;
        }

        /* Clic gauche en mode jeu : capturer la souris */
        if (event.type == SDL_MOUSEBUTTONDOWN
            && event.button.button == SDL_BUTTON_LEFT
            && !app->is_demo_mode) {
            app->mouse_captured = 1;
            SDL_SetRelativeMouseMode(SDL_TRUE);
        }

        if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    if (app->mouse_captured) {
                        /* Premier ESC : relâcher souris */
                        app->mouse_captured = 0;
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                    } else {
                        app->running = 0;
                    }
                    break;

                case SDLK_SPACE:
                    app->is_demo_mode = !app->is_demo_mode;
                    app->current_line = 0;
                    if (!app->is_demo_mode) {
                        /* Entrer en mode jeu : capture souris auto */
                        app->mouse_captured = 1;
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                    } else {
                        app->mouse_captured = 0;
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                    }
                    printf("Mode: %s\n",
                           app->is_demo_mode ? "Demo" : "Jeu (ZQSD + Souris | ESC = libère souris)");
                    break;

                case SDLK_w: case SDLK_z: app->key_w = 1; break;
                case SDLK_s:              app->key_s = 1; break;
                case SDLK_a: case SDLK_q: app->key_a = 1; break;
                case SDLK_d:              app->key_d = 1; break;
                case SDLK_UP:             app->key_up    = 1; break;
                case SDLK_DOWN:           app->key_down  = 1; break;
                case SDLK_LEFT:           app->key_left  = 1; break;
                case SDLK_RIGHT:          app->key_right = 1; break;
                default: break;
            }
        }
        if (event.type == SDL_KEYUP) {
            switch (event.key.keysym.sym) {
                case SDLK_w: case SDLK_z: app->key_w = 0; break;
                case SDLK_s:              app->key_s = 0; break;
                case SDLK_a: case SDLK_q: app->key_a = 0; break;
                case SDLK_d:              app->key_d = 0; break;
                case SDLK_UP:             app->key_up    = 0; break;
                case SDLK_DOWN:           app->key_down  = 0; break;
                case SDLK_LEFT:           app->key_left  = 0; break;
                case SDLK_RIGHT:          app->key_right = 0; break;
                default: break;
            }
        }
    }
}

// ============================================================
//  rebuild_camera  —  reconstruit Camera pinhole depuis pos+dir
// ============================================================
static void rebuild_camera(App* app) {
    vec3 lookat = vec3_add(app->pos, app->dir);
    vec3 vup    = vec3_create(0.0, 1.0, 0.0);
    app->cam = camera_create(app->pos, lookat, vup,
                             68.0, WINDOW_WIDTH, WINDOW_HEIGHT);
    /* En mode jeu, reset le scan pour éviter les bandes décalées.
       En mode démo, la caméra avance si doucement que le scan
       progressif donne un effet de "rideau" acceptable. */
    if (!app->is_demo_mode)
        app->current_line = 0;
}

// ============================================================
//  update_camera  —  dual-mode : Demo (cinématique) / Jeu (FPS)
// ============================================================
static void update_camera(App* app, double dt) {
    /* --- Rotation clavier (flèches) — active dans les deux modes --- */
    double r_spd = 80.0 * dt;   /* degrés/s */
    if (app->key_left)  app->yaw   -= r_spd;
    if (app->key_right) app->yaw   += r_spd;
    if (app->key_up)    app->pitch += r_spd;
    if (app->key_down)  app->pitch -= r_spd;
    if (app->pitch >  85.0) app->pitch =  85.0;
    if (app->pitch < -85.0) app->pitch = -85.0;

    /* --- Recalcul direction depuis yaw/pitch (toujours) --- */
    {
        double yr = app->yaw   * M_PI / 180.0;
        double pr = app->pitch * M_PI / 180.0;
        app->dir   = vec3_normalize(vec3_create(
                         cos(pr)*cos(yr), sin(pr), cos(pr)*sin(yr)));
        vec3 vup   = vec3_create(0.0, 1.0, 0.0);
        app->right = vec3_normalize(vec3_cross(app->dir, vup));
    }

    if (app->is_demo_mode) {
        /* ---- MODE DEMO : avance automatique + respiration ---- */
        app->pos.z += CAM_SPEED_DEMO * dt;
        app->pos.y = CAM_Y
                     + sin(app->frame_count * CAM_BREATH_FREQ) * CAM_BREATH_AMP;
    } else {
        /* ---- MODE JEU : ZQSD + souris ---- */
        double spd = CAM_SPEED_JEU * dt;
        if (app->key_w) app->pos = vec3_add(app->pos, vec3_scale(app->dir,   spd));
        if (app->key_s) app->pos = vec3_sub(app->pos, vec3_scale(app->dir,   spd));
        if (app->key_a) app->pos = vec3_sub(app->pos, vec3_scale(app->right, spd));
        if (app->key_d) app->pos = vec3_add(app->pos, vec3_scale(app->right, spd));
    }

    rebuild_camera(app);   /* toujours reconstruire (coût négligeable) */
    app->frame_count++;
}

// ============================================================
//  update_pixels  —  rendu progressif LINES_PER_FRAME lignes/frame
// ============================================================
void update_pixels(App* app) {
    /* --- Delta temps --- */
    Uint32 now      = SDL_GetTicks();
    double dt       = (now - app->last_ticks) / 1000.0;
    app->last_ticks = now;
    if (dt > 0.1) dt = 0.1;

    app->time += dt;
    g_time     = app->time;

    /* --- Mise à jour du train : avance plus vite que la caméra --- */
    /* En mode démo, le train arrive depuis le fond et passe devant;  */
    /* il se téléporte loin quand il est passé.                       */
    if (app->is_demo_mode) {
        g_train_z -= 6.5 * dt;   /* train avance vers z=0 */
        if (g_train_z < app->pos.z - 30.0)
            g_train_z = app->pos.z + 80.0;  /* reboucle */
    }
    update_train();

    /* --- Mise à jour de la caméra (dual-mode) --- */
    update_camera(app, dt);

    /* --- Rendu complet du frame --- */
    /* Rendre chaque pixel du frame complet à chaque itération */
    /* Grille 2×2 pour 4× MSAA (décalages sous-pixel fixes Halton) */
    static const double sub_x[4] = {0.25, 0.75, 0.25, 0.75};
    static const double sub_y[4] = {0.25, 0.25, 0.75, 0.75};

    double half_w = WINDOW_WIDTH  * 0.5;
    double half_h = WINDOW_HEIGHT * 0.5;

    for (int py = 0; py < WINDOW_HEIGHT; py++) {
        for (int px = 0; px < WINDOW_WIDTH; px++) {

            /* --- 4× MSAA --- */
            color acc = vec3_create(0,0,0);
            for (int s = 0; s < AA_SAMPLES; s++) {
                Ray r = ray_for_pixel_sub(&app->cam,
                            (double)px + sub_x[s],
                            (double)py + sub_y[s]);
                acc = vec3_add(acc, ray_color(r, 0));
            }
            color col = vec3_scale(acc, 1.0 / AA_SAMPLES);

            /* --- Vignette douce (cercle unitaire) --- */
            double nx = ((double)px - half_w) / half_w;
            double ny = ((double)py - half_h) / half_h;
            double vig = 1.0 - (nx*nx + ny*ny) * 0.50;
            if (vig < 0.0) vig = 0.0;
            vig = vig * vig;  /* smooth */
            col = vec3_scale(col, vig);

            app->pixel_buffer[py * WINDOW_WIDTH + px] = color_tonemap_argb(col);
        }
    }
}

// ============================================================
//  render  —  envoie le buffer à l'écran
// ============================================================
void render(App* app) {
    SDL_UpdateTexture(app->texture, NULL,
                      app->pixel_buffer, WINDOW_WIDTH * sizeof(Uint32));
    SDL_RenderClear(app->renderer);
    SDL_RenderCopy(app->renderer, app->texture, NULL, NULL);
    SDL_RenderPresent(app->renderer);
}

// ============================================================
//  cleanup
// ============================================================
void cleanup(App* app) {
    free(app->pixel_buffer);
    SDL_DestroyTexture(app->texture);
    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow(app->window);
    SDL_Quit();
}

// ============================================================
//  main
// ============================================================
int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    App app = {0};
    if (!init_app(&app)) return 1;

    while (app.running) {
        handle_events(&app);
        update_pixels(&app);
        render(&app);
        SDL_Delay(16);
    }

    cleanup(&app);
    printf("Ferme proprement.\n");
    return 0;
}
