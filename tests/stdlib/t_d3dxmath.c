/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/* The D3DX math helpers are inline functions in d3dx8math.inl. In C they used
   to come out as a strong external definition in every file that included
   them, so two C files using D3DX failed to link (duplicate D3DXVec2Add, ...).
   This and t_d3dxmath2.c are both C users of the same helpers, linked into
   one title: the suite linking at all is the regression check; the CHECKs
   make sure the helpers still compute. */
#include "rxdk_test.h"
#include <xtl.h>

/* Taking an address forces an out-of-line copy in this file. */
static D3DXVECTOR2 *(*const add2)(D3DXVECTOR2 *, CONST D3DXVECTOR2 *, CONST D3DXVECTOR2 *) = D3DXVec2Add;

int main(void)
{
    D3DXVECTOR2 a = { 1.0f, 2.0f }, b = { 3.0f, 5.0f }, r2;
    D3DXVECTOR3 x = { 1.0f, 0.0f, 0.0f }, y = { 0.0f, 1.0f, 0.0f }, r3;

    add2(&r2, &a, &b);
    CHECK(r2.x == 4.0f && r2.y == 7.0f, "D3DXVec2Add through a pointer");
    CHECK(D3DXVec2Dot(&a, &b) == 13.0f, "D3DXVec2Dot");
    D3DXVec3Cross(&r3, &x, &y);
    CHECK(r3.x == 0.0f && r3.y == 0.0f && r3.z == 1.0f, "D3DXVec3Cross x*y = z");
    CHECK(D3DXVec3Length(&x) == 1.0f, "D3DXVec3Length");

    CHECK_DONE("d3dxmath");
    return 0;
}
