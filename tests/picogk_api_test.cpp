#include <cstdio>
#include <cstring>

// PicoGK API C bindings
#include "PicoGK.h"
#include "PicoGKApiTypes.h"
#include "PicoGKBuild.h"

int main() {
    printf("=== PicoGK API Test ===\n");
    printf("Library: %s\n", PICOGK_LIB_NAME);
    printf("Version: %s\n", PICOGK_LIB_VERSION);
    printf("Build: %s\n", PICOGK_BUILD);

    // Type checks
    printf("PKVector3 sizeof=%zu\n", sizeof(PKVector3));
    printf("PKBBox3 sizeof=%zu\n", sizeof(PKBBox3));
    printf("PKTriangle sizeof=%zu\n", sizeof(PKTriangle));

    // Initialize types
    PKVector3 vec = {1.0f, 2.0f, 3.0f};
    PKBBox3 box = {{0.0f, 0.0f, 0.0f}, {10.0f, 10.0f, 10.0f}};
    PKTriangle tri = {0, 1, 2};

    printf("Vec: (%f, %f, %f)\n", vec.X, vec.Y, vec.Z);
    printf("Box: min(%f,%f,%f) max(%f,%f,%f)\n",
           box.vecMin.X, box.vecMin.Y, box.vecMin.Z,
           box.vecMax.X, box.vecMax.Y, box.vecMax.Z);
    printf("Tri: %d %d %d\n", tri.A, tri.B, tri.C);

    printf("PicoGK API type tests passed.\n");
    return 0;
}
