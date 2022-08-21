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
  ReadFileData<char> msg{4};
  SendData<char> data{4};
  memcpy (msg.data, &test, sizeof(test));
  data.setData(1, &msg);
  // for ()
}

// RFC 783-1350 ACK packet
TEST (ACKPacket, MinVal) {
  ACKPacket data{0};
  const uint16_t ex_pack_num{0};
  const uint16_t ex_op_code{4};
  uint16_t net_pack_num, net_op_code;
  memcpy(&net_op_code, data.packet, sizeof(uint16_t));
  memcpy(&net_pack_num, &data.packet[2], sizeof(uint16_t));
  auto op_code{ntohs(net_op_code)};
  auto pack_num{ntohs(net_pack_num)};
  EXPECT_EQ (op_code, ex_op_code);  
  EXPECT_EQ (pack_num, ex_pack_num); 
}

TEST (ACKPacket, MaxVal) {
  ACKPacket data{std::numeric_limits<uint16_t>::max()};
  const uint16_t ex_pack_num{std::numeric_limits<uint16_t>::max()};
  const uint16_t ex_op_code{4};
  uint16_t net_pack_num, net_op_code;
  memcpy(&net_op_code, data.packet, sizeof(uint16_t));
  memcpy(&net_pack_num, &data.packet[2], sizeof(uint16_t));
  auto op_code{ntohs(net_op_code)};
  auto pack_num{ntohs(net_pack_num)};
  EXPECT_EQ (op_code, ex_op_code);  
  EXPECT_EQ (pack_num, ex_pack_num);  
}

//  Send data packet tests
TEST (SendData, FullTest_Char) {
  const uint16_t ex_op_code{3};
  const uint16_t ex_pack_num{0};
  uint16_t net_pack_num, net_op_code;
  char pack_msg[4];
  string ex_str{test};
  ReadFileData<char> msg{4};
  memcpy (msg.data, &test, sizeof(test));
  SendData<char> data{4};
  data.setData(0, &msg);
  memcpy(&net_op_code, data.packet, sizeof(uint16_t));
  memcpy(&net_pack_num, &data.packet[2], sizeof(uint16_t));
  auto op_code{ntohs(net_op_code)};
  auto pack_num{ntohs(net_pack_num)};
  memcpy(pack_msg, &data.packet[3], 4);
  string msg_str{pack_msg};
  EXPECT_EQ (op_code, ex_op_code);  
  EXPECT_EQ (pack_num, ex_pack_num);  
  EXPECT_STREQ(msg_str.c_str(), ex_str.c_str());
}

GTEST_API_ int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
