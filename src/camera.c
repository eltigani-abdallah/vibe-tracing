#include "camera.h"
#include <math.h>
#include <stdio.h>

#define PI 3.14159265358979323846

Camera camera_create(vec3 lookfrom, vec3 lookat, vec3 vup,
                     double vfov_deg, int img_width, int img_height) {
    Camera cam;
    cam.img_width  = img_width;
    cam.img_height = img_height;
    cam.origin     = lookfrom;

    double theta = vfov_deg * PI / 180.0;
    double h     = tan(theta / 2.0);
    double aspect = (double)img_width / (double)img_height;
    double viewport_h = 2.0 * h;
    double viewport_w = aspect * viewport_h;

    cam.w = vec3_normalize(vec3_sub(lookfrom, lookat));
    cam.u = vec3_normalize(vec3_cross(vup, cam.w));
    cam.v = vec3_cross(cam.w, cam.u);

    cam.horizontal = vec3_scale(cam.u, viewport_w);
    cam.vertical   = vec3_scale(cam.v, viewport_h);

    cam.lower_left = vec3_sub(
                        vec3_sub(
                            vec3_sub(cam.origin, vec3_scale(cam.horizontal, 0.5)),
                            vec3_scale(cam.vertical, 0.5)
                        ),
                        cam.w
                    );
    return cam;
}

Ray ray_for_pixel(const Camera* cam, int px, int py) {
    double s = (px + 0.5) / (double)cam->img_width;
    double t = ((cam->img_height - 1 - py) + 0.5) / (double)cam->img_height;

    vec3 target = vec3_add(
                    vec3_add(cam->lower_left, vec3_scale(cam->horizontal, s)),
                    vec3_scale(cam->vertical, t)
                  );

    vec3 direction = vec3_sub(target, cam->origin);
    return ray_create(cam->origin, direction);
}

/* Version coordonnées réelles pour l’anti-aliasing sous-pixel */
Ray ray_for_pixel_sub(const Camera* cam, double fx, double fy) {
    double s = fx / (double)cam->img_width;
    double t = ((double)(cam->img_height - 1) - fy + 1.0) / (double)cam->img_height;

    vec3 target = vec3_add(
                    vec3_add(cam->lower_left, vec3_scale(cam->horizontal, s)),
                    vec3_scale(cam->vertical, t)
                  );

    vec3 direction = vec3_sub(target, cam->origin);
    return ray_create(cam->origin, direction);
}

#ifdef RUN_TESTS
/* ---- helpers test ---- */
static int nearly_equal(double a, double b) {
    double d = a - b;
    return (d < 1e-9) && (d > -1e-9);
}

#define TEST(name, condition) \
    do { \
        if (condition) printf("  OK  %s\n", name); \
        else           printf("  FAIL %s\n", name); \
    } while(0)

int main(void) {
    printf("Tests Camera + Ray\n");
    printf("=====================================\n");

    vec3 lookfrom = vec3_create(0.0, 0.0, 3.0);
    vec3 lookat   = vec3_create(0.0, 0.0, 0.0);
    vec3 vup      = vec3_create(0.0, 1.0, 0.0);

    Camera cam = camera_create(lookfrom, lookat, vup, 90.0, 800, 600);

    printf("\nParametres camera (vfov=90, 800x600)\n");
    vec3_print("  origin     ", cam.origin);
    vec3_print("  lower_left ", cam.lower_left);
    vec3_print("  horizontal ", cam.horizontal);
    vec3_print("  vertical   ", cam.vertical);
    vec3_print("  u (droite) ", cam.u);
    vec3_print("  v (haut)   ", cam.v);
    vec3_print("  w (arriere)", cam.w);

    printf("\nTests base orthonormee\n");
    TEST("u unitaire",       nearly_equal(vec3_length(cam.u), 1.0));
    TEST("v unitaire",       nearly_equal(vec3_length(cam.v), 1.0));
    TEST("w unitaire",       nearly_equal(vec3_length(cam.w), 1.0));
    TEST("u.v = 0",          nearly_equal(vec3_dot(cam.u, cam.v), 0.0));
    TEST("u.w = 0",          nearly_equal(vec3_dot(cam.u, cam.w), 0.0));
    TEST("v.w = 0",          nearly_equal(vec3_dot(cam.v, cam.w), 0.0));
    double aspect     = 800.0 / 600.0;
    double viewport_h = 2.0;
    double viewport_w = aspect * viewport_h;
    TEST("|horizontal|==viewport_w", nearly_equal(vec3_length(cam.horizontal), viewport_w));
    TEST("|vertical|==viewport_h",   nearly_equal(vec3_length(cam.vertical),   viewport_h));
    TEST("w == (0,0,1)", nearly_equal(cam.w.x, 0.0) && nearly_equal(cam.w.y, 0.0) && nearly_equal(cam.w.z, 1.0));

    printf("\nTests ray_for_pixel\n");
    Ray r_center    = ray_for_pixel(&cam, 400, 300);
    Ray r_top_left  = ray_for_pixel(&cam,   0,   0);
    Ray r_top_right = ray_for_pixel(&cam, 799,   0);
    Ray r_bot_left  = ray_for_pixel(&cam,   0, 599);
    Ray r_bot_right = ray_for_pixel(&cam, 799, 599);

    TEST("origine == lookfrom",          vec3_length(vec3_sub(r_center.origin, lookfrom)) < 1e-9);
    TEST("rayon central vers -Z",        r_center.direction.z < 0.0);
    double cx = r_center.direction.x; if (cx < 0) cx = -cx;
    double cy = r_center.direction.y; if (cy < 0) cy = -cy;
    TEST("rayon central centre x~0",     cx < 5e-3);
    TEST("rayon central centre y~0",     cy < 5e-3);
    TEST("dir normalisee centre",        nearly_equal(vec3_length(r_center.direction),    1.0));
    TEST("dir normalisee HG",            nearly_equal(vec3_length(r_top_left.direction),  1.0));
    TEST("dir normalisee HD",            nearly_equal(vec3_length(r_top_right.direction), 1.0));
    TEST("dir normalisee BG",            nearly_equal(vec3_length(r_bot_left.direction),  1.0));
    TEST("dir normalisee BD",            nearly_equal(vec3_length(r_bot_right.direction), 1.0));
    TEST("symetrie HG/HD x opposes",     nearly_equal(r_top_left.direction.x, -r_top_right.direction.x));
    TEST("symetrie HG/HD y egaux",       nearly_equal(r_top_left.direction.y,  r_top_right.direction.y));
    TEST("symetrie HG/BG x egaux",       nearly_equal(r_top_left.direction.x,  r_bot_left.direction.x));
    TEST("symetrie HG/BG y opposes",     nearly_equal(r_top_left.direction.y, -r_bot_left.direction.y));

    printf("\nRayons aux coins:\n");
    ray_print("  centre    ", r_center);
    ray_print("  haut-gauche", r_top_left);
    ray_print("  haut-droit ", r_top_right);
    ray_print("  bas-gauche ", r_bot_left);
    ray_print("  bas-droit  ", r_bot_right);

    printf("\nTest ray_at\n");
    Ray r = ray_create(vec3_create(0.0, 0.0, 0.0), vec3_create(1.0, 0.0, 0.0));
    vec3 p = ray_at(r, 5.0);
    TEST("ray_at t=5 -> (5,0,0)", nearly_equal(p.x, 5.0) && nearly_equal(p.y, 0.0) && nearly_equal(p.z, 0.0));
    vec3 p0 = ray_at(r, 0.0);
    TEST("ray_at t=0 -> origin",  nearly_equal(p0.x, 0.0));

    printf("\n=====================================\n");
    printf("Tests termines\n");
    return 0;
}
#endif /* RUN_TESTS */
