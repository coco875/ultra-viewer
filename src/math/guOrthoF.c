#include <libultraship.h>
#include <math.h>

void guOrthoF(float m[4][4], float left, float right, float bottom, float top, float near, float far,
              float scale) {
    int row;
    int col;
    guMtxIdentF(m);
    m[0][0] = 2 / (right - left);
    m[1][1] = 2 / (top - bottom);
    m[2][2] = -2 / (far - near);
    m[3][0] = -(right + left) / (right - left);
    m[3][1] = -(top + bottom) / (top - bottom);
    m[3][2] = -(far + near) / (far - near);
    m[3][3] = 1;
    for (row = 0; row < 4; row++) {
        for (col = 0; col < 4; col++) {
            m[row][col] *= scale;
        }
    }
}

void guOrtho(Mtx *m, float left, float right, float bottom, float top, float near, float far,
             float scale) {
    float sp28[4][4];
    guOrthoF(sp28, left, right, bottom, top, near, far, scale);
    guMtxF2L(sp28, m);
}

void guScaleF(float mf[4][4], float x, float y, float z) {
    guMtxIdentF(mf);
    mf[0][0] = x;
    mf[1][1] = y;
    mf[2][2] = z;
    mf[3][3] = 1.0;
}

void guScale(Mtx *m, float x, float y, float z) {
    float mf[4][4];
    guScaleF(mf, x, y, z);
    guMtxF2L(mf, m);
}

void guMtxScaleRotate(Mtx *m, float x, float y, float z, float a, float xr, float yr, float zr) {
    float mf[4][4];
    guScaleF(mf, x, y, z);
    guRotateF(mf, a, xr, yr, zr);
    guMtxF2L(mf, m);
}