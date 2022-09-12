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
TEST(ReadPacket, ReadRq_Oldfashioned) {
  ReadPacket data;
  char req_data[] {'0', '1', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0'};
  const string ex_file_name{"ak.txt"};
  uint16_t net_code{htons(1)};
  const string ex_tsize_str{"tsize"};
  const string ex_blksize_str{"blksize"};
  const string ex_timeout_str{"timeout"};
  memcpy (req_data, &net_code, sizeof(net_code));
  memcpy (data.packet, &req_data, sizeof(req_data));
  auto make_struct {data.makeFrameStruct(sizeof(req_data))};
  
  auto op_code {(uint16_t)std::get<0>(data.packet_frame_structure)};
  auto error_code{std::get<1>(data.packet_frame_structure)};
  auto transfer_mode{std::get<2>(data.packet_frame_structure).value()};
  auto block_number{std::get<3>(data.packet_frame_structure)};
  auto block_begin{std::get<4>(data.packet_frame_structure).value()};
  auto block_end{std::get<5>(data.packet_frame_structure).value()};
  auto file_name{std::get<6>(data.packet_frame_structure)};

  
  EXPECT_TRUE (make_struct);
  EXPECT_FALSE (data.req_params);
  EXPECT_EQ (op_code, (uint16_t)TFTPOpeCode::TFTP_OPCODE_READ);
  EXPECT_FALSE (error_code);
  EXPECT_EQ (transfer_mode, TFTPMode::netascii);
  EXPECT_FALSE (block_number);
  EXPECT_EQ (block_begin, 2);
  EXPECT_EQ (block_end, 8);
  EXPECT_STREQ (file_name.value().c_str(), ex_file_name.c_str());
}


TEST(ReadPacket, WriteRq_Oldfashioned) {
  ReadPacket data;
  char req_data[] {'0', '2', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0'};
  uint16_t net_code{htons(2)};
  const string ex_file_name{"ak.txt"};
  const string ex_tsize_str{"tsize"};
  const string ex_blksize_str{"blksize"};
  const string ex_timeout_str{"timeout"};
  memcpy (req_data, &net_code, sizeof(net_code));
  memcpy (data.packet, &req_data, sizeof(req_data));
  auto make_struct {data.makeFrameStruct(sizeof(req_data))};
  
  auto op_code {(uint16_t)std::get<0>(data.packet_frame_structure)};
  auto error_code{std::get<1>(data.packet_frame_structure)};
  auto transfer_mode{std::get<2>(data.packet_frame_structure).value()};
  auto block_number{std::get<3>(data.packet_frame_structure)};
  auto block_begin{std::get<4>(data.packet_frame_structure).value()};
  auto block_end{std::get<5>(data.packet_frame_structure).value()};
  auto file_name{std::get<6>(data.packet_frame_structure)};

  EXPECT_TRUE (make_struct);
  EXPECT_FALSE (data.req_params);
  EXPECT_EQ (op_code, (uint16_t)TFTPOpeCode::TFTP_OPCODE_WRITE);
  EXPECT_FALSE (error_code);
  EXPECT_EQ (transfer_mode, TFTPMode::netascii);
  EXPECT_FALSE (block_number);
  EXPECT_EQ (block_begin, 2);
  EXPECT_EQ (block_end, 8);
  EXPECT_STREQ (file_name.value().c_str(), ex_file_name.c_str());
}

TEST(ReadPacket, ReadRq_Negotiation) {
  ReadPacket data;
  char req_data[] {'0', '1', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i', 'z', 'e', '\0', '0', '\0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '\0', '5', '1', '2', '\0', 't', 'i', 'm', 'e', 'o', 'u', 't', '\0', '6', '\0'};
  const string ex_file_name{"ak.txt"};
  uint16_t net_code{htons(1)};
  const string ex_tsize_str{"tsize"};
  const string ex_blksize_str{"blksize"};
  const string ex_timeout_str{"timeout"};
  memcpy (req_data, &net_code, sizeof(net_code));
  memcpy (data.packet, &req_data, sizeof(req_data));
  auto make_struct {data.makeFrameStruct(sizeof(req_data))};
  
  auto op_code {(uint16_t)std::get<0>(data.packet_frame_structure)};
  auto error_code{std::get<1>(data.packet_frame_structure)};
  auto transfer_mode{std::get<2>(data.packet_frame_structure).value()};
  auto block_number{std::get<3>(data.packet_frame_structure)};
  auto block_begin{std::get<4>(data.packet_frame_structure).value()};
  auto block_end{std::get<5>(data.packet_frame_structure).value()};
  auto file_name{std::get<6>(data.packet_frame_structure)};

  auto add_param_vec{data.req_params.value()};
  auto vec_tsize {add_param_vec.at(0)}; 
  auto vec_blksize{add_param_vec.at(1)};
  auto vec_timeout{add_param_vec.at(2)};

  EXPECT_TRUE (make_struct);
  EXPECT_TRUE (data.req_params);
  EXPECT_EQ (data.req_params.value().size(), 3);
  EXPECT_EQ (op_code, (uint16_t)TFTPOpeCode::TFTP_OPCODE_READ);
  EXPECT_FALSE (error_code);
  EXPECT_EQ (transfer_mode, TFTPMode::netascii);
  EXPECT_FALSE (block_number);
  EXPECT_EQ (block_begin, 2);
  EXPECT_EQ (block_end, 8);
  EXPECT_STREQ (file_name.value().c_str(), ex_file_name.c_str());

  EXPECT_STREQ (vec_tsize.first.c_str(), ex_tsize_str.c_str());
  EXPECT_EQ (vec_tsize.second, 0);

  EXPECT_STREQ (vec_blksize.first.c_str(), ex_blksize_str.c_str());
  EXPECT_EQ (vec_blksize.second, 512);

  EXPECT_STREQ (vec_timeout.first.c_str(), ex_timeout_str.c_str());
  EXPECT_EQ (vec_timeout.second, 6);
}

TEST(ReadPacket, WriteRq_Negotiation) {
  ReadPacket data;
  char req_data[] {'0', '2', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i', 'z', 'e', '\0', '5', '\0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '\0', '5', '1', '2', '\0', 't', 'i', 'm', 'e', 'o', 'u', 't', '\0', '6', '\0'};
  uint16_t net_code{htons(2)};
  const string ex_file_name{"ak.txt"};
  const string ex_tsize_str{"tsize"};
  const string ex_blksize_str{"blksize"};
  const string ex_timeout_str{"timeout"};
  memcpy (req_data, &net_code, sizeof(net_code));
  memcpy (data.packet, &req_data, sizeof(req_data));
  auto make_struct {data.makeFrameStruct(sizeof(req_data))};
  
  auto op_code {(uint16_t)std::get<0>(data.packet_frame_structure)};
  auto error_code{std::get<1>(data.packet_frame_structure)};
  auto transfer_mode{std::get<2>(data.packet_frame_structure).value()};
  auto block_number{std::get<3>(data.packet_frame_structure)};
  auto block_begin{std::get<4>(data.packet_frame_structure).value()};
  auto block_end{std::get<5>(data.packet_frame_structure).value()};
  auto file_name{std::get<6>(data.packet_frame_structure)};

  auto add_param_vec{data.req_params.value()};
  auto vec_tsize {add_param_vec.at(0)}; 
  auto vec_blksize{add_param_vec.at(1)};
  auto vec_timeout{add_param_vec.at(2)};

  EXPECT_TRUE (make_struct);
  EXPECT_TRUE (data.req_params);
  EXPECT_EQ (data.req_params.value().size(), 3);
  EXPECT_EQ (op_code, (uint16_t)TFTPOpeCode::TFTP_OPCODE_WRITE);
  EXPECT_FALSE (error_code);
  EXPECT_EQ (transfer_mode, TFTPMode::netascii);
  EXPECT_FALSE (block_number);
  EXPECT_EQ (block_begin, 2);
  EXPECT_EQ (block_end, 8);
  EXPECT_STREQ (file_name.value().c_str(), ex_file_name.c_str());

  EXPECT_STREQ (vec_tsize.first.c_str(), ex_tsize_str.c_str());
  EXPECT_EQ (vec_tsize.second, 5);

  EXPECT_STREQ (vec_blksize.first.c_str(), ex_blksize_str.c_str());
  EXPECT_EQ (vec_blksize.second, 512);

  EXPECT_STREQ (vec_timeout.first.c_str(), ex_timeout_str.c_str());
  EXPECT_EQ (vec_timeout.second, 6);
}

TEST(ReadPacket, ReadRq_Oldfashioned_WrongFN) {
  ReadPacket data;
  char req_data[] {'0', '1', 'a', '\0', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0'};
  uint16_t net_code{htons(1)};
  memcpy (req_data, &net_code, sizeof(net_code));
  memcpy (data.packet, &req_data, sizeof(req_data));
  auto make_struct {data.makeFrameStruct(sizeof(req_data))};
  
  EXPECT_FALSE (make_struct);
}

TEST(ReadPacket, ReadRq_Negotiation_WrongTSizeVal) {
  ReadPacket data;
  char req_data[] {'0', '1', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i', 'z', 'e', '\0', '\0', '\0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '\0', '5', '1', '2', '\0', 't', 'i', 'm', 'e', 'o', 'u', 't', '\0', '6', '\0'};
  uint16_t net_code{htons(1)};
  memcpy (req_data, &net_code, sizeof(net_code));
  memcpy (data.packet, &req_data, sizeof(req_data));
  auto make_struct {data.makeFrameStruct(sizeof(req_data))};
  
  EXPECT_FALSE (make_struct);
}

TEST(ReadPacket, WriteRq_Negotiation_WrongTSizeVal) {
  ReadPacket data;
  char req_data[] {'0', '2', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i', 'z', 'e', '\0', '\0', '\0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '\0', '5', '1', '2', '\0', 't', 'i', 'm', 'e', 'o', 'u', 't', '\0', '6', '\0'};
  const string ex_file_name{"ak.txt"};
  uint16_t net_code{htons(2)};
  const string ex_tsize_str{"tsize"};
  const string ex_blksize_str{"blksize"};
  const string ex_timeout_str{"timeout"};
  memcpy (req_data, &net_code, sizeof(net_code));
  memcpy (data.packet, &req_data, sizeof(req_data));
  auto make_struct {data.makeFrameStruct(sizeof(req_data))};
  
  EXPECT_FALSE (make_struct);
}


// Data transfer packet
TEST (DataPacket, CharData) {
  const uint16_t ex_opcode{htons(3)};
  const uint16_t ex_pack_number{htons(1)};
  uint16_t opcode, pack_number;
  const string_view test_msg{"test12"};
  const string ex_data_str{test_msg};
  ReadFileData<char> msg{6};
  msg.setData(test_msg);
  SendData<char> data{6};
  data.setData(1, &msg);

  memcpy(&opcode, data.packet, sizeof(uint16_t));
  memcpy(&pack_number, &data.packet[2], sizeof(uint16_t));
  string pack_msg {&data.packet[4], 6};

  EXPECT_EQ (opcode, ex_opcode);
  EXPECT_EQ (pack_number, ex_pack_number);
  EXPECT_STREQ(pack_msg.c_str(), ex_data_str.c_str());
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
  auto t_size = make_pair("tsize", (uint16_t) 10); // Size = 9
  auto blk_size = make_pair("blksize", (uint16_t) 512); // Size = 11
  auto t_out = make_pair("timeout", (uint16_t) 6); // Size = 11
  char pack_t_size[5];
  char pack_blk_size[7];
  char pack_t_out[7];
  const string  str_size{"tsize"};
  const string  str_blk{"blksize"};
  const string  str_timeout{"timeout"};
  const string ex_str_size_val{std::to_string(10)};
  const string ex_str_blk_val{std::to_string(512)};
  const string ex_str_tout_val{std::to_string(6)};


  string  ex_str_blk;
  string  ex_str_timeout;
  
  vector<ReqParam> param{t_size, blk_size, t_out};
  OACKPacket pack{&param};
  
  //  Operation code (should be 6)
  memcpy(&net_op_code, pack.packet, sizeof(uint16_t));
  uint16_t const op_code{ntohs(net_op_code)};

  //  Source file size (tsize) 
  memcpy(&pack_t_size, &pack.packet[2], sizeof(pack_t_size));
  const string  pack_str_size{pack_t_size, sizeof(pack_t_size)};
  auto pack_str_zero = pack.packet[7];
  const string str_size_val{&pack.packet[8], 2};
  auto pack_val_zero = pack.packet[10];

  //  Transfer block size
  memcpy(&pack_blk_size, &pack.packet[11], sizeof(pack_blk_size));
  const string  pack_str_blk{pack_blk_size, sizeof(pack_blk_size)};
  auto blk_str_zero = pack.packet[18];
  const string str_blk_val{&pack.packet[19], 3};
  auto blk_val_zero = pack.packet[22];
  
  //  Transfer timeout 
  memcpy(&pack_t_out, &pack.packet[23], sizeof(pack_t_out));
  const string  pack_str_timeout{pack_t_out, sizeof(pack_t_out)};
  auto t_out_str_zero = pack.packet[30];
  const string str_tout_val{&pack.packet[31], 1};
  auto t_out_val_zero = pack.packet[32];

  EXPECT_EQ (op_code, ex_op_code);  

  EXPECT_STREQ(pack_str_size.c_str(), str_size.c_str());
  EXPECT_EQ (pack_str_zero, zero);
  EXPECT_STREQ (str_size_val.c_str(), ex_str_size_val.c_str());
  EXPECT_EQ (pack_val_zero, zero);
  
  EXPECT_STREQ(pack_str_blk.c_str(), str_blk.c_str());
  EXPECT_EQ (blk_str_zero, zero);
  EXPECT_STREQ (str_blk_val.c_str(), ex_str_blk_val.c_str());
  EXPECT_EQ (blk_val_zero, zero);

  EXPECT_STREQ(pack_str_timeout.c_str(), str_timeout.c_str());
  EXPECT_EQ (t_out_str_zero, zero);
  EXPECT_STREQ (str_tout_val.c_str(), ex_str_tout_val.c_str());
  EXPECT_EQ (t_out_val_zero, zero);
}


GTEST_API_ int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
