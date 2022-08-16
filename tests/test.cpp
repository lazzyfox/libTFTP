#include <gtest/gtest.h>
#include "gmock/gmock.h"

#include <variant>
#include <iostream>

#include "../src/libTFTP.hpp"

//  Data type check
char test[] = {'t', 'e', 's', 't'};
char read_RQ[] = {'\0', '1', 'a', 'k','.', 't', 'x', 't'};

//  ReadFile Data
TEST(ReadData, CharData) {
  ReadFileData<char> data{4};
  memcpy (data.data, &test, sizeof(test));
  for (auto count = 0; count < 4; ++ count) {
    EXPECT_EQ (test[count], data.data[count]);
  }
}

//  Base packet dataset test
TEST(Packet, CharData) {
  Packet<char> data{4};
  memcpy (data.packet, &test, sizeof(test));
  for (auto count = 0; count < 4; ++ count) {
    EXPECT_EQ (test[count], data.packet[count]);
  }
}

// Client request packet test
// TEST(ReadPacket, ReadRq) {
//   ReadPacket data;
//   memcpy (data.packet, &test, sizeof(test));
//   // for (auto count = 0; count < 4; ++ count) {
//   //   EXPECT_EQ (test[count], data.packet[count]);
//   // }
// }

// Data transfer packet
TEST (DataPacket, CharData) {
  DataPacket<char> msg{4};
  SendData<char> data{4};
  memcpy (msg.data, &test, sizeof(test));
  data.setData(1, &msg);
  // for ()
}

// RFC 783-1350 ACK packet
TEST (ACKPacket, CharData) {
  DataPacket<char> msg{4};
  SendData<char> data{4};
  memcpy (msg.data, &test, sizeof(test));
  data.setData(1, &msg);
  // for ()
}


GTEST_API_ int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
