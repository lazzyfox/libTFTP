#include <gtest/gtest.h>
#include "gmock/gmock.h"

#include <variant>
#include <iostream>

#include "../src/libTFTP.hpp"

//  Data type check
char test[] = {'t', 'e', 's', 't'};

//  ReadFile Data
TEST(ReadData, CharData) {
  ReadFileData<char> data{4};
  memcpy (data.data, &test, sizeof(test));
  for (auto count = 0; count < 4; ++ count) {
    EXPECT_EQ (test[count], data.data[count]);
  }
}

//  ReadFile Data
TEST(Packet, CharData) {
  Packet<char> data{4};
  memcpy (data.packet, &test, sizeof(test));
  for (auto count = 0; count < 4; ++ count) {
    EXPECT_EQ (test[count], data.packet[count]);
  }
}


GTEST_API_ int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
