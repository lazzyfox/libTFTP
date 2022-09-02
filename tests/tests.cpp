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
//  char req_data[] = {'0', '1', 'a', 'k', '.', 't', 'x', 't', '0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '0', 't', 's', 'i', 'z', 'e', '0', '0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '0', '5', '1', '2', '0', 't', 'i', 'm', 'e', 'o', 'u', 't', '0', '6', '0'};
//   memcpy (data.packet, &test, sizeof(test));, 
//   // for (auto count = 0; count < 4; ++ count) {
//   //   EXPECT_EQ (test[count], data.packet[count]);
//   // }
// }

TEST(ReadPacket, WriteRq) {
  ReadPacket data;
  char req_data[] = {'0', '2', 'a', 'k', '.', 't', 'x', 't', '0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '0', 't', 's', 'i', 'z', 'e', '0', '5', 'b', 'l', 'k', 's', 'i', 'z', 'e', '0', '5', '1', '2', '0', 't', 'i', 'm', 'e', 'o', 'u', 't', '0', '6', '0'};
  uint16_t net_code{htons(2)};
  memcpy (req_data, &net_code, sizeof(net_code));
  memcpy (data.packet, &req_data, sizeof(req_data));
  data.makeFrameStruct(sizeof(req_data));
  const uint16_t ex_op_code{2};
  uint16_t op_code;
  auto data_frame{std::get<0>(data.packet_frame_structure)};
  switch (data_frame) {
    case TFTPOpeCode::TFTP_OPCODE_READ : op_code = 1; break;
    case TFTPOpeCode::TFTP_OPCODE_WRITE: op_code = 2; break;
    case TFTPOpeCode::TFTP_OPCODE_DATA : op_code = 3; break;
    case TFTPOpeCode::TFTP_OPCODE_ACK : op_code = 4; break;
    case TFTPOpeCode::TFTP_OPCODE_ERROR : op_code = 5; break;
    case TFTPOpeCode::TFTP_OPCODE_OACK : op_code = 6; break;
    default :;
  }
  EXPECT_EQ (op_code, ex_op_code);  
}

// Data transfer packet
// TEST (DataPacket, CharData) {
//   ReadFileData<char> msg{4};
//   SendData<char> data{4};
//char req_data[] = {'0', '3', '0', '1', 't', 'e', 's', 't', '\n', '0'};
//   memcpy (msg.data, &test, sizeof(test));
//   data.setData(1, &msg);
//   // for ()
// }

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
  memcpy(pack_msg, &data.packet[4], 4);
  string msg_str{pack_msg};

  EXPECT_EQ (op_code, ex_op_code);  
  EXPECT_EQ (pack_num, ex_pack_num);  
  EXPECT_STREQ(msg_str.c_str(), ex_str.c_str());
}
//  OACK packet
TEST (OACK, TypicalRequest) {
  const uint16_t ex_op_code{6};
  uint16_t net_op_code, net_val;
  const uint16_t ex_size_val{10};
  const uint16_t ex_blk_val{512};
  const uint16_t ex_timeout_val{6};
  const char zero{'\0'};
  //char req_data[] = {'0', '1', 'a', 'k', '.', 't', 'x', 't', '0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '0', 't', 's', 'i', 'z', 'e', '0', '0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '0', '5', '1', '2', '0', 't', 'i', 'm', 'e', 'o', 'u', 't', '0', '6', '0'};  
  auto t_size = make_pair("tsize", (uint16_t) 10); // Size = 9
  auto blk_size = make_pair("blksize", (uint16_t) 512); // Size = 11
  auto t_out = make_pair("timeout", (uint16_t) 6); // Size = 11
  char pack_t_size[5];
  char pack_blk_size[7];
  char pack_t_out[7];
  const string  str_size{"tsize"};
  const string  str_blk{"blksize"};
  const string  str_timeout{"timeout"};
  
  string  ex_str_blk;
  string  ex_str_timeout;
  
  vector<ReqParam> param{t_size, blk_size, t_out};
  OACKPacket pack{33};
  pack.makeData(&param);
  
  //  Operation code (should be 6)
  memcpy(&net_op_code, pack.packet, sizeof(uint16_t));
  uint16_t const op_code{ntohs(net_op_code)};

  //  Source file size (tsize) 
  memcpy(&pack_t_size, &pack.packet[2], sizeof(pack_t_size));
  const string  pack_str_size{pack_t_size, sizeof(pack_t_size)};
  auto pack_str_zero = pack.packet[7];
  memcpy(&net_val, &pack.packet[8], sizeof(net_val));
  const uint16_t net_size{ntohs(net_val)};
  auto pack_val_zero = pack.packet[10];

  //  Transfer block size
  memcpy(&pack_blk_size, &pack.packet[11], sizeof(pack_blk_size));
  const string  pack_str_blk{pack_blk_size, sizeof(pack_blk_size)};
  auto blk_str_zero = pack.packet[18];
  memcpy(&net_val, &pack.packet[19], sizeof(net_val));
  const uint16_t net_blk_size{ntohs(net_val)};
  auto blk_val_zero = pack.packet[21];
  
  //  Transfer timeout 
  memcpy(&pack_t_out, &pack.packet[22], sizeof(pack_t_out));
  const string  pack_str_timeout{pack_t_out, sizeof(pack_t_out)};
  auto t_out_str_zero = pack.packet[29];
  memcpy(&net_val, &pack.packet[30], sizeof(net_val));
  const uint16_t blk_timeout{ntohs(net_val)};
  auto t_out_val_zero = pack.packet[32];

  EXPECT_EQ (op_code, ex_op_code);  

  EXPECT_STREQ(pack_str_size.c_str(), str_size.c_str());
  EXPECT_EQ (pack_str_zero, zero);
  EXPECT_EQ (net_size, ex_size_val);
  EXPECT_EQ (pack_val_zero, zero);
  
  EXPECT_STREQ(pack_str_blk.c_str(), str_blk.c_str());
  EXPECT_EQ (blk_str_zero, zero);
  EXPECT_EQ (net_blk_size, ex_blk_val);
  EXPECT_EQ (blk_val_zero, zero);

  EXPECT_STREQ(pack_str_timeout.c_str(), str_timeout.c_str());
  EXPECT_EQ (t_out_str_zero, zero);
  EXPECT_EQ (blk_timeout, ex_timeout_val);
  EXPECT_EQ (t_out_val_zero, zero);
}


GTEST_API_ int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
