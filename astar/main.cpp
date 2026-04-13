/*
 * A* Pathfinding — Single-file build
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

/* ================================================================== */
/*  SECTION 1: Grid representation                                     */
/*  - Each cell holds a positive weight (cost to ENTER that cell).     */
/*  - A weight of 0 means the cell is impassable (wall).               */
/*  - Movement: 4-connected (up/down/left/right), no diagonals.        */
/*  - Cost of moving from A to neighbor B = B's weight.                */
/* ================================================================== */

typedef struct {
    int rows;
    int cols;
    float *weights;   /* row-major: weights[r * cols + c] */
} Grid;

typedef struct {
    int r;
    int c;
} Vec2i;

typedef struct {
    Vec2i *points;    /* heap-allocated array of waypoints */
    int    length;    /* number of waypoints (0 = no path found) */
    float  cost;      /* total path cost (-1 if no path) */
} Path;

/* ------------------------------------------------------------------ */
/*  Grid helpers                                                       */
/* ------------------------------------------------------------------ */

static inline int grid_index(const Grid *g, int r, int c) {
    return r * g->cols + c;
}

static inline int grid_in_bounds(const Grid *g, int r, int c) {
    return r >= 0 && r < g->rows && c >= 0 && c < g->cols;
}

static inline int grid_passable(const Grid *g, int r, int c) {
    return grid_in_bounds(g, r, c) && g->weights[grid_index(g, r, c)] > 0.0f;
}

static inline float grid_cost(const Grid *g, int r, int c) {
    return g->weights[grid_index(g, r, c)];
}

static Grid grid_create(int rows, int cols, float default_weight) {
    Grid g;
    g.rows = rows;
    g.cols = cols;
    g.weights = (float *)malloc(sizeof(float) * rows * cols);
    for (int i = 0; i < rows * cols; i++) {
        g.weights[i] = default_weight;
    }
    return g;
}

static void grid_destroy(Grid *g) {
    free(g->weights);
    g->weights = NULL;
}

static void grid_set(Grid *g, int r, int c, float w) {
    g->weights[grid_index(g, r, c)] = w;
}

/* Pretty-print the grid with an optional path overlay.
 * '.' = open, '#' = wall, '*' = path, 'S' = start, 'E' = end. */
static void grid_print(const Grid *g, const Path *path) {
    char *overlay = (char *)calloc(g->rows * g->cols, sizeof(char));

    if (path && path->length > 0) {
        for (int i = 0; i < path->length; i++) {
            int idx = grid_index(g, path->points[i].r, path->points[i].c);
            overlay[idx] = '*';
        }
        int si = grid_index(g, path->points[0].r, path->points[0].c);
        int ei = grid_index(g, path->points[path->length - 1].r,
                                path->points[path->length - 1].c);
        overlay[si] = 'S';
        overlay[ei] = 'E';
    }

    for (int r = 0; r < g->rows; r++) {
        for (int c = 0; c < g->cols; c++) {
            int idx = grid_index(g, r, c);
            if (overlay[idx]) {
                printf(" %c ", overlay[idx]);
            } else if (g->weights[idx] <= 0.0f) {
                printf(" # ");
            } else {
                printf("%2.0f ", g->weights[idx]);
            }
        }
        printf("\n");
    }
    free(overlay);
}

/* ------------------------------------------------------------------ */
/*  Heuristic                                                          */
/*  Manhattan distance scaled by the minimum edge weight on the grid.  */
/* ------------------------------------------------------------------ */

static inline float heuristic(Vec2i a, Vec2i b, float min_weight) {
    return (float)(abs(a.r - b.r) + abs(a.c - b.c)) * min_weight;
}

static float grid_min_weight(const Grid *g) {
    float mn = INFINITY;
    for (int i = 0; i < g->rows * g->cols; i++) {
        if (g->weights[i] > 0.0f && g->weights[i] < mn) {
            mn = g->weights[i];
        }
    }
    return mn;
}

/* ------------------------------------------------------------------ */
/*  Path helpers                                                       */
/* ------------------------------------------------------------------ */

static Path path_none(void) {
    Path p;
    p.points = NULL;
    p.length = 0;
    p.cost   = -1.0f;
    return p;
}

static void path_destroy(Path *p) {
    free(p->points);
    p->points = NULL;
    p->length = 0;
}

/* ------------------------------------------------------------------ */
/*  Direction table for 4-connected neighbors                          */
/* ------------------------------------------------------------------ */

static const int DIR_DR[4] = { -1,  1,  0,  0 };
static const int DIR_DC[4] = {  0,  0, -1,  1 };


/* ================================================================== */
/*  SECTION 2: A* Implementation (YOUR WORK GOES HERE)                 */
/*                                                                     */
/*  Data structures you'll need:                                       */
/*                                                                     */
/*  1. A min-heap (priority queue) keyed on f = g + h.                 */
/*     Each entry needs at least: (row, col, f_score).                 */
/*     Hint: a simple binary heap in a flat array works great.         */
/*                                                                     */
/*  2. A g_score array (float, rows*cols) — best cost found so far     */
/*     to reach each cell. Initialize to FLT_MAX.                      */
/*                                                                     */
/*  3. A came_from array (int, rows*cols) — stores the flat index of   */
/*     the predecessor for each cell, so you can reconstruct the path. */
/*     Initialize to -1.                                               */
/*                                                                     */
/*  Algorithm sketch (fill in the details!):                           */
/*                                                                     */
/*    - Push start onto the open set with g=0, f=h(start, goal).       */
/*    - While open set is not empty:                                   */
/*        - Pop the node with lowest f.                                */
/*        - If it's the goal, reconstruct and return the path.         */
/*        - For each 4-connected neighbor:                             */
/*            - Skip if out of bounds or impassable.                   */
/*            - Compute tentative_g = g[current] + cost(neighbor).     */
/*            - If tentative_g < g[neighbor]:                          */
/*                - Update g[neighbor], came_from[neighbor].           */
/*                - Push neighbor onto the open set.                   */
/*    - If the loop ends without finding the goal, return path_none(). */
/*                                                                     */
/*  Notes:                                                             */
/*    - "Lazy deletion": it's fine to push duplicates onto the heap    */
/*      and skip stale entries when you pop (check if popped g > g[]). */
/*    - Use the heuristic() function defined above.                    */
/*    - Precompute min_weight with grid_min_weight() once.             */
/*    - Reconstruct the path by walking came_from[] from goal to start,*/
/*      then reversing the result.                                     */
/* ================================================================== */

Path astar(const Grid *grid, Vec2i start, Vec2i goal) {
    (void)grid;
    (void)start;
    (void)goal;

    /* TODO: implement A* here */

    return path_none();
}


/* ================================================================== */
/*  SECTION 3: Test Suite                                              */
/* ================================================================== */

static int g_tests_run    = 0;
static int g_tests_passed = 0;

#define ASSERT_MSG(cond, fmt, ...)                                         \
    do {                                                                   \
        if (!(cond)) {                                                     \
            printf("  FAIL: " fmt "\n", ##__VA_ARGS__);                    \
            return 0;                                                      \
        }                                                                  \
    } while (0)

#define RUN_TEST(fn)                                                       \
    do {                                                                   \
        g_tests_run++;                                                     \
        printf("[ ] %s\n", #fn);                                           \
        if (fn()) {                                                        \
            g_tests_passed++;                                              \
            printf("[PASS] %s\n", #fn);                                    \
        } else {                                                           \
            printf("[FAIL] %s\n", #fn);                                    \
        }                                                                  \
        printf("\n");                                                      \
    } while (0)

static int float_eq(float a, float b) {
    return fabsf(a - b) < 0.001f;
}

static int path_is_contiguous(const Path *p) {
    for (int i = 1; i < p->length; i++) {
        int dr = abs(p->points[i].r - p->points[i - 1].r);
        int dc = abs(p->points[i].c - p->points[i - 1].c);
        if (dr + dc != 1) return 0;
    }
    return 1;
}

static float path_recompute_cost(const Grid *g, const Path *p) {
    if (p->length == 0) return -1.0f;
    float total = g->weights[grid_index(g, p->points[0].r, p->points[0].c)];
    for (int i = 1; i < p->length; i++) {
        total += g->weights[grid_index(g, p->points[i].r, p->points[i].c)];
    }
    return total;
}

/* Test 1: start equals goal */
static int test_start_is_goal(void) {
    Grid g = grid_create(3, 3, 1.0f);
    Vec2i s = {1, 1}, e = {1, 1};
    Path p = astar(&g, s, e);

    ASSERT_MSG(p.length == 1,
        "Expected path length 1, got %d", p.length);
    ASSERT_MSG(p.points[0].r == 1 && p.points[0].c == 1,
        "Expected point (1,1)");
    ASSERT_MSG(float_eq(p.cost, 1.0f),
        "Expected cost 1.0 (cost of standing on start), got %.2f", p.cost);

    path_destroy(&p);
    grid_destroy(&g);
    return 1;
}

/*
 * Test 2: Straight line on a uniform grid
 *
 *   1 1 1 1 1       S * * * E
 *   1 1 1 1 1       . . . . .
 *   1 1 1 1 1       . . . . .
 *
 *   (0,0) -> (0,4), all weights 1.  Cost = 5.
 */
static int test_straight_line(void) {
    Grid g = grid_create(3, 5, 1.0f);
    Vec2i s = {0, 0}, e = {0, 4};
    Path p = astar(&g, s, e);

    ASSERT_MSG(p.length == 5,
        "Expected path length 5, got %d", p.length);
    ASSERT_MSG(float_eq(p.cost, 5.0f),
        "Expected cost 5.0, got %.2f", p.cost);
    ASSERT_MSG(path_is_contiguous(&p), "Path is not contiguous");
    ASSERT_MSG(float_eq(path_recompute_cost(&g, &p), p.cost),
        "Reported cost doesn't match recomputed cost");

    grid_print(&g, &p);
    path_destroy(&p);
    grid_destroy(&g);
    return 1;
}

/*
 * Test 3: Wall forces a detour
 *
 *   1 1 1 1 1       S . . . .
 *   1 0 0 0 1       . # # # .
 *   1 1 1 1 1       . . . . E
 *
 *   (0,0) -> (2,4).  Optimal cost = 7.
 */
static int test_wall_detour(void) {
    Grid g = grid_create(3, 5, 1.0f);
    grid_set(&g, 1, 1, 0.0f);
    grid_set(&g, 1, 2, 0.0f);
    grid_set(&g, 1, 3, 0.0f);

    Vec2i s = {0, 0}, e = {2, 4};
    Path p = astar(&g, s, e);

    ASSERT_MSG(p.length == 7,
        "Expected path length 7, got %d", p.length);
    ASSERT_MSG(float_eq(p.cost, 7.0f),
        "Expected cost 7.0, got %.2f", p.cost);
    ASSERT_MSG(path_is_contiguous(&p), "Path is not contiguous");

    grid_print(&g, &p);
    path_destroy(&p);
    grid_destroy(&g);
    return 1;
}

/*
 * Test 4: No path exists
 *
 *   1 0 1       S # .
 *   1 0 1       . # E
 */
static int test_no_path(void) {
    Grid g = grid_create(2, 3, 1.0f);
    grid_set(&g, 0, 1, 0.0f);
    grid_set(&g, 1, 1, 0.0f);

    Vec2i s = {0, 0}, e = {1, 2};
    Path p = astar(&g, s, e);

    ASSERT_MSG(p.length == 0,
        "Expected no path (length 0), got %d", p.length);
    ASSERT_MSG(float_eq(p.cost, -1.0f),
        "Expected cost -1, got %.2f", p.cost);

    grid_print(&g, &p);
    path_destroy(&p);
    grid_destroy(&g);
    return 1;
}

/*
 * Test 5: Weighted — cheapest path IS the shortest
 *
 *   1  1  1  1  1
 *   1  9  9  9  1
 *   1  1  1  1  1
 *
 *   (0,0) -> (0,4).  Straight across row 0: cost = 5.
 */
static int test_weighted_simple(void) {
    Grid g = grid_create(3, 5, 1.0f);
    grid_set(&g, 1, 1, 9.0f);
    grid_set(&g, 1, 2, 9.0f);
    grid_set(&g, 1, 3, 9.0f);

    Vec2i s = {0, 0}, e = {0, 4};
    Path p = astar(&g, s, e);

    ASSERT_MSG(float_eq(p.cost, 5.0f),
        "Expected cost 5.0, got %.2f", p.cost);
    ASSERT_MSG(path_is_contiguous(&p), "Path is not contiguous");
    ASSERT_MSG(float_eq(path_recompute_cost(&g, &p), p.cost),
        "Reported cost doesn't match recomputed cost");

    grid_print(&g, &p);
    path_destroy(&p);
    grid_destroy(&g);
    return 1;
}

/*
 * Test 6: Weighted — longer path is cheaper
 *
 *   1  1  9  1  1
 *   1  1  9  1  1
 *   1  1  1  1  1
 *
 *   (0,0) -> (0,4).
 *   Straight across row 0: 1+1+9+1+1 = 13
 *   Detour via row 2:      cost = 9
 */
static int test_weighted_detour(void) {
    Grid g = grid_create(3, 5, 1.0f);
    grid_set(&g, 0, 2, 9.0f);
    grid_set(&g, 1, 2, 9.0f);

    Vec2i s = {0, 0}, e = {0, 4};
    Path p = astar(&g, s, e);

    ASSERT_MSG(float_eq(p.cost, 9.0f),
        "Expected cost 9.0 (detour around heavy column), got %.2f", p.cost);
    ASSERT_MSG(path_is_contiguous(&p), "Path is not contiguous");
    ASSERT_MSG(float_eq(path_recompute_cost(&g, &p), p.cost),
        "Reported cost doesn't match recomputed cost");

    grid_print(&g, &p);
    path_destroy(&p);
    grid_destroy(&g);
    return 1;
}

/*
 * Test 7: Maze-like corridor
 *
 *   . . . . # . . .
 *   . # # . # . # .
 *   . # . . . . # .
 *   . # . # # # # .
 *   . . . . . . . .
 *
 *   (0,0) -> (0,7), all passable cells weight 1.  Optimal cost = 16.
 */
static int test_maze(void) {
    Grid g = grid_create(5, 8, 1.0f);

    grid_set(&g, 0, 4, 0.0f);
    grid_set(&g, 1, 1, 0.0f);
    grid_set(&g, 1, 2, 0.0f);
    grid_set(&g, 1, 4, 0.0f);
    grid_set(&g, 1, 6, 0.0f);
    grid_set(&g, 2, 1, 0.0f);
    grid_set(&g, 2, 6, 0.0f);
    grid_set(&g, 3, 1, 0.0f);
    grid_set(&g, 3, 3, 0.0f);
    grid_set(&g, 3, 4, 0.0f);
    grid_set(&g, 3, 5, 0.0f);
    grid_set(&g, 3, 6, 0.0f);

    Vec2i s = {0, 0}, e = {0, 7};
    Path p = astar(&g, s, e);

    ASSERT_MSG(p.length > 0, "Expected a valid path");
    ASSERT_MSG(float_eq(p.cost, 16.0f),
        "Expected cost 16.0, got %.2f", p.cost);
    ASSERT_MSG(path_is_contiguous(&p), "Path is not contiguous");
    ASSERT_MSG(float_eq(path_recompute_cost(&g, &p), p.cost),
        "Reported cost doesn't match recomputed cost");

    grid_print(&g, &p);
    path_destroy(&p);
    grid_destroy(&g);
    return 1;
}

/* ================================================================== */
/*  Main                                                               */
/* ================================================================== */

int main(void) {
    printf("========================================\n");
    printf("  A* Pathfinding Test Suite\n");
    printf("========================================\n\n");

    RUN_TEST(test_start_is_goal);
    RUN_TEST(test_straight_line);
    RUN_TEST(test_wall_detour);
    RUN_TEST(test_no_path);
    RUN_TEST(test_weighted_simple);
    RUN_TEST(test_weighted_detour);
    RUN_TEST(test_maze);

    printf("========================================\n");
    printf("  Results: %d / %d passed\n", g_tests_passed, g_tests_run);
    printf("========================================\n");

    return (g_tests_passed == g_tests_run) ? 0 : 1;
}
