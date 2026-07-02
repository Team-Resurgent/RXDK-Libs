#pragma once

int test_matrix_multiply_identity(void);
int test_matrix_multiply_known(void);
int test_matrix_multiply_aliased_pM1(void);
int test_matrix_multiply_aliased_pM2(void);
int test_matrix_product4x4_known(void);
int test_matrix_multiply_vs_product4x4(void);
int test_vec3_transform_coord_identity(void);
int test_vec3_transform_coord_translate(void);
int test_lookat_identity_axes(void);
int test_perspective_known(void);
int test_vec3_project_identity(void);
int test_vec3_project_cube_corner(void);
int test_matrix_transpose_known(void);
int test_matrix_transpose_self_aliased(void);
int test_matrix_inverse_diagonal(void);
int test_inverse4x4_diagonal(void);
