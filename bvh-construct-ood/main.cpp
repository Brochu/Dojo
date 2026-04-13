#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <cfloat>
#include <utility>

#define min(a, b) ((a < b) ? a : b)
#define max(a, b) ((a > b) ? a : b)
#define EPSILON 0.0001f

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
    Vec3 min = { FLT_MAX, FLT_MAX, FLT_MAX };
    Vec3 max = { FLT_MIN, FLT_MIN, FLT_MIN };
};

Vec3 aabb_centroid(AABB box) {
    return vec3_mul(vec3_sub(box.max, box.min), 2.f);
}

AABB aabb_union(AABB box, AABB other) {
    return { vec3_min(box.min, other.min), vec3_max(box.max, other.max) };
}

AABB aabb_expand(AABB box, Vec3 p) {
    return { vec3_min(box.min, p), vec3_max(box.max, p) };
}

bool aabb_contains(AABB box, Vec3 point) {
    return point.x >= box.min.x && point.x <= box.max.x &&
           point.y >= box.min.y && point.y <= box.max.y &&
           point.z >= box.min.z && point.z <= box.max.z;
}

// Slab test. Returns true if ray hits the box within [t_min, t_max).
// Writes the entry t into `t_out` on hit.
// Use the standard min/max-of-slabs approach, handle inv_dir = inf gracefully.
bool aabb_intersect(AABB box, Ray ray, float t_min, float t_max, float *t_out) {
    float start_t = t_min;
    float end_t = t_max;

    for (int32_t axis = 0; axis < 3; ++axis) {
        float o = ((float*)(&ray.origin.x))[axis];
        float d = ((float*)(&ray.dir.x))[axis];
        float val0 = ((float*)(&box.min.x))[axis];
        float val1 = ((float*)(&box.max.x))[axis];
        if (abs(d) < EPSILON) {
            if (o < val0 || o > val1) {
                return false;
            }
            continue;
        }

        float x0 = (val0 - o) / d;
        float x1 = (val1 - o) / d;

        start_t = max(start_t, min(x0, x1));
        end_t = min(end_t, max(x0, x1));
    }

    if (start_t > end_t) {
        *t_out = 0.f;
        return false;
    }

    *t_out = start_t;
    return true;
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
    Vec3 v0v1 = vec3_sub(tri.v1, tri.v0);
    Vec3 v0v2 = vec3_sub(tri.v2, tri.v0);
    Vec3 pvec = vec3_cross(ray.dir, v0v2);

    float det = vec3_dot(v0v1, pvec);
    if (abs(det) < EPSILON) {
        *t_out = 0.f;
        return false;
    }
    float inv_det = 1.f / det;

    Vec3 tvec = vec3_sub(ray.origin, tri.v0);
    float u = vec3_dot(tvec, pvec) * inv_det;
    if (u < 0 || u > 1) {
        *t_out = 0.f;
        return false;
    }

    Vec3 qvec = vec3_cross(tvec, v0v1);
    float v = vec3_dot(ray.dir, qvec) * inv_det;
    if (v < 0 || v + u > 1) {
        *t_out = 0.f;
        return false;
    }

    float t = vec3_dot(v0v2, qvec) * inv_det;
    if (t < t_min || t > t_max) {
        *t_out = 0.f;
        return false;
    }

    *t_out = t;
    return true;
}

// Returns the AABB that tightly encloses the triangle
AABB triangle_bounds(Triangle tri) {
    return { vec3_min(vec3_min(tri.v0, tri.v1), tri.v2), vec3_max(vec3_max(tri.v0, tri.v1), tri.v2) };
}

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

uint32_t bvh_alloc_node(BVH *bvh) {
    if (bvh->node_count >= bvh->node_capacity) {
        bvh->node_capacity *= 2;
        bvh->nodes = (BVHNode*)realloc(bvh->nodes, bvh->node_capacity * sizeof(BVHNode));
    }

    return bvh->node_count++;
};

void bvh_build_inner(BVH *bvh, uint32_t node_idx, uint32_t tri_offset, uint32_t tri_count, uint32_t max_prims_per_leaf);

// Build a BVH over `tris[0..tri_count)`.
// Strategy: longest-axis midpoint split, recurse until leaf has <= max_prims.
// The BVH takes ownership of its own copies of the data — caller keeps theirs.
void bvh_build(BVH *bvh, const Triangle *tris, uint32_t tri_count, uint32_t max_prims_per_leaf) {
    static constexpr int32_t default_node_count = 128;
    bvh->node_capacity = default_node_count;
    bvh->node_count = 0;
    bvh->nodes = (BVHNode*)calloc(default_node_count, sizeof(BVHNode));

    bvh->prim_count = tri_count;
    bvh->prims = (Triangle*)calloc(tri_count, sizeof(Triangle));
    memcpy_s(bvh->prims, sizeof(Triangle) * bvh->prim_count, tris, sizeof(Triangle) * tri_count);

    uint32_t root_idx = bvh_alloc_node(bvh);
    return bvh_build_inner(bvh, root_idx, 0, tri_count, max_prims_per_leaf);
}

void bvh_build_inner(BVH *bvh, uint32_t node_idx, uint32_t tri_offset, uint32_t tri_count, uint32_t max_prims_per_leaf) {
    AABB bounds = {};
    for (uint32_t i = tri_offset; i < (tri_offset + tri_count); i++) {
        bounds = aabb_union(bounds, triangle_bounds(bvh->prims[i]));
    }
    bvh->nodes[node_idx].bounds = bounds;

    if (tri_count <= max_prims_per_leaf) {
        bvh->nodes[node_idx].prim_offset = tri_offset;
        bvh->nodes[node_idx].prim_count = tri_count;
        return;
    }

    Vec3 center = aabb_centroid(bounds);
    uint32_t lo = tri_offset;
    uint32_t hi = tri_offset + tri_count - 1;
    auto partition = [&](float part_val, int axis_idx) {
        while(lo < hi) {
            Vec3 tri_center = aabb_centroid(triangle_bounds(bvh->prims[lo]));
            float target = ((float*)&tri_center)[axis_idx];
            if (target > part_val) {
                std::swap(bvh->prims[lo], bvh->prims[hi]);
                hi--;
            } else {
                lo++;
            }
        }
    };

    float width = bounds.max.x - bounds.min.x;
    float height = bounds.max.y - bounds.min.y;
    float depth = bounds.max.z - bounds.min.z;
    if (width > height && width > depth) {
        partition(center.x, 0);
    }
    else if (height > width && height > depth) {
        partition(center.y, 1);
    }
    else if (depth > width && depth > height) {
        partition(center.z, 2);
    }

    uint32_t left_count = lo - tri_offset;
    uint32_t right_count = (tri_offset + tri_count) - lo;
    if (left_count == 0 || right_count == 0) {
        // In case the partition is the worst case, all on one side
        lo = tri_offset + (tri_count / 2);
    }

    uint32_t left_idx = bvh_alloc_node(bvh);
    uint32_t right_idx = bvh_alloc_node(bvh);
    bvh->nodes[node_idx].left = left_idx;
    bvh->nodes[node_idx].right = right_idx;
    bvh->nodes[node_idx].prim_count = 0;

    bvh_build_inner(bvh, left_idx, tri_offset, lo - tri_offset, max_prims_per_leaf);
    bvh_build_inner(bvh, right_idx, lo, (tri_offset + tri_count) - lo, max_prims_per_leaf);
}

HitRecord bvh_intersect_inner(const BVH* bvh, uint32_t node_idx, Ray ray, float t_min, float t_max);

// Find the closest hit along `ray` in [t_min, t_max).
HitRecord bvh_intersect(const BVH *bvh, Ray ray, float t_min, float t_max) {
    return bvh_intersect_inner(bvh, 0, ray, t_min, t_max);
}

HitRecord bvh_intersect_inner(const BVH* bvh, uint32_t node_idx, Ray ray, float t_min, float t_max) {
    HitRecord rec {};
    float t_out = 0;
    BVHNode *node = &bvh->nodes[node_idx];

    if (node == nullptr || !aabb_intersect(node->bounds, ray, t_min, t_max, &t_out)) {
        rec.hit = false;
        rec.prim_index = 0;
        rec.t = 0.f;
        return rec;
    }

    if (bvh->nodes[node_idx].prim_count > 0) {
        HitRecord current {};
        for (uint32_t i = node->prim_offset; i < node->prim_offset + node->prim_count; i++) {
            //TODO: Need bvh pointer here to access prims
        }
    }

    return rec;
}

// Free everything allocated by bvh_build.
void bvh_free(BVH *bvh) {
    bvh->prim_count = 0;
    free(bvh->prims);
    bvh->prims = nullptr;

    bvh->node_count = 0;
    bvh->node_capacity = 0;
    free(bvh->nodes);
    bvh->nodes = nullptr;
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
