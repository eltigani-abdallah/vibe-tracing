#include "geometry.h"
#include <stdlib.h>
#include <math.h>

// Création d'une sphère
GeometryObject* create_sphere(Vec3 center, double radius, Material material) {
    GeometryObject* obj = malloc(sizeof(GeometryObject));
    if (!obj) return NULL;
    
    obj->type = GEOMETRY_SPHERE;
    obj->material = material;
    obj->sphere.center = center;
    obj->sphere.radius = radius;
    obj->next = NULL;
    
    return obj;
}

// Création d'un plan
GeometryObject* create_plane(Vec3 point, Vec3 normal, Material material) {
    GeometryObject* obj = malloc(sizeof(GeometryObject));
    if (!obj) return NULL;
    
    obj->type = GEOMETRY_PLANE;
    obj->material = material;
    obj->plane.point = point;
    obj->plane.normal = vec3_normalize(normal);
    obj->next = NULL;
    
    return obj;
}

// Création d'une boîte
GeometryObject* create_box(Vec3 min_corner, Vec3 max_corner, Material material) {
    GeometryObject* obj = malloc(sizeof(GeometryObject));
    if (!obj) return NULL;
    
    obj->type = GEOMETRY_BOX;
    obj->material = material;
    obj->box.min_corner = min_corner;
    obj->box.max_corner = max_corner;
    obj->next = NULL;
    
    return obj;
}

// Intersection avec une sphère
static int sphere_hit(GeometryObject* sphere, Ray ray, double t_min, double t_max, HitRecord* hit) {
    Vec3 oc = vec3_sub(ray.origin, sphere->sphere.center);
    double a = vec3_length_squared(ray.direction);
    double half_b = vec3_dot(oc, ray.direction);
    double c = vec3_length_squared(oc) - sphere->sphere.radius * sphere->sphere.radius;
    
    double discriminant = half_b * half_b - a * c;
    
    if (discriminant < 0) return 0;
    
    double sqrtd = sqrt(discriminant);
    double root = (-half_b - sqrtd) / a;
    
    if (root < t_min || t_max < root) {
        root = (-half_b + sqrtd) / a;
        if (root < t_min || t_max < root) return 0;
    }
    
    hit->t = root;
    hit->point = ray_at(ray, hit->t);
    Vec3 outward_normal = vec3_div(vec3_sub(hit->point, sphere->sphere.center), sphere->sphere.radius);
    hit_record_set_face_normal(hit, ray, outward_normal);
    hit->hit = 1;
    hit->material = &sphere->material;
    
    return 1;
}

// Intersection avec un plan
static int plane_hit(GeometryObject* plane, Ray ray, double t_min, double t_max, HitRecord* hit) {
    double denom = vec3_dot(plane->plane.normal, ray.direction);
    
    // Le rayon est parallèle au plan
    if (fabs(denom) < 1e-8) return 0;
    
    double t = vec3_dot(vec3_sub(plane->plane.point, ray.origin), plane->plane.normal) / denom;
    
    if (t < t_min || t > t_max) return 0;
    
    hit->t = t;
    hit->point = ray_at(ray, t);
    hit_record_set_face_normal(hit, ray, plane->plane.normal);
    hit->hit = 1;
    hit->material = &plane->material;
    
    return 1;
}

// Intersection avec une boîte (AABB)
static int box_hit(GeometryObject* box, Ray ray, double t_min, double t_max, HitRecord* hit) {
    Vec3 inv_dir = vec3_create(1.0 / ray.direction.x, 1.0 / ray.direction.y, 1.0 / ray.direction.z);
    
    Vec3 t0 = vec3_create(
        (box->box.min_corner.x - ray.origin.x) * inv_dir.x,
        (box->box.min_corner.y - ray.origin.y) * inv_dir.y,
        (box->box.min_corner.z - ray.origin.z) * inv_dir.z
    );
    
    Vec3 t1 = vec3_create(
        (box->box.max_corner.x - ray.origin.x) * inv_dir.x,
        (box->box.max_corner.y - ray.origin.y) * inv_dir.y,
        (box->box.max_corner.z - ray.origin.z) * inv_dir.z
    );
    
    // Assurer que t0 < t1
    if (t0.x > t1.x) { double temp = t0.x; t0.x = t1.x; t1.x = temp; }
    if (t0.y > t1.y) { double temp = t0.y; t0.y = t1.y; t1.y = temp; }
    if (t0.z > t1.z) { double temp = t0.z; t0.z = t1.z; t1.z = temp; }
    
    double tmin = fmax(fmax(t0.x, t0.y), t0.z);
    double tmax = fmin(fmin(t1.x, t1.y), t1.z);
    
    if (tmax < 0 || tmin > tmax || tmin < t_min || tmin > t_max) return 0;
    
    double t = tmin > 0 ? tmin : tmax;
    if (t < t_min || t > t_max) return 0;
    
    hit->t = t;
    hit->point = ray_at(ray, t);
    hit->hit = 1;
    hit->material = &box->material;
    
    // Calculer la normale de la face touchée
    Vec3 center = vec3_mul(vec3_add(box->box.min_corner, box->box.max_corner), 0.5);
    Vec3 local_point = vec3_sub(hit->point, center);
    Vec3 extent = vec3_mul(vec3_sub(box->box.max_corner, box->box.min_corner), 0.5);
    
    Vec3 normal = vec3_zero();
    double min_dist = INFINITY;
    
    // Déterminer quelle face a été touchée
    double dist = fabs(extent.x - fabs(local_point.x));
    if (dist < min_dist) {
        min_dist = dist;
        normal = vec3_create(local_point.x > 0 ? 1 : -1, 0, 0);
    }
    
    dist = fabs(extent.y - fabs(local_point.y));
    if (dist < min_dist) {
        min_dist = dist;
        normal = vec3_create(0, local_point.y > 0 ? 1 : -1, 0);
    }
    
    dist = fabs(extent.z - fabs(local_point.z));
    if (dist < min_dist) {
        normal = vec3_create(0, 0, local_point.z > 0 ? 1 : -1);
    }
    
    hit_record_set_face_normal(hit, ray, normal);
    
    return 1;
}

// Fonction principale d'intersection
int geometry_hit(GeometryObject* obj, Ray ray, double t_min, double t_max, HitRecord* hit) {
    if (!obj) return 0;
    
    switch (obj->type) {
        case GEOMETRY_SPHERE:
            return sphere_hit(obj, ray, t_min, t_max, hit);
        case GEOMETRY_PLANE:
            return plane_hit(obj, ray, t_min, t_max, hit);
        case GEOMETRY_BOX:
            return box_hit(obj, ray, t_min, t_max, hit);
        default:
            return 0;
    }
}

// Libération de mémoire
void geometry_free(GeometryObject* obj) {
    if (obj) free(obj);
}

// Création d'une scène
Scene* scene_create(void) {
    Scene* scene = malloc(sizeof(Scene));
    if (!scene) return NULL;
    
    scene->objects = NULL;
    scene->count = 0;
    return scene;
}

// Ajout d'un objet à la scène
void scene_add(Scene* scene, GeometryObject* obj) {
    if (!scene || !obj) return;
    
    obj->next = scene->objects;
    scene->objects = obj;
    scene->count++;
}

// Test d'intersection avec toute la scène
int scene_hit(Scene* scene, Ray ray, double t_min, double t_max, HitRecord* hit) {
    HitRecord temp_rec;
    int hit_anything = 0;
    double closest_so_far = t_max;
    
    GeometryObject* obj = scene->objects;
    while (obj) {
        if (geometry_hit(obj, ray, t_min, closest_so_far, &temp_rec)) {
            hit_anything = 1;
            closest_so_far = temp_rec.t;
            *hit = temp_rec;
        }
        obj = obj->next;
    }
    
    return hit_anything;
}

// Libération de la scène
void scene_free(Scene* scene) {
    if (!scene) return;
    
    GeometryObject* obj = scene->objects;
    while (obj) {
        GeometryObject* next = obj->next;
        geometry_free(obj);
        obj = next;
    }
    
    free(scene);
}

// Construction de la scène de la "Sixième Station"
Scene* create_chihiro_sixth_station_scene(void) {
    Scene* scene = scene_create();
    if (!scene) return NULL;
    
    // Sol de la gare (plan légèrement en dessous de l'eau)
    GeometryObject* floor = create_plane(
        vec3_create(0, -1.5, 0),
        vec3_create(0, 1, 0),
        chihiro_station_floor()
    );
    scene_add(scene, floor);
    
    // Surface d'eau
    add_water_surface_to_scene(scene, -0.5);
    
    // Quai de la gare
    add_station_platform_to_scene(scene);
    
    // Ajout du train fantôme
    add_train_to_scene(scene, vec3_create(5, -0.2, 0), 0);
    
    // Lumières de la gare
    add_station_lights_to_scene(scene);
    
    // Pas de sphère de ciel - utiliser l'arrière-plan du renderer
    
    return scene;
}

void add_water_surface_to_scene(Scene* scene, double water_level) {
    // Plan d'eau avec material spécial
    GeometryObject* water = create_plane(
        vec3_create(0, water_level, 0),
        vec3_create(0, 1, 0),
        chihiro_water()
    );
    scene_add(scene, water);
}

void add_station_platform_to_scene(Scene* scene) {
    // Quai principal
    GeometryObject* platform = create_box(
        vec3_create(-10, -0.5, -5),
        vec3_create(10, 0, 5),
        chihiro_station_walls()
    );
    scene_add(scene, platform);
    
    // Colonnes de support
    for (int i = -8; i <= 8; i += 4) {
        GeometryObject* column = create_box(
            vec3_create(i - 0.2, 0, -4.8),
            vec3_create(i + 0.2, 4, -4.4),
            chihiro_station_walls()
        );
        scene_add(scene, column);
    }
}

void add_train_to_scene(Scene* scene, Vec3 position, double rotation) {
    (void)rotation; // Pour l'instant on ignore la rotation
    
    // Corps principal du train - plus gros et mieux positionné
    GeometryObject* train_body = create_box(
        vec3_create(position.x - 3, position.y, position.z - 1.5),
        vec3_create(position.x + 3, position.y + 2, position.z + 1.5),
        chihiro_train_body()
    );
    scene_add(scene, train_body);
    
    // Fenêtres du train - plus visibles
    for (int i = 0; i < 4; i++) {
        GeometryObject* window = create_box(
            vec3_create(position.x - 2.5 + i * 1.2, position.y + 0.4, position.z + 1.45),
            vec3_create(position.x - 1.8 + i * 1.2, position.y + 1.6, position.z + 1.55),
            chihiro_train_window()
        );
        scene_add(scene, window);
        
        // Fenêtres de l'autre côté
        window = create_box(
            vec3_create(position.x - 2.5 + i * 1.2, position.y + 0.4, position.z - 1.55),
            vec3_create(position.x - 1.8 + i * 1.2, position.y + 1.6, position.z - 1.45),
            chihiro_train_window()
        );
        scene_add(scene, window);
    }
}

void add_station_lights_to_scene(Scene* scene) {
    // Lampadaires le long du quai - plus nombreux et plus lumineux
    for (int i = -8; i <= 8; i += 2) {
        // Poteau
        GeometryObject* pole = create_box(
            vec3_create(i - 0.1, 0, 4.0),
            vec3_create(i + 0.1, 4, 4.2),
            chihiro_station_walls()
        );
        scene_add(scene, pole);
        
        // Lumière principale
        GeometryObject* light = create_sphere(
            vec3_create(i, 3.5, 4.1),
            0.3,
            chihiro_lamplight()
        );
        scene_add(scene, light);
    }
    
    // Lumières additionnelles pour éclairer la scène
    GeometryObject* ambient_light = create_sphere(
        vec3_create(0, 8, 0),
        0.5,
        material_emissive(color_create(0.8, 0.8, 1.0), 3.0)
    );
    scene_add(scene, ambient_light);
    
    // Éclairage du train
    GeometryObject* train_light = create_sphere(
        vec3_create(8, 2, 0),
        0.2,
        material_emissive(color_create(1.0, 1.0, 0.8), 4.0)
    );
    scene_add(scene, train_light);
}