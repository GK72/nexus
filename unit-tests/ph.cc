#include <gmock/gmock.h>

#include "nxs/ph.h"

TEST(Ph, sample) {
    EXPECT_EQ(nxs::ph(), 1);
}
