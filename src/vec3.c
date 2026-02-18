#include "vec3.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>

// ============================================================
//  Macro de test (compare deux doubles avec tolérance)
// ============================================================
#define EPSILON 1e-9

static int nearly_equal(double a, double b) {
    return fabs(a - b) < EPSILON;
}

static void test_pass(const char* name) {
    printf("  ✅ %s\n", name);
}

static void test_fail(const char* name) {
    printf("  ❌ ÉCHEC: %s\n", name);
}

#define TEST(name, condition) \
    do { if (condition) test_pass(name); else test_fail(name); } while(0)

// ============================================================
//  Suite de tests
// ============================================================

void test_constructeurs(void) {
    printf("\n📐 Test constructeurs\n");

    vec3 v = vec3_create(1.0, 2.0, 3.0);
    TEST("vec3_create x", nearly_equal(v.x, 1.0));
    TEST("vec3_create y", nearly_equal(v.y, 2.0));
    TEST("vec3_create z", nearly_equal(v.z, 3.0));

    vec3 z = vec3_zero();
    TEST("vec3_zero", nearly_equal(z.x, 0.0) && nearly_equal(z.y, 0.0) && nearly_equal(z.z, 0.0));

    vec3 o = vec3_one();
    TEST("vec3_one", nearly_equal(o.x, 1.0) && nearly_equal(o.y, 1.0) && nearly_equal(o.z, 1.0));
}

void test_operations_base(void) {
    printf("\n➕ Test opérations de base\n");

    vec3 a = vec3_create(1.0, 2.0, 3.0);
    vec3 b = vec3_create(4.0, 5.0, 6.0);

    // Addition
    vec3 s = vec3_add(a, b);
    TEST("vec3_add", nearly_equal(s.x, 5.0) && nearly_equal(s.y, 7.0) && nearly_equal(s.z, 9.0));

    // Soustraction
    vec3 d = vec3_sub(a, b);
    TEST("vec3_sub", nearly_equal(d.x, -3.0) && nearly_equal(d.y, -3.0) && nearly_equal(d.z, -3.0));

    // Négation
    vec3 n = vec3_neg(a);
    TEST("vec3_neg", nearly_equal(n.x, -1.0) && nearly_equal(n.y, -2.0) && nearly_equal(n.z, -3.0));

    // Multiplication scalaire
    vec3 m = vec3_scale(a, 2.0);
    TEST("vec3_scale", nearly_equal(m.x, 2.0) && nearly_equal(m.y, 4.0) && nearly_equal(m.z, 6.0));

    // Division scalaire
    vec3 q = vec3_div(a, 2.0);
    TEST("vec3_div", nearly_equal(q.x, 0.5) && nearly_equal(q.y, 1.0) && nearly_equal(q.z, 1.5));

    // Multiplication composante par composante
    vec3 p = vec3_mul(a, b);
    TEST("vec3_mul", nearly_equal(p.x, 4.0) && nearly_equal(p.y, 10.0) && nearly_equal(p.z, 18.0));
}

void test_produits(void) {
    printf("\n✖️  Test produits\n");

    vec3 a = vec3_create(1.0, 2.0, 3.0);
    vec3 b = vec3_create(4.0, 5.0, 6.0);

    // Produit scalaire : 1*4 + 2*5 + 3*6 = 4+10+18 = 32
    double dot = vec3_dot(a, b);
    TEST("vec3_dot", nearly_equal(dot, 32.0));

    // Produit vectoriel : (2*6-3*5, 3*4-1*6, 1*5-2*4) = (-3, 6, -3)
    vec3 cross = vec3_cross(a, b);
    TEST("vec3_cross x", nearly_equal(cross.x, -3.0));
    TEST("vec3_cross y", nearly_equal(cross.y,  6.0));
    TEST("vec3_cross z", nearly_equal(cross.z, -3.0));

    // Le produit vectoriel doit être perpendiculaire à a et b
    TEST("cross ⊥ a", nearly_equal(vec3_dot(cross, a), 0.0));
    TEST("cross ⊥ b", nearly_equal(vec3_dot(cross, b), 0.0));
}

void test_longueur_normalisation(void) {
    printf("\n📏 Test longueur et normalisation\n");

    // Longueur de (3, 4, 0) = 5
    vec3 v = vec3_create(3.0, 4.0, 0.0);
    TEST("vec3_length_sq", nearly_equal(vec3_length_sq(v), 25.0));
    TEST("vec3_length",    nearly_equal(vec3_length(v), 5.0));

    // Normalisation : doit avoir une longueur de 1
    vec3 n = vec3_normalize(v);
    TEST("vec3_normalize length = 1", nearly_equal(vec3_length(n), 1.0));
    TEST("vec3_normalize x",          nearly_equal(n.x, 3.0 / 5.0));
    TEST("vec3_normalize y",          nearly_equal(n.y, 4.0 / 5.0));

    // Normalisation du vecteur zéro → retourne (0,0,0) sans crash
    vec3 nz = vec3_normalize(vec3_zero());
    TEST("vec3_normalize(zero) safe", nearly_equal(vec3_length(nz), 0.0));
}

void test_utilitaires(void) {
    printf("\n🛠️  Test utilitaires raytracer\n");

    // Lerp à t=0 → a, t=1 → b
    vec3 a = vec3_create(0.0, 0.0, 0.0);
    vec3 b = vec3_create(2.0, 4.0, 6.0);
    vec3 mid = vec3_lerp(a, b, 0.5);
    TEST("vec3_lerp t=0.5", nearly_equal(mid.x, 1.0) && nearly_equal(mid.y, 2.0) && nearly_equal(mid.z, 3.0));
    TEST("vec3_lerp t=0",   nearly_equal(vec3_lerp(a, b, 0.0).x, 0.0));
    TEST("vec3_lerp t=1",   nearly_equal(vec3_lerp(a, b, 1.0).x, 2.0));

    // Réflexion : vecteur vers le bas, normale vers le haut → rebond vers le haut
    vec3 incident = vec3_normalize(vec3_create(1.0, -1.0, 0.0));
    vec3 normal   = vec3_create(0.0,  1.0, 0.0);
    vec3 reflected = vec3_reflect(incident, normal);
    TEST("vec3_reflect y > 0", reflected.y > 0.0);
    TEST("vec3_reflect longueur", nearly_equal(vec3_length(reflected), vec3_length(incident)));

    // vec3_near_zero
    TEST("vec3_near_zero vrai",  vec3_near_zero(vec3_create(1e-10, 0.0, 0.0)));
    TEST("vec3_near_zero faux", !vec3_near_zero(vec3_create(1.0, 0.0, 0.0)));

    // Distance
    vec3 p1 = vec3_create(0.0, 0.0, 0.0);
    vec3 p2 = vec3_create(3.0, 4.0, 0.0);
    TEST("vec3_dist", nearly_equal(vec3_dist(p1, p2), 5.0));
}

void test_couleurs(void) {
    printf("\n🎨 Test couleurs\n");

    // color_to_byte
    TEST("byte(0.0) = 0",   color_to_byte(0.0)  == 0);
    TEST("byte(1.0) = 255",  color_to_byte(1.0)  == 255);
    TEST("byte(0.5) = 127",  color_to_byte(0.5)  == 127);
    TEST("byte(-0.5) clamp", color_to_byte(-0.5) == 0);
    TEST("byte(1.5) clamp",  color_to_byte(1.5)  == 255);

    // color_to_argb : rouge pur → 0xFFFF0000
    color red = color_create(1.0, 0.0, 0.0);
    TEST("argb rouge", color_to_argb(red) == 0xFFFF0000);

    // Vert pur → 0xFF00FF00
    color green = color_create(0.0, 1.0, 0.0);
    TEST("argb vert", color_to_argb(green) == 0xFF00FF00);

    // Bleu pur → 0xFF0000FF
    color blue = color_create(0.0, 0.0, 1.0);
    TEST("argb bleu", color_to_argb(blue) == 0xFF0000FF);
}

// ============================================================
//  main de test
// ============================================================
#ifdef RUN_TESTS
int main(void) {
    printf("🔬 Tests vec3 - Librairie Mathématique\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");

    // Affichage de démonstration
    printf("\n📌 Exemple d'utilisation :\n");
    vec3 a = vec3_create(1.0, 2.0, 3.0);
    vec3 b = vec3_create(4.0, 5.0, 6.0);
    vec3_print("  a", a);
    vec3_print("  b", b);
    vec3_print("  a + b", vec3_add(a, b));
    vec3_print("  a × b", vec3_cross(a, b));
    vec3_print("  norm(a)", vec3_normalize(a));
    printf("  |a| = %.4f\n", vec3_length(a));
    printf("  a·b = %.4f\n", vec3_dot(a, b));

    // Tests unitaires
    test_constructeurs();
    test_operations_base();
    test_produits();
    test_longueur_normalisation();
    test_utilitaires();
    test_couleurs();

    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("✅ Tests terminés\n");
    return 0;
}
#endif /* RUN_TESTS */