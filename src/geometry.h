#ifndef GEOMETRY_H
#define GEOMETRY_H

// ============================================================
//  geometry.h  —  Intersections rayon / objets géométriques
//
//  Objets supportés :
//    - Sphère  : centre + rayon
//    - Plan    : plan horizontal infini  y = level
//
//  Toutes les fonctions sont static inline pour la perf.
// ============================================================

#include "vec3.h"
#include "ray.h"
#include <math.h>

// ------------------------------------------------------------
//  HitRecord  —  résultat d'une intersection
// ------------------------------------------------------------
typedef struct {
    int    hit;       // 1 si intersection, 0 sinon
    double t;         // paramètre sur le rayon (distance)
    vec3   point;     // point 3D d'impact
    vec3   normal;    // normale unitaire au point d'impact
                      //   toujours pointée VERS l'observateur
                      //   (normal . ray_dir < 0)
} HitRecord;

// Sentinelle : aucune intersection
static inline HitRecord hit_miss(void) {
    HitRecord h;
    h.hit    = 0;
    h.t      = 1e308; /* "infini" */
    h.point  = vec3_zero();
    h.normal = vec3_zero();
    return h;
}

// ------------------------------------------------------------
//  hit_sphere
//
//  Equation : ||P(t) - C||² = r²
//  Développée : at² + bt + c = 0, avec
//    a = d·d          (d = direction du rayon)
//    b = 2 * d·(O-C)  (O = origine du rayon)
//    c = (O-C)·(O-C) - r²
//
//  Discriminant : disc = (b/2)² - a*c   (forme réduite)
//  t = (-b/2 ± sqrt(disc)) / a
//
//  On prend la racine la plus petite dans [t_min, t_max].
//
// ------------------------------------------------------------
static inline HitRecord hit_sphere(Ray r, vec3 center, double radius,
                                   double t_min, double t_max) {
    vec3   oc = vec3_sub(r.origin, center);
    double a  = vec3_dot(r.direction, r.direction);
    double hb = vec3_dot(r.direction, oc);      /* b/2 */
    double c  = vec3_dot(oc, oc) - radius * radius;

    double disc = hb * hb - a * c;

    if (disc < 0.0) return hit_miss();

    double sqrtd = sqrt(disc);

    /* Racine la plus proche dans [t_min, t_max] */
    double t = (-hb - sqrtd) / a;
    if (t < t_min || t > t_max) {
        t = (-hb + sqrtd) / a;
        if (t < t_min || t > t_max) return hit_miss();
    }

    HitRecord rec;
    rec.hit   = 1;
    rec.t     = t;
    rec.point = ray_at(r, t);

    /* Normale : de la surface vers l'extérieur, puis orientée
       vers l'observateur si besoin */
    vec3 outward_normal = vec3_scale(
                            vec3_sub(rec.point, center),
                            1.0 / radius);
    /* On s'assure que normal . ray_dir < 0 */
    if (vec3_dot(r.direction, outward_normal) > 0.0)
        outward_normal = vec3_neg(outward_normal);
    rec.normal = outward_normal;

    return rec;
}

// ------------------------------------------------------------
//  hit_plane  —  plan horizontal infini y = level
//
//  Equation : P(t).y = level
//   ⟹  origin.y + t * direction.y = level
//   ⟹  t = (level - origin.y) / direction.y
//
//  Pas d'intersection si direction.y ≈ 0 (rayon parallèle).
//
// ------------------------------------------------------------
static inline HitRecord hit_plane(Ray r, double level,
                                  double t_min, double t_max) {
    /* Rayon presque parallèle au plan → pas d'intersection */
    if (fabs(r.direction.y) < 1e-8) return hit_miss();

    double t = (level - r.origin.y) / r.direction.y;
    if (t < t_min || t > t_max) return hit_miss();

    HitRecord rec;
    rec.hit   = 1;
    rec.t     = t;
    rec.point = ray_at(r, t);

    /* Normale vers le haut (+Y), orientée vers l'observateur */
    vec3 n = vec3_create(0.0, 1.0, 0.0);
    if (vec3_dot(r.direction, n) > 0.0) n = vec3_neg(n);
    rec.normal = n;

    return rec;
}

// ------------------------------------------------------------
//  hit_cylinder_v  —  cylindre vertical (axe Y)
//
//  Centre de la base (cx, y_min, cz), rayon r, hauteur y_max.
//
//  Équation du cylindre infini (axe Y) :
//    (Px - cx)² + (Pz - cz)² = r²
//
//  Substitution P(t) = O + t*D :
//    let dx = Ox - cx,  dz = Oz - cz
//    a  = Dx² + Dz²
//    hb = dx*Dx + dz*Dz    (= b/2)
//    c  = dx² + dz² - r²
//    disc = hb² - a*c
//
//  Après avoir trouvé t :
//    - vérifier y_min ≤ P(t).y ≤ y_max  (hauteur du cylindre)
//    - normale latérale : (Px-cx, 0, Pz-cz) / r
//    - tester aussi les deux disques (caps haut et bas)
//
// ------------------------------------------------------------
static inline HitRecord hit_cylinder_v(Ray r,
                                       double cx, double y_min,
                                       double cz, double radius,
                                       double y_max,
                                       double t_min, double t_max) {
    double dx = r.origin.x - cx;
    double dz = r.origin.z - cz;

    double a  = r.direction.x * r.direction.x
              + r.direction.z * r.direction.z;
    double hb = dx * r.direction.x + dz * r.direction.z;
    double c  = dx * dx + dz * dz - radius * radius;

    HitRecord best = hit_miss();

    /* --- Corps latéral --- */
    if (a > 1e-12) {
        double disc = hb * hb - a * c;
        if (disc >= 0.0) {
            double sqrtd = sqrt(disc);
            /* Deux racines — on prend la plus petite valide */
            double roots[2];
            roots[0] = (-hb - sqrtd) / a;
            roots[1] = (-hb + sqrtd) / a;
            for (int i = 0; i < 2; i++) {
                double t = roots[i];
                if (t < t_min || t > t_max) continue;
                double py = r.origin.y + t * r.direction.y;
                if (py < y_min || py > y_max) continue;

                /* C'est valide et plus proche */
                if (!best.hit || t < best.t) {
                    best.hit   = 1;
                    best.t     = t;
                    best.point = ray_at(r, t);
                    /* Normale radiale (horizontale) */
                    vec3 n = vec3_create(
                        (best.point.x - cx) / radius,
                        0.0,
                        (best.point.z - cz) / radius);
                    if (vec3_dot(r.direction, n) > 0.0) n = vec3_neg(n);
                    best.normal = n;
                }
                break; /* Ne garder que la plus petite */
            }
        }
    }

    /* --- Disques caps (haut et bas) --- */
    if (fabs(r.direction.y) > 1e-8) {
        double caps[2] = { y_min, y_max };
        vec3   normals[2] = { vec3_create(0,-1,0), vec3_create(0,1,0) };
        for (int i = 0; i < 2; i++) {
            double t = (caps[i] - r.origin.y) / r.direction.y;
            if (t < t_min || t > t_max) continue;
            if (best.hit && t >= best.t) continue;
            vec3 p = ray_at(r, t);
            double px = p.x - cx, pz = p.z - cz;
            if (px*px + pz*pz > radius * radius) continue;
            best.hit   = 1;
            best.t     = t;
            best.point = p;
            vec3 n = normals[i];
            if (vec3_dot(r.direction, n) > 0.0) n = vec3_neg(n);
            best.normal = n;
        }
    }

    return best;
}

// ------------------------------------------------------------
//  hit_box  —  parallélépipède rectangle aligné sur les axes
//              (AABB — Axis-Aligned Bounding Box)
//
//  Méthode des dalles (slab method) :
//    Pour chaque paire de plans (axe X, Y, Z) :
//      t_near = (min_i - O_i) / D_i
//      t_far  = (max_i - O_i) / D_i
//      si t_near > t_far : swap
//    t_enter = max(t_near_x, t_near_y, t_near_z)
//    t_exit  = min(t_far_x,  t_far_y,  t_far_z)
//    Si t_enter > t_exit ou t_exit < t_min → pas d'intersection.
//
//  La normale est déterminée par la face d'entrée (la dalle
//  dont t_near est le maximum parmi les trois axes).
//
// ------------------------------------------------------------
static inline HitRecord hit_box(Ray r,
                                vec3 bmin, vec3 bmax,
                                double t_min, double t_max) {
    double tx_near = (bmin.x - r.origin.x);
    double tx_far  = (bmax.x - r.origin.x);
    double ty_near = (bmin.y - r.origin.y);
    double ty_far  = (bmax.y - r.origin.y);
    double tz_near = (bmin.z - r.origin.z);
    double tz_far  = (bmax.z - r.origin.z);

    /* Division par la direction (gestion du rayon vertical / horizontal) */
    int nx_neg = 0, ny_neg = 0, nz_neg = 0;

    if (fabs(r.direction.x) > 1e-12) {
        tx_near /= r.direction.x;
        tx_far  /= r.direction.x;
    } else {
        /* Rayon parallèle à YZ : hors dalle → miss */
        if (r.origin.x < bmin.x || r.origin.x > bmax.x)
            return hit_miss();
        tx_near = -1e308; tx_far = 1e308;
    }
    if (fabs(r.direction.y) > 1e-12) {
        ty_near /= r.direction.y;
        ty_far  /= r.direction.y;
    } else {
        if (r.origin.y < bmin.y || r.origin.y > bmax.y)
            return hit_miss();
        ty_near = -1e308; ty_far = 1e308;
    }
    if (fabs(r.direction.z) > 1e-12) {
        tz_near /= r.direction.z;
        tz_far  /= r.direction.z;
    } else {
        if (r.origin.z < bmin.z || r.origin.z > bmax.z)
            return hit_miss();
        tz_near = -1e308; tz_far = 1e308;
    }

    /* Swap pour garantir near < far */
    if (tx_near > tx_far) { double tmp=tx_near; tx_near=tx_far; tx_far=tmp; nx_neg=1; }
    if (ty_near > ty_far) { double tmp=ty_near; ty_near=ty_far; ty_far=tmp; ny_neg=1; }
    if (tz_near > tz_far) { double tmp=tz_near; tz_near=tz_far; tz_far=tmp; nz_neg=1; }
    (void)nx_neg; (void)ny_neg; (void)nz_neg;

    /* t_enter = max des near, t_exit = min des far */
    int   face  = 0;   /* 0=X, 1=Y, 2=Z — face d'entrée */
    double t_enter = tx_near; face = 0;
    if (ty_near > t_enter) { t_enter = ty_near; face = 1; }
    if (tz_near > t_enter) { t_enter = tz_near; face = 2; }

    double t_exit = tx_far;
    if (ty_far < t_exit) t_exit = ty_far;
    if (tz_far < t_exit) t_exit = tz_far;

    if (t_enter > t_exit + 1e-9) return hit_miss();

    double t = t_enter;
    if (t < t_min) { t = t_exit; }   /* On est à l'intérieur → face de sortie */
    if (t < t_min || t > t_max) return hit_miss();

    HitRecord rec;
    rec.hit   = 1;
    rec.t     = t;
    rec.point = ray_at(r, t);

    /* Normale selon la face touchée */
    vec3 n = vec3_zero();
    if      (face == 0) n = vec3_create(r.direction.x < 0 ? 1.0 : -1.0, 0, 0);
    else if (face == 1) n = vec3_create(0, r.direction.y < 0 ? 1.0 : -1.0, 0);
    else                n = vec3_create(0, 0, r.direction.z < 0 ? 1.0 : -1.0);
    rec.normal = n;

    return rec;
}

// ------------------------------------------------------------
//  Utilitaire : compare deux HitRecord et retourne le plus
//  proche (le t le plus petit parmi ceux qui ont hit==1)
// ------------------------------------------------------------
static inline HitRecord hit_closest(HitRecord a, HitRecord b) {
    if (!a.hit) return b;
    if (!b.hit) return a;
    return (a.t < b.t) ? a : b;
}

#endif /* GEOMETRY_H */
