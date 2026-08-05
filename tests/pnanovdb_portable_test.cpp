#include <cstdio>
#include <cstring>
#include <cmath>

#include "nanodb/nanovdb/tools/PNanoVDBPortableTest.h"

using namespace nanovdb::tools;

int main() {
    int rc = PNanoVDBPortableTest::run();
    return rc;
}
