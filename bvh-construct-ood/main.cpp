#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <cfloat>

#define min(a, b) ((a < b) ? a : b)
#define max(a, b) ((a > b) ? a : b)

// ============================================================================
// Core types
// ============================================================================

struct Vec3 {
    float x, y, z;
};

// Arithmetic — implement these first, everything else depends on them
Vec3 vec3_add(Vec3 a, Vec3 b) {
    return { a.x + b.x, a.y + b.y, a.z + b.z };
};
Vec3 vec3_sub(Vec3 a, Vec3 b) {
    return { a.x - b.x, a.y - b.y, a.z - b.z };
};
Vec3 vec3_mul(Vec3 a, float s) {
    return { a.x * s, a.y * s, a.z * s };
};
float vec3_dot(Vec3 a, Vec3 b) {
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}
Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return { (a.y * b.z - a.z * b.y), (a.z * b.x - a.x * b.z), (a.x * b.y - a.y * b.x)};
}
Vec3 vec3_min(Vec3 a, Vec3 b) {
    return { min(a.x, b.x), min(a.y, b.y), min(a.z, b.z) };
};
Vec3 vec3_max(Vec3 a, Vec3 b) {
    return { max(a.x, b.x), max(a.y, b.y), max(a.z, b.z) };
};

struct Ray {
    Vec3 origin;
    Vec3 dir;
};

struct AABB {
    Vec3 min;
    Vec3 max;
};

// Returns the centroid of the box — useful during build
Vec3 aabb_centroid(AABB box) {
    return { (box.max.x - box.max.x)/2.f, (box.max.y - box.min.y)/2.f, (box.max.z - box.min.z)/2.f };
}

// Expand `box` to also contain `other`
AABB aabb_union(AABB box, AABB other);

// Expand `box` to contain point `p`
AABB aabb_expand(AABB box, Vec3 p);

// Slab test. Returns true if ray hits the box within [t_min, t_max).
// Writes the entry t into `t_out` on hit.
// Use the standard min/max-of-slabs approach, handle inv_dir = inf gracefully.
bool aabb_intersect(AABB box, Ray ray, float t_min, float t_max, float *t_out) {
    *t_out = 0.f;
    return false;
}

// ============================================================================
// Triangle primitive
// ============================================================================

struct Triangle {
    Vec3 v0, v1, v2;
};

// Möller–Trumbore. Returns true on hit within [t_min, t_max).
// Writes hit t into `t_out`.
bool triangle_intersect(Triangle tri, Ray ray, float t_min, float t_max, float *t_out) {
    *t_out = 0.f;
    return false;
}

// Returns the AABB that tightly encloses the triangle
AABB triangle_bounds(Triangle tri);

// ============================================================================
// BVH
// ============================================================================

// A node is either:
//   internal — left/right are valid child indices, prim_count == 0
//   leaf     — prim_offset is an index into the reordered prim array,
//              prim_count > 0
struct BVHNode {
    AABB bounds;
    uint32_t left;        // child index (internal) or unused (leaf)
    uint32_t right;       // child index (internal) or unused (leaf)
    uint32_t prim_offset; // first prim index (leaf only)
    uint32_t prim_count;  // 0 = internal node
};

struct BVH {
    BVHNode *nodes;
    uint32_t node_count;
    uint32_t node_capacity;

    // The BVH reorders primitives during build so that each leaf's
    // prims are contiguous.  This array is the reordered copy.
    Triangle *prims;
    uint32_t  prim_count;
};

struct HitRecord {
    float t;
    uint32_t prim_index; // index into bvh.prims[]
    bool hit;
};

// Build a BVH over `tris[0..tri_count)`.
// Strategy: longest-axis midpoint split, recurse until leaf has <= max_prims.
// The BVH takes ownership of its own copies of the data — caller keeps theirs.
void bvh_build(BVH *bvh, const Triangle *tris, uint32_t tri_count, uint32_t max_prims_per_leaf) {
}

// Find the closest hit along `ray` in [t_min, t_max).
HitRecord bvh_intersect(const BVH *bvh, Ray ray, float t_min, float t_max) {
    return {};
}

// Free everything allocated by bvh_build.
void bvh_free(BVH *bvh) {
}


// ============================================================================
// Tests
// ============================================================================

static void test_vec3_basics() {
    {
        Vec3 a = {1, 2, 3};
        Vec3 b = {4, 5, 6};
        Vec3 r = vec3_add(a, b);
        bool result = (r.x == 5.0f && r.y == 7.0f && r.z == 9.0f);
        assert(result);
    }
    {
        Vec3 a = {1, 0, 0};
        Vec3 b = {0, 1, 0};
        Vec3 r = vec3_cross(a, b);
        bool result = (r.x == 0.0f && r.y == 0.0f && r.z == 1.0f);
        assert(result);
    }
    printf("[PASS] vec3 basics\n");
}

static void test_aabb_intersect() {
    AABB box = {{-1, -1, -1}, {1, 1, 1}};

    // Ray along +Z hitting the box
    {
        Ray ray = {{0, 0, -5}, {0, 0, 1}};
        float t_hit = 0.0f;
        bool hit = aabb_intersect(box, ray, 0.0f, FLT_MAX, &t_hit);
        bool result = (hit && fabsf(t_hit - 4.0f) < 1e-5f);
        assert(result);
    }
    // Ray pointing away — should miss
    {
        Ray ray = {{0, 0, -5}, {0, 0, -1}};
        float t_hit = 0.0f;
        bool hit = aabb_intersect(box, ray, 0.0f, FLT_MAX, &t_hit);
        bool result = (!hit);
        assert(result);
    }
    // Ray origin inside box — t_out should be 0 or the exit slab,
    // depending on your convention.  Entry t is negative, so the
    // first valid t within [0, max) is the exit face.
    {
        Ray ray = {{0, 0, 0}, {0, 0, 1}};
        float t_hit = 0.0f;
        bool hit = aabb_intersect(box, ray, 0.0f, FLT_MAX, &t_hit);
        // Should still report a hit — the ray exits the box at t=1
        bool result = (hit && t_hit >= 0.0f);
        assert(result);
    }
    printf("[PASS] aabb intersect\n");
}

static void test_triangle_intersect() {
    // Flat triangle on the XY plane at z=0
    Triangle tri = {{-1, -1, 0}, {1, -1, 0}, {0, 1, 0}};

    // Direct hit from -Z
    {
        Ray ray = {{0, 0, -5}, {0, 0, 1}};
        float t_hit = 0.0f;
        bool hit = triangle_intersect(tri, ray, 0.0f, FLT_MAX, &t_hit);
        bool result = (hit && fabsf(t_hit - 5.0f) < 1e-5f);
        assert(result);
    }
    // Miss — ray offset to the side
    {
        Ray ray = {{10, 10, -5}, {0, 0, 1}};
        float t_hit = 0.0f;
        bool hit = triangle_intersect(tri, ray, 0.0f, FLT_MAX, &t_hit);
        bool result = (!hit);
        assert(result);
    }
    // Behind the ray — triangle at z=0, ray starts at z=5 going +Z
    {
        Ray ray = {{0, 0, 5}, {0, 0, 1}};
        float t_hit = 0.0f;
        bool hit = triangle_intersect(tri, ray, 0.0f, FLT_MAX, &t_hit);
        bool result = (!hit);
        assert(result);
    }
    printf("[PASS] triangle intersect\n");
}

static void test_bvh_single_triangle() {
    Triangle tris[] = {
        {{-1, -1, 0}, {1, -1, 0}, {0, 1, 0}}
    };

    BVH bvh;
    memset(&bvh, 0, sizeof(bvh));
    bvh_build(&bvh, tris, 1, 1);

    // Hit
    {
        Ray ray = {{0, 0, -3}, {0, 0, 1}};
        HitRecord rec = bvh_intersect(&bvh, ray, 0.0f, FLT_MAX);
        bool result = (rec.hit && fabsf(rec.t - 3.0f) < 1e-5f);
        assert(result);
    }
    // Miss
    {
        Ray ray = {{10, 10, -3}, {0, 0, 1}};
        HitRecord rec = bvh_intersect(&bvh, ray, 0.0f, FLT_MAX);
        bool result = (!rec.hit);
        assert(result);
    }

    bvh_free(&bvh);
    printf("[PASS] bvh single triangle\n");
}

static void test_bvh_two_triangles_closest_hit() {
    // Two parallel triangles along Z.  Near at z=2, far at z=5.
    Triangle tris[] = {
        {{-1, -1, 2}, {1, -1, 2}, {0, 1, 2}},  // near
        {{-1, -1, 5}, {1, -1, 5}, {0, 1, 5}},  // far
    };

    BVH bvh;
    memset(&bvh, 0, sizeof(bvh));
    bvh_build(&bvh, tris, 2, 1);

    // Should return the near triangle
    {
        Ray ray = {{0, 0, 0}, {0, 0, 1}};
        HitRecord rec = bvh_intersect(&bvh, ray, 0.0f, FLT_MAX);
        bool result = (rec.hit && fabsf(rec.t - 2.0f) < 1e-5f);
        assert(result);
    }

    bvh_free(&bvh);
    printf("[PASS] bvh two triangles closest hit\n");
}

static void test_bvh_separated_triangles() {
    // Two triangles far apart on X axis — forces an actual split
    Triangle tris[] = {
        {{-10, -1, 0}, {-8, -1, 0}, {-9, 1, 0}},  // left cluster
        {{ 8,  -1, 0}, {10, -1, 0}, { 9, 1, 0}},   // right cluster
    };

    BVH bvh;
    memset(&bvh, 0, sizeof(bvh));
    bvh_build(&bvh, tris, 2, 1);

    // Hit the right triangle only
    {
        Ray ray = {{9, 0, -5}, {0, 0, 1}};
        HitRecord rec = bvh_intersect(&bvh, ray, 0.0f, FLT_MAX);
        bool result = (rec.hit && fabsf(rec.t - 5.0f) < 1e-5f);
        assert(result);
    }
    // Miss both — ray goes through the gap
    {
        Ray ray = {{0, 0, -5}, {0, 0, 1}};
        HitRecord rec = bvh_intersect(&bvh, ray, 0.0f, FLT_MAX);
        bool result = (!rec.hit);
        assert(result);
    }

    bvh_free(&bvh);
    printf("[PASS] bvh separated triangles\n");
}


int main() {
    test_vec3_basics();
    test_aabb_intersect();
    test_triangle_intersect();
    test_bvh_single_triangle();
    test_bvh_two_triangles_closest_hit();
    test_bvh_separated_triangles();

    printf("\nAll tests passed.\n");
    return 0;
}
