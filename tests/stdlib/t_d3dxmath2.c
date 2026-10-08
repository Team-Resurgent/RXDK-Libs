/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/* The second C user of the D3DX inline math helpers -- see t_d3dxmath.c. */
#include "rxdk_test.h"
#include <xtl.h>

static D3DXVECTOR2 *(*const add2)(D3DXVECTOR2 *, CONST D3DXVECTOR2 *, CONST D3DXVECTOR2 *) = D3DXVec2Add;

int main(void)
{
    D3DXVECTOR2 a = { -1.0f, 0.5f }, b = { 1.0f, 0.5f }, r2;
    D3DXVECTOR4 v = { 1.0f, 2.0f, 3.0f, 4.0f }, r4;

    add2(&r2, &a, &b);
    CHECK(r2.x == 0.0f && r2.y == 1.0f, "D3DXVec2Add through a pointer (second file)");
    D3DXVec4Scale(&r4, &v, 2.0f);
    CHECK(r4.x == 2.0f && r4.w == 8.0f, "D3DXVec4Scale");
    CHECK(D3DXVec4Dot(&v, &v) == 30.0f, "D3DXVec4Dot");

    CHECK_DONE("d3dxmath2");
    return 0;
}
