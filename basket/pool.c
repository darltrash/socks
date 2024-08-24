#include "basket.h"
#include "float.h"
#include "stdlib.h"

// This implements an incomplete BVH structure, specialized for triangles

static void expand_box(Box* box, const Vertex* v) {
    for (int i = 0; i < 3; i++) {
        if (v->position[i] < box->min[i]) box->min[i] = v->position[i];
        if (v->position[i] > box->max[i]) box->max[i] = v->position[i];
    }
}

static Box calculate_box(Triangle* triangles, int count) {
    Box box = {
        .min = { FLT_MAX,  FLT_MAX,  FLT_MAX}, 
        .max = {-FLT_MAX, -FLT_MAX, -FLT_MAX}
    };

    for (int i = 0; i < count; ++i) {
        expand_box(&box, &triangles[i].a);
        expand_box(&box, &triangles[i].b);
        expand_box(&box, &triangles[i].c);
    }

    return box;
}

static void split_triangles(Triangle* triangles, int count, int axis, int* mid) {
    float mid_point = 0.0f;

    for (int i = 0; i < count; ++i) {
        mid_point += (
            triangles[i].a.position[axis] + 
            triangles[i].b.position[axis] + 
            triangles[i].c.position[axis]
        ) / 3.0f;
    }

    mid_point /= count;

    int left = 0;
    int right = count - 1;

    while (left <= right) {
        while (
            (
                triangles[left].a.position[axis] + 
                triangles[left].b.position[axis] + 
                triangles[left].c.position[axis]
            ) / 3.0f < mid_point) 
                left++;

        while (
            (
                triangles[right].a.position[axis] + 
                triangles[right].b.position[axis] + 
                triangles[right].c.position[axis]
            ) / 3.0f >= mid_point) 
                right--;
        
        if (left <= right) {
            Triangle temp = triangles[left];
            triangles[left] = triangles[right];
            triangles[right] = temp;
            left++;
            right--;
        }
    }

    *mid = left;
}

static int build_bvh(VertexPool* node, Triangle* triangles, int count, int depth) {
    if (count == 0) 
        return 1;

    node = (VertexPool*)calloc(1, sizeof(VertexPool));
    node->bounds = calculate_box(triangles, count);
    node->triangles = triangles;
    node->triangle_count = count;

    if (count <= 2) 
        return 0;

    int axis = depth % 3;
    int mid;
    split_triangles(triangles, count, axis, &mid);

    build_bvh(node->left, triangles, mid, depth + 1);
    build_bvh(node->right, triangles + mid, count - mid, depth + 1);

    return 0;
}


int pool_init(VertexPool* pool, Triangle* triangles, u32 count) {
    return build_bvh(pool, triangles, count, 0);
}

void pool_free(VertexPool* node) {
    if (node) {
        pool_free(node->left);
        pool_free(node->right);
    }
}