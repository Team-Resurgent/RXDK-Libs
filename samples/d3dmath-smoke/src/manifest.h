#pragma once

#include "tests.h"

typedef struct D3DMathSmokeTest {
    const char* name;
    int (*run)(void);
} D3DMathSmokeTest;

static const D3DMathSmokeTest kD3DMathSmokeTests[] = {
    { "matrix_multiply_identity",     test_matrix_multiply_identity },
    { "matrix_multiply_known",        test_matrix_multiply_known },
    { "matrix_multiply_aliased_pM1",  test_matrix_multiply_aliased_pM1 },
    { "matrix_multiply_aliased_pM2",  test_matrix_multiply_aliased_pM2 },
    { "matrix_product4x4_known",      test_matrix_product4x4_known },
    { "matrix_multiply_vs_product4x4", test_matrix_multiply_vs_product4x4 },
    { "vec3_transform_coord_identity", test_vec3_transform_coord_identity },
    { "vec3_transform_coord_translate", test_vec3_transform_coord_translate },
    { "lookat_identity_axes",         test_lookat_identity_axes },
    { "perspective_known",            test_perspective_known },
    { "vec3_project_identity",        test_vec3_project_identity },
    { "vec3_project_cube_corner",     test_vec3_project_cube_corner },
    { "matrix_transpose_known",       test_matrix_transpose_known },
    { "matrix_transpose_self_aliased", test_matrix_transpose_self_aliased },
    { "matrix_inverse_diagonal",      test_matrix_inverse_diagonal },
    { "inverse4x4_diagonal",          test_inverse4x4_diagonal },
};

#define kD3DMathSmokeTestCount (sizeof(kD3DMathSmokeTests) / sizeof(kD3DMathSmokeTests[0]))
