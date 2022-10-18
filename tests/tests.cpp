#include <gtest/gtest.h>
#include "gmock/gmock.h"

#include <variant>
#include <iostream>

#include "../src/libTFTP.hpp"

//  Data type check
char test[] = { 't', 'e', 's', 't' };
char read_RQ[] = { '\0', '1', 'a', 'k','.', 't', 'x', 't' };

//  ReadFile Data
TEST(ReadData, CharData) {
  ReadFileData<char> data{ 4 };
  memcpy(data.data, &test, sizeof(test));
  for (auto count = 0; count < 4; ++count) {
    EXPECT_EQ(test[count], data.data[count]);
  }
}

//  Base packet dataset test
TEST(Packet, CharData) {
  Packet<char> data{ 4 };
  memcpy(data.packet, &test, sizeof(test));
  for (auto count = 0; count < 4; ++count) {
    EXPECT_EQ(test[count], data.packet[count]);
  }
}

// Client request packet test
TEST(ReadPacket, ReadRq_old_fashioned) {
  ReadPacket data;
  char req_data[]{ '0', '1', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0' };
  const string ex_file_name{ "ak.txt" };
  uint16_t net_code{ htons(1) };
  const string ex_tsize_str{ "tsize" };
  const string ex_blksize_str{ "blksize" };
  const string ex_timeout_str{ "timeout" };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  auto op_code{ (uint16_t)std::get<0>(data.packet_frame_structure) };
  auto error_code{ std::get<1>(data.packet_frame_structure) };
  auto transfer_mode{ std::get<2>(data.packet_frame_structure).value() };
  auto block_number{ std::get<3>(data.packet_frame_structure) };
  auto block_begin{ std::get<4>(data.packet_frame_structure).value() };
  auto block_end{ std::get<5>(data.packet_frame_structure).value() };
  auto file_name{ std::get<6>(data.packet_frame_structure) };


  ASSERT_TRUE(make_struct);
  ASSERT_FALSE(data.req_params);
  ASSERT_FALSE(data.multicast);

  EXPECT_EQ(op_code, (uint16_t)TFTPOpeCode::TFTP_OPCODE_READ);
  EXPECT_FALSE(error_code);
  EXPECT_EQ(transfer_mode, TFTPMode::netascii);
  EXPECT_FALSE(block_number);
  EXPECT_EQ(block_begin, 2);
  EXPECT_EQ(block_end, 8);
  EXPECT_STREQ(file_name.value().c_str(), ex_file_name.c_str());
}


TEST(ReadPacket, WriteRq_old_fashioned) {
  ReadPacket data;
  char req_data[]{ '0', '2', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0' };
  uint16_t net_code{ htons(2) };
  const string ex_file_name{ "ak.txt" };
  const string ex_tsize_str{ "tsize" };
  const string ex_blksize_str{ "blksize" };
  const string ex_timeout_str{ "timeout" };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  auto op_code{ (uint16_t)std::get<0>(data.packet_frame_structure) };
  auto error_code{ std::get<1>(data.packet_frame_structure) };
  auto transfer_mode{ std::get<2>(data.packet_frame_structure).value() };
  auto block_number{ std::get<3>(data.packet_frame_structure) };
  auto block_begin{ std::get<4>(data.packet_frame_structure).value() };
  auto block_end{ std::get<5>(data.packet_frame_structure).value() };
  auto file_name{ std::get<6>(data.packet_frame_structure) };

  ASSERT_TRUE(make_struct);
  ASSERT_FALSE(data.req_params);
  ASSERT_FALSE(data.multicast);

  EXPECT_EQ(op_code, (uint16_t)TFTPOpeCode::TFTP_OPCODE_WRITE);
  EXPECT_FALSE(error_code);
  EXPECT_EQ(transfer_mode, TFTPMode::netascii);
  EXPECT_FALSE(block_number);
  EXPECT_EQ(block_begin, 2);
  EXPECT_EQ(block_end, 8);
  EXPECT_STREQ(file_name.value().c_str(), ex_file_name.c_str());
}

TEST(ReadPacket, ReadRq_Negotiation) {
  ReadPacket data;
  char req_data[]{ '0', '1', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i', 'z', 'e', '\0', '0', '\0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '\0', '5', '1', '2', '\0', 't', 'i', 'm', 'e', 'o', 'u', 't', '\0', '6', '\0' };
  const string ex_file_name{ "ak.txt" };
  uint16_t net_code{ htons(1) };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  auto op_code{ (uint16_t)std::get<0>(data.packet_frame_structure) };
  auto error_code{ std::get<1>(data.packet_frame_structure) };
  auto transfer_mode{ std::get<2>(data.packet_frame_structure).value() };
  auto block_number{ std::get<3>(data.packet_frame_structure) };
  auto block_begin{ std::get<4>(data.packet_frame_structure).value() };
  auto block_end{ std::get<5>(data.packet_frame_structure).value() };
  auto file_name{ std::get<6>(data.packet_frame_structure) };

  auto add_param_vec{ data.req_params.value() };
  auto vec_tsize{ add_param_vec.at(0) };
  auto vec_blksize{ add_param_vec.at(1) };
  auto vec_timeout{ add_param_vec.at(2) };

  ASSERT_TRUE(make_struct);
  ASSERT_TRUE(data.req_params);

  ASSERT_EQ(data.req_params.value().size(), 3);
  EXPECT_EQ(op_code, (uint16_t)TFTPOpeCode::TFTP_OPCODE_READ);
  EXPECT_FALSE(error_code);
  EXPECT_EQ(transfer_mode, TFTPMode::netascii);
  EXPECT_FALSE(block_number);
  EXPECT_EQ(block_begin, 2);
  EXPECT_EQ(block_end, 8);
  EXPECT_STREQ(file_name.value().c_str(), ex_file_name.c_str());

  EXPECT_EQ(vec_tsize.first, OptExtent::tsize);
  EXPECT_EQ(vec_tsize.second, 0);

  EXPECT_EQ(vec_blksize.first, OptExtent::blksize);
  EXPECT_EQ(vec_blksize.second, 512);

  EXPECT_EQ(vec_timeout.first, OptExtent::timeout);
  EXPECT_EQ(vec_timeout.second, 6);
}

TEST(ReadPacket, WriteRq_Negotiation) {
  ReadPacket data;
  char req_data[]{ '0', '2', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i', 'z', 'e', '\0', '5', '\0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '\0', '5', '1', '2', '\0', 't', 'i', 'm', 'e', 'o', 'u', 't', '\0', '6', '\0' };
  uint16_t net_code{ htons(2) };
  const string ex_file_name{ "ak.txt" };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  auto op_code{ (uint16_t)std::get<0>(data.packet_frame_structure) };
  auto error_code{ std::get<1>(data.packet_frame_structure) };
  auto transfer_mode{ std::get<2>(data.packet_frame_structure).value() };
  auto block_number{ std::get<3>(data.packet_frame_structure) };
  auto block_begin{ std::get<4>(data.packet_frame_structure).value() };
  auto block_end{ std::get<5>(data.packet_frame_structure).value() };
  auto file_name{ std::get<6>(data.packet_frame_structure) };

  auto add_param_vec{ data.req_params.value() };
  auto vec_tsize{ add_param_vec.at(0) };
  auto vec_blksize{ add_param_vec.at(1) };
  auto vec_timeout{ add_param_vec.at(2) };

  ASSERT_TRUE(make_struct);
  ASSERT_TRUE(data.req_params);

  ASSERT_EQ(data.req_params.value().size(), 3);
  EXPECT_EQ(op_code, (uint16_t)TFTPOpeCode::TFTP_OPCODE_WRITE);
  EXPECT_FALSE(error_code);
  EXPECT_EQ(transfer_mode, TFTPMode::netascii);
  EXPECT_FALSE(block_number);
  EXPECT_EQ(block_begin, 2);
  EXPECT_EQ(block_end, 8);
  EXPECT_EQ(file_name.value(), ex_file_name.c_str());

  EXPECT_EQ(vec_tsize.first, OptExtent::tsize);
  EXPECT_EQ(vec_tsize.second, 5);

  EXPECT_EQ(vec_blksize.first, OptExtent::blksize);
  EXPECT_EQ(vec_blksize.second, 512);

  EXPECT_EQ(vec_timeout.first, OptExtent::timeout);
  EXPECT_EQ(vec_timeout.second, 6);
}
//  TODO: Check multicast wrong format!!!
TEST(ReadPacket, ReadRq_Negotiation_Multicast_v4_Range_224) {
  ReadPacket data;
  char req_data[]{ '0', '1', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i', 'z', 'e', '\0', '0', '\0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '\0', '5', '1', '2', '\0', 't', 'i', 'm', 'e', 'o', 'u', 't', '\0', '6', '\0', 'm', 'u', 'l', 't', 'i', 'c', 'a', 's', 't', '\0', '2', '2', '4', '.', '0', '.', '0', '.','1', ',', '1', '7', '5', '8', ',' , '1', '\0' };
  const string ex_file_name{ "ak.txt" };
  uint16_t net_code{ htons(1) };
  const string ex_mult_addr{ "224.0.0.1" };
  const uint32_t ex_mult_port{ 1758 };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  ASSERT_TRUE(make_struct);
  ASSERT_TRUE(data.req_params);
  ASSERT_TRUE(data.multicast);

  auto op_code{ (uint16_t)std::get<0>(data.packet_frame_structure) };
  auto error_code{ std::get<1>(data.packet_frame_structure) };
  auto transfer_mode{ std::get<2>(data.packet_frame_structure).value() };
  auto block_number{ std::get<3>(data.packet_frame_structure) };
  auto block_begin{ std::get<4>(data.packet_frame_structure).value() };
  auto block_end{ std::get<5>(data.packet_frame_structure).value() };
  auto file_name{ std::get<6>(data.packet_frame_structure) };

  auto add_param_vec{ data.req_params.value() };
  const auto vec_tsize{ add_param_vec.at(0) };
  const auto vec_blksize{ add_param_vec.at(1) };
  const auto vec_timeout{ add_param_vec.at(2) };

  auto mult_address{ std::get<0>(data.multicast.value()) };
  const auto mult_port{ std::get<1>(data.multicast.value()) };
  const auto mult_master{ std::get<2>(data.multicast.value()) };



  ASSERT_EQ(data.req_params.value().size(), 3);
  EXPECT_EQ(op_code, (uint16_t)TFTPOpeCode::TFTP_OPCODE_READ);
  EXPECT_FALSE(error_code);
  EXPECT_EQ(transfer_mode, TFTPMode::netascii);
  EXPECT_FALSE(block_number);
  EXPECT_EQ(block_begin, 2);
  EXPECT_EQ(block_end, 8);
  EXPECT_STREQ(file_name.value().c_str(), ex_file_name.c_str());

  EXPECT_EQ(vec_tsize.first, OptExtent::tsize);
  EXPECT_EQ(vec_tsize.second, 0);

  EXPECT_EQ(vec_blksize.first, OptExtent::blksize);
  EXPECT_EQ(vec_blksize.second, 512);

  EXPECT_EQ(vec_timeout.first, OptExtent::timeout);
  EXPECT_EQ(vec_timeout.second, 6);

  EXPECT_STREQ(mult_address.c_str(), ex_mult_addr.c_str());
  EXPECT_EQ(mult_port, ex_mult_port);
  EXPECT_TRUE(mult_master);
}

TEST(ReadPacket, ReadRq_Negotiation_Multicast_v4_Range_234) {
  ReadPacket data;
  char req_data[]{ '0', '1', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i', 'z', 'e', '\0', '0', '\0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '\0', '5', '1', '2', '\0', 't', 'i', 'm', 'e', 'o', 'u', 't', '\0', '6', '\0', 'm', 'u', 'l', 't', 'i', 'c', 'a', 's', 't', '\0', '2', '3', '4', '.', '0', '.', '0', '.','1', ',', '1', '7', '5', '8', ',' , '1', '\0' };
  const string ex_file_name{ "ak.txt" };
  uint16_t net_code{ htons(1) };
  const string ex_mult_addr{ "234.0.0.1" };
  const uint32_t ex_mult_port{ 1758 };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  auto op_code{ (uint16_t)std::get<0>(data.packet_frame_structure) };
  auto error_code{ std::get<1>(data.packet_frame_structure) };
  auto transfer_mode{ std::get<2>(data.packet_frame_structure).value() };
  auto block_number{ std::get<3>(data.packet_frame_structure) };
  auto block_begin{ std::get<4>(data.packet_frame_structure).value() };
  auto block_end{ std::get<5>(data.packet_frame_structure).value() };
  auto file_name{ std::get<6>(data.packet_frame_structure) };

  auto add_param_vec{ data.req_params.value() };
  const auto vec_tsize{ add_param_vec.at(0) };
  const auto vec_blksize{ add_param_vec.at(1) };
  const auto vec_timeout{ add_param_vec.at(2) };

  auto mult_address{ std::get<0>(data.multicast.value()) };
  const auto mult_port{ std::get<1>(data.multicast.value()) };
  const auto mult_master{ std::get<2>(data.multicast.value()) };

  ASSERT_TRUE(make_struct);
  ASSERT_TRUE(data.req_params);
  ASSERT_TRUE(data.multicast);

  ASSERT_EQ(data.req_params.value().size(), 3);
  EXPECT_EQ(op_code, (uint16_t)TFTPOpeCode::TFTP_OPCODE_READ);
  EXPECT_FALSE(error_code);
  EXPECT_EQ(transfer_mode, TFTPMode::netascii);
  EXPECT_FALSE(block_number);
  EXPECT_EQ(block_begin, 2);
  EXPECT_EQ(block_end, 8);
  EXPECT_STREQ(file_name.value().c_str(), ex_file_name.c_str());

  EXPECT_EQ(vec_tsize.first, OptExtent::tsize);
  EXPECT_EQ(vec_tsize.second, 0);

  EXPECT_EQ(vec_blksize.first, OptExtent::blksize);
  EXPECT_EQ(vec_blksize.second, 512);

  EXPECT_EQ(vec_timeout.first, OptExtent::timeout);
  EXPECT_EQ(vec_timeout.second, 6);

  EXPECT_STREQ(mult_address.c_str(), ex_mult_addr.c_str());
  EXPECT_EQ(mult_port, ex_mult_port);
  EXPECT_TRUE(mult_master);
}

TEST(ReadPacket, ReadRq_Negotiation_Multicast_v6) {
  ReadPacket data;
  char req_data[]{ '0', '1', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i', 'z', 'e', '\0', '0', '\0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '\0', '5', '1', '2', '\0', 't', 'i', 'm', 'e', 'o', 'u', 't', '\0', '6', '\0', 'm', 'u', 'l', 't', 'i', 'c', 'a', 's', 't', '\0', 'f', 'f','f','f',  ':', '1','1','1','1',':', 'f','f','f', 'f',':', '1','1','1','1',':', 'f','f','f','f', ':',  '1','1','1','1', ':', 'f','f','f','f',':', '1','1','1','1',',', '1', '7', '5', '8', ',' , '1', '\0' };
  const string ex_file_name{ "ak.txt" };
  uint16_t net_code{ htons(1) };
  const string ex_mult_addr{ "ffff:1111:ffff:1111:ffff:1111:ffff:1111" };
  const uint32_t ex_mult_port{ 1758 };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  ASSERT_TRUE(make_struct);
  ASSERT_TRUE(data.req_params);
  ASSERT_TRUE(data.multicast);

  auto op_code{ (uint16_t)std::get<0>(data.packet_frame_structure) };
  auto error_code{ std::get<1>(data.packet_frame_structure) };
  auto transfer_mode{ std::get<2>(data.packet_frame_structure).value() };
  auto block_number{ std::get<3>(data.packet_frame_structure) };
  auto block_begin{ std::get<4>(data.packet_frame_structure).value() };
  auto block_end{ std::get<5>(data.packet_frame_structure).value() };
  auto file_name{ std::get<6>(data.packet_frame_structure) };

  auto add_param_vec{ data.req_params.value() };
  const auto vec_tsize{ add_param_vec.at(0) };
  const auto vec_blksize{ add_param_vec.at(1) };
  const auto vec_timeout{ add_param_vec.at(2) };

  auto mult_address{ std::get<0>(data.multicast.value()) };
  const auto mult_port{ std::get<1>(data.multicast.value()) };
  const auto mult_master{ std::get<2>(data.multicast.value()) };

  ASSERT_EQ(data.req_params.value().size(), 3);
  EXPECT_EQ(op_code, (uint16_t)TFTPOpeCode::TFTP_OPCODE_READ);
  EXPECT_FALSE(error_code);
  EXPECT_EQ(transfer_mode, TFTPMode::netascii);
  EXPECT_FALSE(block_number);
  EXPECT_EQ(block_begin, 2);
  EXPECT_EQ(block_end, 8);
  EXPECT_STREQ(file_name.value().c_str(), ex_file_name.c_str());

  EXPECT_EQ(vec_tsize.first, OptExtent::tsize);
  EXPECT_EQ(vec_tsize.second, 0);

  EXPECT_EQ(vec_blksize.first, OptExtent::blksize);
  EXPECT_EQ(vec_blksize.second, 512);

  EXPECT_EQ(vec_timeout.first, OptExtent::timeout);
  EXPECT_EQ(vec_timeout.second, 6);

  EXPECT_STREQ(mult_address.c_str(), ex_mult_addr.c_str());
  EXPECT_EQ(mult_port, ex_mult_port);
  EXPECT_TRUE(mult_master);
}

TEST(ReadPacket, ReadRq_Negotiation_Multicast_Wrong_Opt_name) {
  ReadPacket data;
  char req_data[]{ '0', '1', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i', 'z', 'e', '\0', '0', '\0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '\0', '5', '1', '2', '\0', 't', 'i', 'm', 'e', 'o', 'u', 't', '\0', '6', '\0', 'm', 'a', 'l', 't', 'i', 'c', 'a', 's', 't', '\0', '2', '2', '4', '.', '0', '.', '0', '.','1', ',', '1', '7', '5', '8', ',' , '1', '\0' };
  const string ex_file_name{ "ak.txt" };
  uint16_t net_code{ htons(1) };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  ASSERT_FALSE(make_struct);
  ASSERT_FALSE(data.req_params);
  ASSERT_FALSE(data.multicast);
}

TEST(ReadPacket, ReadRq_Negotiation_Multicast_Wrong_V4_Range) {
  ReadPacket data;
  char req_data[]{ '0', '1', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i', 'z', 'e', '\0', '0', '\0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '\0', '5', '1', '2', '\0', 't', 'i', 'm', 'e', 'o', 'u', 't', '\0', '6', '\0', 'm', 'u', 'l', 't', 'i', 'c', 'a', 's', 't', '\0', '1', '9', '4', '.', '0', '.', '0', '.','1', ',', '1', '7', '5', '8', ',' , '1', '\0' };
  const string ex_file_name{ "ak.txt" };
  uint16_t net_code{ htons(1) };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  ASSERT_FALSE(make_struct);
  ASSERT_FALSE(data.req_params);
  ASSERT_FALSE(data.multicast);
}

TEST(ReadPacket, ReadRq_Negotiation_Multicast_Wrong_V4_Addr) {
  ReadPacket data;
  char req_data[]{ '0', '1', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i', 'z', 'e', '\0', '0', '\0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '\0', '5', '1', '2', '\0', 't', 'i', 'm', 'e', 'o', 'u', 't', '\0', '6', '\0', 'm', 'u', 'l', 't', 'i', 'c', 'a', 's', 't', '\0', '1', '9', 'a', '.', '0', '.', '0', '.','1', ',', '1', '7', '5', '8', ',' , '1', '\0' };
  const string ex_file_name{ "ak.txt" };
  uint16_t net_code{ htons(1) };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  ASSERT_FALSE(make_struct);
  ASSERT_FALSE(data.req_params);
  ASSERT_FALSE(data.multicast);
}

TEST(ReadPacket, ReadRq_Negotiation_Multicast_Wrong_V6_Addr) {
  ReadPacket data;
  char req_data[]{ '0', '1', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i', 'z', 'e', '\0', '0', '\0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '\0', '5', '1', '2', '\0', 't', 'i', 'm', 'e', 'o', 'u', 't', '\0', '6', '\0', 'm', 'u', 'l', 't', 'i', 'c', 'a', 's', 't', '\0', 'f', 'f','0','0', '1','1','1','1','f','f','f', '1','1','1','1','k','f','f', '1','1','1','1','f','f','f', '1','1','1','1','.','1', ',', '1', '7', '5', '8', ',' , '1', '\0' };
  const string ex_file_name{ "ak.txt" };
  uint16_t net_code{ htons(1) };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  ASSERT_FALSE(make_struct);
  ASSERT_FALSE(data.req_params);
  ASSERT_FALSE(data.multicast);
}

TEST(ReadPacket, ReadRq_old_fashioned_Wrong_opcode) {
  ReadPacket data;
  char req_data[]{ '0', '0', 'a', '\0', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0' };
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  EXPECT_FALSE(make_struct);
}

TEST(ReadPacket, ReadRq_old_fashioned_Wrong_filename) {
  ReadPacket data;
  char req_data[]{ '0', '1', 'a', '\0', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0' };
  uint16_t net_code{ htons(1) };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  EXPECT_FALSE(make_struct);
}

TEST(ReadPacket, ReadRq_old_fashioned_Wrong_transfer_mode) {
  ReadPacket data;
  char req_data[]{ '0', '1', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', '\0', 'a', 's', 'c', 'i', 'i', '\0' };
  uint16_t net_code{ htons(1) };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  EXPECT_FALSE(make_struct);
}

TEST(ReadPacket, ReadRq_Negotiation_WrongTSizeOpt) {
  ReadPacket data;
  char req_data[]{ '0', '1', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i','\0', 'z', 'e', '\0', '0', '\0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '\0', '5', '1', '2', '\0', 't', 'i', 'm', 'e', 'o', 'u', 't', '\0', '6', '\0' };
  uint16_t net_code{ htons(1) };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  EXPECT_FALSE(make_struct);
}

TEST(ReadPacket, WriteRq_Negotiation_WrongTSizeVal) {
  ReadPacket data;
  char req_data[]{ '0', '2', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i', 'z', 'e', '\0', '\0', '\0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '\0', '5', '1', '2', '\0', 't', 'i', 'm', 'e', 'o', 'u', 't', '\0', '6', '\0' };
  const string ex_file_name{ "ak.txt" };
  uint16_t net_code{ htons(2) };
  const string ex_tsize_str{ "tsize" };
  const string ex_blksize_str{ "blksize" };
  const string ex_timeout_str{ "timeout" };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  EXPECT_FALSE(make_struct);
}

TEST(ReadPacket, WriteRq_Negotiation_WrongBlkSizeOpt) {
  ReadPacket data;
  char req_data[]{ '0', '2', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i', 'z', 'e', '\0', '0', '\0', 'b', 'l', 'k', 's', 'i', 'z', '\0', 'e', '\0', '5', '1', '2', '\0', 't', 'i', 'm', 'e', 'o', 'u', 't', '\0', '6', '\0' };
  const string ex_file_name{ "ak.txt" };
  uint16_t net_code{ htons(2) };
  const string ex_tsize_str{ "tsize" };
  const string ex_blksize_str{ "blksize" };
  const string ex_timeout_str{ "timeout" };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  EXPECT_FALSE(make_struct);
}

TEST(ReadPacket, WriteRq_Negotiation_WrongBlkSizeVal) {
  ReadPacket data;
  char req_data[]{ '0', '2', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i', 'z', 'e', '\0', '0', '\0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '\0', '5', '\0', '1', '2', '\0', 't', 'i', 'm', 'e', 'o', 'u', 't', '\0', '6', '\0' };
  const string ex_file_name{ "ak.txt" };
  uint16_t net_code{ htons(2) };
  const string ex_tsize_str{ "tsize" };
  const string ex_blksize_str{ "blksize" };
  const string ex_timeout_str{ "timeout" };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  EXPECT_FALSE(make_struct);
}

TEST(ReadPacket, WriteRq_Negotiation_WrongTimeOutOpt) {
  ReadPacket data;
  char req_data[]{ '0', '2', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i', 'z', 'e', '\0', '0', '\0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '\0', '5', '1', '2', '\0', 't', 'i', 'm', 'e', '\0', 'o', 'u', 't', '\0', '6', '\0' };
  const string ex_file_name{ "ak.txt" };
  uint16_t net_code{ htons(2) };
  const string ex_tsize_str{ "tsize" };
  const string ex_blksize_str{ "blksize" };
  const string ex_timeout_str{ "timeout" };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  EXPECT_FALSE(make_struct);
}

TEST(ReadPacket, WriteRq_Negotiation_WrongTimeOutVal) {
  ReadPacket data;
  char req_data[]{ '0', '2', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i', 'z', 'e', '\0', '0', '\0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '\0', '5',  '1', '2', '\0', 't', 'i', 'm', 'e', 'o', 'u', 't', '\0', '\0', '6', '\0' };
  const string ex_file_name{ "ak.txt" };
  uint16_t net_code{ htons(2) };
  const string ex_tsize_str{ "tsize" };
  const string ex_blksize_str{ "blksize" };
  const string ex_timeout_str{ "timeout" };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  EXPECT_FALSE(make_struct);
}

TEST(ReadPacket, Data_PackNum) {
  ReadPacket data;
  char req_data[]{ '0', '3', '0', '1', 't', 'e', 's', 't' };
  uint16_t net_code{ htons(3) };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };
  auto op_code{ std::get<0>(data.packet_frame_structure) };
  auto err_code{ std::get<1>(data.packet_frame_structure) };
  auto transfer_mode{ std::get<2>(data.packet_frame_structure) };
  auto pack_num{ std::get<3>(data.packet_frame_structure).value() };
  auto first_data_address{ std::get<4>(data.packet_frame_structure).value() };
  auto last_data_address{ std::get<5>(data.packet_frame_structure).value() };

  ASSERT_TRUE(make_struct);
  ASSERT_FALSE(data.req_params);
  ASSERT_FALSE(data.multicast);

  EXPECT_EQ(op_code, TFTPOpeCode::TFTP_OPCODE_DATA);
  EXPECT_FALSE(err_code);
  EXPECT_FALSE(transfer_mode);
  EXPECT_EQ(pack_num, 1);
  EXPECT_EQ(first_data_address, 5);
  EXPECT_EQ(last_data_address, 7);
}

TEST(ReadPacket, Data_PackNum_Wrong_BlockNumber) {
  ReadPacket data;
  char req_data[]{ '0', '3', '\0', '1', 't', 'e', 's', 't' };
  uint16_t net_code{ htons(3) };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  EXPECT_FALSE(make_struct);
}

TEST(ReadPacket, ACK_PackNum) {
  ReadPacket data;
  char req_data[]{ '0', '4', '0', '1' };
  uint16_t net_code{ htons(4) };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };
  auto op_code{ std::get<0>(data.packet_frame_structure) };
  auto err_code{ std::get<1>(data.packet_frame_structure) };
  auto transfer_mode{ std::get<2>(data.packet_frame_structure) };
  auto pack_num{ std::get<3>(data.packet_frame_structure).value() };
  auto first_data_address{ std::get<4>(data.packet_frame_structure) };
  auto last_data_address{ std::get<5>(data.packet_frame_structure) };

  ASSERT_TRUE(make_struct);
  ASSERT_FALSE(data.req_params);
  ASSERT_FALSE(data.multicast);

  EXPECT_EQ(op_code, TFTPOpeCode::TFTP_OPCODE_ACK);
  EXPECT_FALSE(err_code);
  EXPECT_FALSE(transfer_mode);
  EXPECT_EQ(pack_num, 1);
  EXPECT_FALSE(first_data_address);
  EXPECT_FALSE(last_data_address);
}

TEST(ReadPacket, ACK_PackNum_Wrong_BlockNumber) {
  ReadPacket data;
  char req_data[]{ '0', '4', '\0', '1' };
  uint16_t net_code{ htons(4) };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  EXPECT_FALSE(make_struct);
}

TEST(ReadPacket, ERR_PackNum) {
  ReadPacket data;
  char req_data[]{ '0', '5', '0', '1', 't', 'e', 's', 't', 'e', 'r', 'r', 'o', 'r', '\0' };
  uint16_t net_code{ htons(5) };
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };
  auto op_code{ std::get<0>(data.packet_frame_structure) };
  auto err_code{ std::get<1>(data.packet_frame_structure).value() };
  auto transfer_mode{ std::get<2>(data.packet_frame_structure) };
  auto pack_num{ std::get<3>(data.packet_frame_structure) };
  auto first_data_address{ std::get<4>(data.packet_frame_structure).value() };
  auto last_data_address{ std::get<5>(data.packet_frame_structure).value() };

  ASSERT_TRUE(make_struct);
  ASSERT_FALSE(data.req_params);
  ASSERT_FALSE(data.multicast);

  EXPECT_EQ(op_code, TFTPOpeCode::TFTP_OPCODE_ERROR);
  EXPECT_EQ(err_code, TFTPError::File_not_found);
  EXPECT_FALSE(transfer_mode);
  EXPECT_FALSE(pack_num);
  EXPECT_EQ(first_data_address, 5);
  EXPECT_EQ(last_data_address, 12);
}

TEST(ReadPacket, GetResultNegotiationRead) {
  ReadPacket data;
  char req_data[]{ '0', '1', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i', 'z', 'e', '\0', '0', '\0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '\0', '5', '1', '2', '\0', 't', 'i', 'm', 'e', 'o', 'u', 't', '\0', '6', '\0' };
  const string ex_file_name{ "ak.txt" };
  uint16_t net_code{ htons(1) };
  optional<size_t> port{};
  sockaddr_storage addr_stor;
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  ASSERT_TRUE(make_struct);
  ASSERT_TRUE(data.req_params);
  ASSERT_FALSE(data.multicast);
  ASSERT_TRUE(data.getParams(addr_stor, port));

  string file_name{ std::get<0>(data.trans_params).string() };

  EXPECT_STREQ(file_name.c_str(), ex_file_name.c_str());
  ASSERT_FALSE(std::get<3>(data.trans_params));
  ASSERT_TRUE(std::get<4>(data.trans_params));
  ASSERT_TRUE(std::get<5>(data.trans_params));
  ASSERT_TRUE(std::get<6>(data.trans_params));
  EXPECT_FALSE(std::get<7>(data.trans_params));
  EXPECT_FALSE(std::get<8>(data.trans_params));
  
  EXPECT_TRUE(std::get<1>(data.trans_params));
  EXPECT_FALSE(std::get<2>(data.trans_params));
  EXPECT_EQ(std::get<4>(data.trans_params).value(), 512);
  EXPECT_EQ(std::get<5>(data.trans_params).value(), 6);
  EXPECT_EQ(std::get<6>(data.trans_params).value(), 0);
}

TEST(ReadPacket, GetResultMulticast_V4) {
  ReadPacket data;
  char req_data[]{ '0', '1', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i', 'z', 'e', '\0', '0', '\0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '\0', '5', '1', '2', '\0', 't', 'i', 'm', 'e', 'o', 'u', 't', '\0', '6', '\0', 'm', 'u', 'l', 't', 'i', 'c', 'a', 's', 't', '\0', '2', '2', '4', '.', '0', '.', '0', '.','1',',', '1', '7', '5', '8', ',' , '1', '\0' };
  const string ex_file_name{ "ak.txt" };
  uint16_t net_code{ htons(1) };
  optional<size_t> port{};
  sockaddr_storage addr_stor;
  const char mult_addr[] {"224.0.0.1"};
  const uint16_t mult_port{1758};
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  ASSERT_TRUE(make_struct);
  ASSERT_TRUE(data.req_params);
  ASSERT_TRUE(data.multicast);
  ASSERT_TRUE(data.getParams(addr_stor, port));

  string file_name{ std::get<0>(data.trans_params).string() };

  EXPECT_STREQ(file_name.c_str(), ex_file_name.c_str());
  ASSERT_FALSE(std::get<3>(data.trans_params));
  ASSERT_TRUE(std::get<4>(data.trans_params));
  ASSERT_TRUE(std::get<5>(data.trans_params));
  ASSERT_TRUE(std::get<6>(data.trans_params));
  EXPECT_TRUE(std::get<7>(data.trans_params));
  EXPECT_TRUE(std::get<8>(data.trans_params));
  
  EXPECT_TRUE(std::get<1>(data.trans_params));
  EXPECT_FALSE(std::get<2>(data.trans_params));
  EXPECT_EQ(std::get<4>(data.trans_params).value(), 512);
  EXPECT_EQ(std::get<5>(data.trans_params).value(), 6);
  EXPECT_EQ(std::get<6>(data.trans_params).value(), 0);
  EXPECT_STREQ(std::get<7>(data.trans_params).value().c_str(), mult_addr);
  EXPECT_EQ(std::get<8>(data.trans_params).value(), mult_port);
}


TEST(ReadPacket, GetResultMulticast_V6) {
  ReadPacket data;
  char req_data[]{ '0', '1', 'a', 'k', '.', 't', 'x', 't', '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0', 't', 's', 'i', 'z', 'e', '\0', '0', '\0', 'b', 'l', 'k', 's', 'i', 'z', 'e', '\0', '5', '1', '2', '\0', 't', 'i', 'm', 'e', 'o', 'u', 't', '\0', '6', '\0', 'm', 'u', 'l', 't', 'i', 'c', 'a', 's', 't', '\0', 'f', 'f','f','f',  ':', '1','1','1','1',':', 'f','f','f', 'f',':', '1','1','1','1',':', 'f','f','f','f', ':',  '1','1','1','1', ':', 'f','f','f','f',':', '1','1','1','1',',', '1', '7', '5', '8', ',' , '1', '\0' };
  const string ex_file_name{ "ak.txt" };
  uint16_t net_code{ htons(1) };
  optional<size_t> port{};
  sockaddr_storage addr_stor;
  const char mult_addr[] {"ffff:1111:ffff:1111:ffff:1111:ffff:1111"};
  const uint16_t mult_port{1758};
  memcpy(req_data, &net_code, sizeof(net_code));
  memcpy(data.packet, &req_data, sizeof(req_data));
  auto make_struct{ data.makeFrameStruct(sizeof(req_data)) };

  ASSERT_TRUE(make_struct);
  ASSERT_TRUE(data.req_params);
  ASSERT_TRUE(data.multicast);
  ASSERT_TRUE(data.getParams(addr_stor, port));

  string file_name{ std::get<0>(data.trans_params).string() };

  EXPECT_STREQ(file_name.c_str(), ex_file_name.c_str());
  ASSERT_FALSE(std::get<3>(data.trans_params));
  ASSERT_TRUE(std::get<4>(data.trans_params));
  ASSERT_TRUE(std::get<5>(data.trans_params));
  ASSERT_TRUE(std::get<6>(data.trans_params));
  EXPECT_TRUE(std::get<7>(data.trans_params));
  EXPECT_TRUE(std::get<8>(data.trans_params));
  
  EXPECT_TRUE(std::get<1>(data.trans_params));
  EXPECT_FALSE(std::get<2>(data.trans_params));
  EXPECT_EQ(std::get<4>(data.trans_params).value(), 512);
  EXPECT_EQ(std::get<5>(data.trans_params).value(), 6);
  EXPECT_EQ(std::get<6>(data.trans_params).value(), 0);
  EXPECT_STREQ(std::get<7>(data.trans_params).value().c_str(), mult_addr);
  EXPECT_EQ(std::get<8>(data.trans_params).value(), mult_port);
}

// Data transfer packet
TEST(DataPacket, CharData) {
  const uint16_t ex_opcode{ htons(3) };
  const uint16_t ex_pack_number{ htons(1) };
  uint16_t opcode, pack_number;
  const string_view test_msg{ "test12" };
  const string ex_data_str{ test_msg };
  ReadFileData<char> msg{ 6 };
  msg.setData(test_msg);
  SendData<char> data{ 6 };
  data.setData(1, &msg);

  memcpy(&opcode, data.packet, sizeof(uint16_t));
  memcpy(&pack_number, &data.packet[2], sizeof(uint16_t));
  string pack_msg{ &data.packet[4], 6 };

  EXPECT_EQ(opcode, ex_opcode);
  EXPECT_EQ(pack_number, ex_pack_number);
  EXPECT_STREQ(pack_msg.c_str(), ex_data_str.c_str());
}

// RFC 783-1350 ACK packet
TEST(ACKPacket, MinVal) {
  ACKPacket data{ 0 };
  const uint16_t ex_pack_num{ 0 };
  const uint16_t ex_op_code{ 4 };
  uint16_t net_pack_num, net_op_code;
  memcpy(&net_op_code, data.packet, sizeof(uint16_t));
  memcpy(&net_pack_num, &data.packet[2], sizeof(uint16_t));
  auto op_code{ ntohs(net_op_code) };
  auto pack_num{ ntohs(net_pack_num) };

  EXPECT_EQ(op_code, ex_op_code);
  EXPECT_EQ(pack_num, ex_pack_num);
}

TEST(ACKPacket, MaxVal) {
  ACKPacket data{ std::numeric_limits<uint16_t>::max() };
  const uint16_t ex_pack_num{ std::numeric_limits<uint16_t>::max() };
  const uint16_t ex_op_code{ 4 };
  uint16_t net_pack_num, net_op_code;
  memcpy(&net_op_code, data.packet, sizeof(uint16_t));
  memcpy(&net_pack_num, &data.packet[2], sizeof(uint16_t));
  auto op_code{ ntohs(net_op_code) };
  auto pack_num{ ntohs(net_pack_num) };

  EXPECT_EQ(op_code, ex_op_code);
  EXPECT_EQ(pack_num, ex_pack_num);
}

//  Send data packet tests
TEST(SendData, FullTest_Char) {
  const uint16_t ex_op_code{ 3 };
  const uint16_t ex_pack_num{ 0 };
  uint16_t net_pack_num, net_op_code;
  char pack_msg[4];
  string ex_str{ test };
  ReadFileData<char> msg{ 4 };
  memcpy(msg.data, &test, sizeof(test));
  SendData<char> data{ 4 };
  data.setData(0, &msg);
  memcpy(&net_op_code, data.packet, sizeof(uint16_t));
  memcpy(&net_pack_num, &data.packet[2], sizeof(uint16_t));
  auto op_code{ ntohs(net_op_code) };
  auto pack_num{ ntohs(net_pack_num) };
  memcpy(pack_msg, &data.packet[4], 4);
  string msg_str{ pack_msg };

  EXPECT_EQ(op_code, ex_op_code);
  EXPECT_EQ(pack_num, ex_pack_num);
  EXPECT_STREQ(msg_str.c_str(), ex_str.c_str());
}

//  OACK packet
TEST(OACK, TypicalRequest) {
  const uint16_t ex_op_code{ 6 };
  uint16_t net_op_code, net_val;
  const uint16_t ex_size_val{ 10 };
  const uint16_t ex_blk_val{ 512 };
  const uint16_t ex_timeout_val{ 6 };
  const char zero{ '\0' };
  optional<ReqParam> t_size{ make_pair(OptExtent::tsize, (uint16_t)10) }; // Size = 9
  optional<ReqParam> blk_size{ make_pair(OptExtent::blksize, (uint16_t)512) }; // Size = 11
  optional<ReqParam> t_out{ make_pair(OptExtent::timeout, (uint16_t)6) }; // Size = 11
  optional<MulticastOption> mult{};
  char pack_t_size[5];
  char pack_blk_size[7];
  char pack_t_out[7];
  const string  str_size{ "tsize" };
  const string  str_blk{ "blksize" };
  const string  str_timeout{ "timeout" };
  const string ex_str_size_val{ std::to_string(10) };
  const string ex_str_blk_val{ std::to_string(512) };
  const string ex_str_tout_val{ std::to_string(6) };


  string  ex_str_blk;
  string  ex_str_timeout;

  OACKOption param = make_tuple(t_size, blk_size, t_out, mult);
  OACKPacket pack{ &param };

  //  Operation code (should be 6)
  memcpy(&net_op_code, pack.packet, sizeof(uint16_t));
  uint16_t const op_code{ ntohs(net_op_code) };

  //  Source file size (tsize) 
  memcpy(&pack_t_size, &pack.packet[2], 5);
  const string  pack_str_size{ pack_t_size, 5 };
  auto pack_str_zero = pack.packet[7];
  const string str_size_val{ &pack.packet[8], 2 };
  auto pack_val_zero = pack.packet[10];

  //  Transfer block size
  memcpy(&pack_blk_size, &pack.packet[11], sizeof(pack_blk_size));
  const string  pack_str_blk{ pack_blk_size, sizeof(pack_blk_size) };
  auto blk_str_zero = pack.packet[18];
  const string str_blk_val{ &pack.packet[19], 3 };
  auto blk_val_zero = pack.packet[22];

  //  Transfer timeout 
  memcpy(&pack_t_out, &pack.packet[23], sizeof(pack_t_out));
  const string  pack_str_timeout{ pack_t_out, sizeof(pack_t_out) };
  auto t_out_str_zero = pack.packet[30];
  const string str_tout_val{ &pack.packet[31], 1 };
  auto t_out_val_zero = pack.packet[32];

  EXPECT_EQ(op_code, ex_op_code);

  EXPECT_STREQ(pack_str_size.c_str(), str_size.c_str());
  EXPECT_EQ(pack_str_zero, zero);
  EXPECT_STREQ(str_size_val.c_str(), ex_str_size_val.c_str());
  EXPECT_EQ(pack_val_zero, zero);

  EXPECT_STREQ(pack_str_blk.c_str(), str_blk.c_str());
  EXPECT_EQ(blk_str_zero, zero);
  EXPECT_STREQ(str_blk_val.c_str(), ex_str_blk_val.c_str());
  EXPECT_EQ(blk_val_zero, zero);

  EXPECT_STREQ(pack_str_timeout.c_str(), str_timeout.c_str());
  EXPECT_EQ(t_out_str_zero, zero);
  EXPECT_STREQ(str_tout_val.c_str(), ex_str_tout_val.c_str());
  EXPECT_EQ(t_out_val_zero, zero);
}

TEST(OACK, RequestTSize) {
  const uint16_t ex_op_code{ 6 };
  uint16_t net_op_code, net_val;
  const uint16_t ex_size_val{ 10 };

  const char zero{ '\0' };
  optional<ReqParam> t_size{ make_pair(OptExtent::tsize, (uint16_t)10) }; // Size = 9
  optional<ReqParam> blk_size{};
  optional<ReqParam> t_out{};
  optional<MulticastOption> mult{};
  char pack_t_size[5];
  const string  str_size{ "tsize" };
  const string ex_str_size_val{ std::to_string(10) };

  OACKOption param = make_tuple(t_size, blk_size, t_out, mult);
  OACKPacket pack{ &param };

  //  Operation code (should be 6)
  memcpy(&net_op_code, pack.packet, sizeof(uint16_t));
  uint16_t const op_code{ ntohs(net_op_code) };

  //  Source file size (tsize) 
  memcpy(&pack_t_size, &pack.packet[2], 5);
  const string  pack_str_size{ pack_t_size, 5 };
  auto pack_str_zero = pack.packet[7];
  const string str_size_val{ &pack.packet[8], 2 };
  auto pack_val_zero = pack.packet[10];

  EXPECT_EQ(op_code, ex_op_code);

  EXPECT_STREQ(pack_str_size.c_str(), str_size.c_str());
  EXPECT_EQ(pack_str_zero, zero);
  EXPECT_STREQ(str_size_val.c_str(), ex_str_size_val.c_str());
  EXPECT_EQ(pack_val_zero, zero);
}

TEST(OACK, RequestBlkSize) {
  const uint16_t ex_op_code{ 6 };
  uint16_t net_op_code;
  const char zero{ '\0' };
  optional<ReqParam> t_size{};
  optional<ReqParam> blk_size{ make_pair(OptExtent::blksize, (uint16_t)512) }; // Size = 11
  optional<ReqParam> t_out{};
  optional<MulticastOption> mult{};
  char pack_blk_size[7];
  const string  str_blk{ "blksize" };
  const string ex_str_blk_val{ std::to_string(512) };
  string  ex_str_blk;

  OACKOption param = make_tuple(t_size, blk_size, t_out, mult);
  OACKPacket pack{ &param };

  //  Operation code (should be 6)
  memcpy(&net_op_code, pack.packet, sizeof(uint16_t));
  uint16_t const op_code{ ntohs(net_op_code) };

  //  Transfer block size
  memcpy(&pack_blk_size, &pack.packet[2], sizeof(pack_blk_size));
  const string  pack_str_blk{ pack_blk_size, sizeof(pack_blk_size) };
  auto blk_str_zero = pack.packet[9];
  const string str_blk_val{ &pack.packet[10], 3 };
  auto blk_val_zero = pack.packet[13];


  EXPECT_EQ(op_code, ex_op_code);

  EXPECT_STREQ(pack_str_blk.c_str(), str_blk.c_str());
  EXPECT_EQ(blk_str_zero, zero);
  EXPECT_STREQ(str_blk_val.c_str(), ex_str_blk_val.c_str());
  EXPECT_EQ(blk_val_zero, zero);
}

TEST(OACK, RequestTOut) {
  const uint16_t ex_op_code{ 6 };
  uint16_t net_op_code;
  const uint16_t ex_timeout_val{ 6 };
  const char zero{ '\0' };
  optional<ReqParam> t_size{};
  optional<ReqParam> blk_size{};
  optional<ReqParam> t_out{ make_pair(OptExtent::timeout, (uint16_t)6) }; // Size = 11
  optional<MulticastOption> mult{};
  char pack_t_out[7];
  const string  str_timeout{ "timeout" };
  const string ex_str_tout_val{ std::to_string(6) };
  string  ex_str_timeout;

  OACKOption param = make_tuple(t_size, blk_size, t_out, mult);
  OACKPacket pack{ &param };

  //  Operation code (should be 6)
  memcpy(&net_op_code, pack.packet, sizeof(uint16_t));
  uint16_t const op_code{ ntohs(net_op_code) };

  //  Transfer timeout 
  memcpy(&pack_t_out, &pack.packet[2], sizeof(pack_t_out));
  const string  pack_str_timeout{ pack_t_out, sizeof(pack_t_out) };
  auto t_out_str_zero = pack.packet[9];
  const string str_tout_val{ &pack.packet[10], 1 };
  auto t_out_val_zero = pack.packet[11];

  EXPECT_EQ(op_code, ex_op_code);

  EXPECT_STREQ(pack_str_timeout.c_str(), str_timeout.c_str());
  EXPECT_EQ(t_out_str_zero, zero);
  EXPECT_STREQ(str_tout_val.c_str(), ex_str_tout_val.c_str());
  EXPECT_EQ(t_out_val_zero, zero);
}

TEST(OACK, TypicalTSizeBlkSize) {
  const uint16_t ex_op_code{ 6 };
  uint16_t net_op_code;
  const uint16_t ex_size_val{ 10 };
  const uint16_t ex_blk_val{ 512 };
  const char zero{ '\0' };
  optional<ReqParam> t_size{ make_pair(OptExtent::tsize, (uint16_t)10) }; // Size = 9
  optional<ReqParam> blk_size{ make_pair(OptExtent::blksize, (uint16_t)512) }; // Size = 11
  optional<ReqParam> t_out{};
  optional<MulticastOption> mult{};
  char pack_t_size[5];
  char pack_blk_size[7];
  const string  str_size{ "tsize" };
  const string  str_blk{ "blksize" };
  const string ex_str_size_val{ std::to_string(10) };
  const string ex_str_blk_val{ std::to_string(512) };
  string  ex_str_blk;

  OACKOption param = make_tuple(t_size, blk_size, t_out, mult);
  OACKPacket pack{ &param };

  //  Operation code (should be 6)
  memcpy(&net_op_code, pack.packet, sizeof(uint16_t));
  uint16_t const op_code{ ntohs(net_op_code) };

  //  Source file size (tsize) 
  memcpy(&pack_t_size, &pack.packet[2], 5);
  const string  pack_str_size{ pack_t_size, 5 };
  auto pack_str_zero = pack.packet[7];
  const string str_size_val{ &pack.packet[8], 2 };
  auto pack_val_zero = pack.packet[10];

  //  Transfer block size
  memcpy(&pack_blk_size, &pack.packet[11], sizeof(pack_blk_size));
  const string  pack_str_blk{ pack_blk_size, sizeof(pack_blk_size) };
  auto blk_str_zero = pack.packet[18];
  const string str_blk_val{ &pack.packet[19], 3 };
  auto blk_val_zero = pack.packet[22];


  EXPECT_EQ(op_code, ex_op_code);

  EXPECT_STREQ(pack_str_size.c_str(), str_size.c_str());
  EXPECT_EQ(pack_str_zero, zero);
  EXPECT_STREQ(str_size_val.c_str(), ex_str_size_val.c_str());
  EXPECT_EQ(pack_val_zero, zero);

  EXPECT_STREQ(pack_str_blk.c_str(), str_blk.c_str());
  EXPECT_EQ(blk_str_zero, zero);
  EXPECT_STREQ(str_blk_val.c_str(), ex_str_blk_val.c_str());
  EXPECT_EQ(blk_val_zero, zero);
}

TEST(OACK, RequestTSizeTOut) {
  const uint16_t ex_op_code{ 6 };
  uint16_t net_op_code, net_val;
  const uint16_t ex_size_val{ 10 };
  const uint16_t ex_timeout_val{ 6 };
  const char zero{ '\0' };
  optional<ReqParam> t_size{ make_pair(OptExtent::tsize, (uint16_t)10) }; // Size = 9
  optional<ReqParam> blk_size{};
  optional<ReqParam> t_out{ make_pair(OptExtent::timeout, (uint16_t)6) }; // Size = 11
  optional<MulticastOption> mult{};
  char pack_t_size[5];
  char pack_t_out[7];
  const string  str_size{ "tsize" };
  const string  str_timeout{ "timeout" };
  const string ex_str_size_val{ std::to_string(10) };
  const string ex_str_tout_val{ std::to_string(6) };
  string  ex_str_timeout;

  OACKOption param = make_tuple(t_size, blk_size, t_out, mult);
  OACKPacket pack{ &param };

  //  Operation code (should be 6)
  memcpy(&net_op_code, pack.packet, sizeof(uint16_t));
  uint16_t const op_code{ ntohs(net_op_code) };

  //  Source file size (tsize) 
  memcpy(&pack_t_size, &pack.packet[2], 5);
  const string  pack_str_size{ pack_t_size, 5 };
  auto pack_str_zero = pack.packet[7];
  const string str_size_val{ &pack.packet[8], 2 };
  auto pack_val_zero = pack.packet[10];


  //  Transfer timeout 
  memcpy(&pack_t_out, &pack.packet[11], sizeof(pack_t_out));
  const string  pack_str_timeout{ pack_t_out, sizeof(pack_t_out) };
  auto t_out_str_zero = pack.packet[18];
  const string str_tout_val{ &pack.packet[19], 1 };
  auto t_out_val_zero = pack.packet[20];

  EXPECT_EQ(op_code, ex_op_code);

  EXPECT_STREQ(pack_str_size.c_str(), str_size.c_str());
  EXPECT_EQ(pack_str_zero, zero);
  EXPECT_STREQ(str_size_val.c_str(), ex_str_size_val.c_str());
  EXPECT_EQ(pack_val_zero, zero);

  EXPECT_STREQ(pack_str_timeout.c_str(), str_timeout.c_str());
  EXPECT_EQ(t_out_str_zero, zero);
  EXPECT_STREQ(str_tout_val.c_str(), ex_str_tout_val.c_str());
  EXPECT_EQ(t_out_val_zero, zero);
}

TEST(OACK, TypicalBlkSizeTOut) {
  const uint16_t ex_op_code{ 6 };
  uint16_t net_op_code, net_val;
  const uint16_t ex_blk_val{ 512 };
  const uint16_t ex_timeout_val{ 6 };
  const char zero{ '\0' };
  optional<ReqParam> t_size{};
  optional<ReqParam> blk_size{ make_pair(OptExtent::blksize, (uint16_t)512) }; // Size = 11
  optional<ReqParam> t_out{ make_pair(OptExtent::timeout, (uint16_t)6) }; // Size = 11
  optional<MulticastOption> mult{};
  char pack_blk_size[7];
  char pack_t_out[7];
  const string  str_blk{ "blksize" };
  const string  str_timeout{ "timeout" };
  const string ex_str_blk_val{ std::to_string(512) };
  const string ex_str_tout_val{ std::to_string(6) };
  string  ex_str_blk;
  string  ex_str_timeout;

  OACKOption param = make_tuple(t_size, blk_size, t_out, mult);
  OACKPacket pack{ &param };

  //  Operation code (should be 6)
  memcpy(&net_op_code, pack.packet, sizeof(uint16_t));
  uint16_t const op_code{ ntohs(net_op_code) };

  //  Transfer block size
  memcpy(&pack_blk_size, &pack.packet[2], sizeof(pack_blk_size));
  const string  pack_str_blk{ pack_blk_size, sizeof(pack_blk_size) };
  auto blk_str_zero = pack.packet[9];
  const string str_blk_val{ &pack.packet[10], 3 };
  auto blk_val_zero = pack.packet[13];

  //  Transfer timeout 
  memcpy(&pack_t_out, &pack.packet[14], sizeof(pack_t_out));
  const string  pack_str_timeout{ pack_t_out, sizeof(pack_t_out) };
  auto t_out_str_zero = pack.packet[21];
  const string str_tout_val{ &pack.packet[22], 1 };
  auto t_out_val_zero = pack.packet[23];

  EXPECT_EQ(op_code, ex_op_code);

  EXPECT_STREQ(pack_str_blk.c_str(), str_blk.c_str());
  EXPECT_EQ(blk_str_zero, zero);
  EXPECT_STREQ(str_blk_val.c_str(), ex_str_blk_val.c_str());
  EXPECT_EQ(blk_val_zero, zero);

  EXPECT_STREQ(pack_str_timeout.c_str(), str_timeout.c_str());
  EXPECT_EQ(t_out_str_zero, zero);
  EXPECT_STREQ(str_tout_val.c_str(), ex_str_tout_val.c_str());
  EXPECT_EQ(t_out_val_zero, zero);
}

TEST(OACK, MulticastRequest_V4) {
  const uint16_t ex_op_code{ 6 };
  uint16_t net_op_code, net_val;
  const uint16_t ex_size_val{ 10 };
  const uint16_t ex_blk_val{ 512 };
  const uint16_t ex_timeout_val{ 6 };
  const char zero{ '\0' };
  optional<ReqParam> t_size{ make_pair(OptExtent::tsize, (uint16_t)10) }; // Size = 9
  optional<ReqParam> blk_size{ make_pair(OptExtent::blksize, (uint16_t)512) }; // Size = 11
  optional<ReqParam> t_out{ make_pair(OptExtent::timeout, (uint16_t)6) }; // Size = 11
  optional<MulticastOption> mult = make_tuple("224.0.0.1", (uint16_t)1758, true);
  char pack_t_size[5];
  char pack_blk_size[7];
  char pack_t_out[7];
  char mult_addr[9];
  char mult_port[4];
  char delim_1[1];
  char delim_2[1];
  const string str_size{ "tsize" };
  const string str_blk{ "blksize" };
  const string str_timeout{ "timeout" };
  const string str_multicast{ "multicast" };
  const string ex_str_size_val{ std::to_string(10) };
  const string ex_str_blk_val{ std::to_string(512) };
  const string ex_str_tout_val{ std::to_string(6) };
  const string ex_mult_addr{ "224.0.0.1" };
  const string ex_mult_port{ "1758" };
  const char ex_mult_master{ '1' };
  const char mult_delim{ ',' };

  string  ex_str_blk;
  string  ex_str_timeout;
  OACKOption param = make_tuple(t_size, blk_size, t_out, mult);
  OACKPacket pack{ &param };

  //  Operation code (should be 6)
  memcpy(&net_op_code, pack.packet, sizeof(uint16_t));
  uint16_t const op_code{ ntohs(net_op_code) };

  //  Source file size (tsize) 
  memcpy(&pack_t_size, &pack.packet[2], 5);
  const string  pack_str_size{ pack_t_size, 5 };
  auto pack_str_zero = pack.packet[7];
  const string str_size_val{ &pack.packet[8], 2 };
  auto pack_val_zero = pack.packet[10];

  //  Transfer block size
  memcpy(&pack_blk_size, &pack.packet[11], sizeof(pack_blk_size));
  const string  pack_str_blk{ pack_blk_size, sizeof(pack_blk_size) };
  auto blk_str_zero = pack.packet[18];
  const string str_blk_val{ &pack.packet[19], 3 };
  auto blk_val_zero = pack.packet[22];

  //  Transfer timeout 
  memcpy(&pack_t_out, &pack.packet[23], sizeof(pack_t_out));
  const string  pack_str_timeout{ pack_t_out, sizeof(pack_t_out) };
  auto t_out_str_zero = pack.packet[30];
  const string str_tout_val{ &pack.packet[31], 1 };
  auto t_out_val_zero = pack.packet[32];

  //  Multicast address
  const string pack_mult_option{ &pack.packet[33], 9 };
  auto pack_mult_delim_1{ pack.packet[42] };
  const string pack_mult_addr{ &pack.packet[43], 9 };
  auto pack_mult_delim_2{ pack.packet[52] };
  const string pack_mult_port{ &pack.packet[53], 4 };
  auto pack_mult_delim_3{ pack.packet[57] };
  auto pack_mult_master{ pack.packet[58] };
  const char pack_mult_delim_4{ pack.packet[59] };

  EXPECT_EQ(op_code, ex_op_code);

  EXPECT_STREQ(pack_str_size.c_str(), str_size.c_str());
  EXPECT_EQ(pack_str_zero, zero);
  EXPECT_STREQ(str_size_val.c_str(), ex_str_size_val.c_str());
  EXPECT_EQ(pack_val_zero, zero);

  EXPECT_STREQ(pack_str_blk.c_str(), str_blk.c_str());
  EXPECT_EQ(blk_str_zero, zero);
  EXPECT_STREQ(str_blk_val.c_str(), ex_str_blk_val.c_str());
  EXPECT_EQ(blk_val_zero, zero);

  EXPECT_STREQ(pack_str_timeout.c_str(), str_timeout.c_str());
  EXPECT_EQ(t_out_str_zero, zero);
  EXPECT_STREQ(str_tout_val.c_str(), ex_str_tout_val.c_str());
  EXPECT_EQ(t_out_val_zero, zero);

  EXPECT_STREQ(pack_mult_option.c_str(), str_multicast.c_str());
  EXPECT_EQ(pack_mult_delim_1, zero);
  EXPECT_STREQ(pack_mult_addr.c_str(), ex_mult_addr.c_str());
  EXPECT_EQ(pack_mult_delim_2, mult_delim);
  EXPECT_STREQ(pack_mult_port.c_str(), ex_mult_port.c_str());
  EXPECT_EQ(pack_mult_delim_3, mult_delim);
  EXPECT_EQ(pack_mult_master, ex_mult_master);
  EXPECT_EQ(pack_mult_delim_4, zero);
}

TEST(OACK, MulticastRequest_V6) {
  const uint16_t ex_op_code{ 6 };
  uint16_t net_op_code, net_val;
  const uint16_t ex_size_val{ 10 };
  const uint16_t ex_blk_val{ 512 };
  const uint16_t ex_timeout_val{ 6 };
  const char zero{ '\0' };
  optional<ReqParam> t_size{ make_pair(OptExtent::tsize, (uint16_t)10) }; // Size = 9
  optional<ReqParam> blk_size{ make_pair(OptExtent::blksize, (uint16_t)512) }; // Size = 11
  optional<ReqParam> t_out{ make_pair(OptExtent::timeout, (uint16_t)6) }; // Size = 11
  optional<MulticastOption> mult = make_tuple("ffff:1111:ffff:1111:ffff:1111:ffff:1111", (uint16_t)1758, true);
  char pack_t_size[5];
  char pack_blk_size[7];
  char pack_t_out[7];
  char mult_addr[9];
  char mult_port[4];
  char delim_1[1];
  char delim_2[1];
  const string  str_size{ "tsize" };
  const string  str_blk{ "blksize" };
  const string  str_timeout{ "timeout" };
  const string str_multicast{ "multicast" };
  const string ex_str_size_val{ std::to_string(10) };
  const string ex_str_blk_val{ std::to_string(512) };
  const string ex_str_tout_val{ std::to_string(6) };
  const string ex_mult_addr{ "ffff:1111:ffff:1111:ffff:1111:ffff:1111" };
  const string ex_mult_port{ "1758" };
  const char ex_mult_master{ '1' };
  const char mult_delim{ ',' };

  string  ex_str_blk;
  string  ex_str_timeout;

  OACKOption param = make_tuple(t_size, blk_size, t_out, mult);

  OACKPacket pack{ &param };

  //  Operation code (should be 6)
  memcpy(&net_op_code, pack.packet, sizeof(uint16_t));
  uint16_t const op_code{ ntohs(net_op_code) };

  //  Source file size (tsize) 
  memcpy(&pack_t_size, &pack.packet[2], 5);
  const string  pack_str_size{ pack_t_size, 5 };
  auto pack_str_zero = pack.packet[7];
  const string str_size_val{ &pack.packet[8], 2 };
  auto pack_val_zero = pack.packet[10];

  //  Transfer block size
  memcpy(&pack_blk_size, &pack.packet[11], sizeof(pack_blk_size));
  const string  pack_str_blk{ pack_blk_size, sizeof(pack_blk_size) };
  auto blk_str_zero = pack.packet[18];
  const string str_blk_val{ &pack.packet[19], 3 };
  auto blk_val_zero = pack.packet[22];

  //  Transfer timeout 
  memcpy(&pack_t_out, &pack.packet[23], sizeof(pack_t_out));
  const string  pack_str_timeout{ pack_t_out, sizeof(pack_t_out) };
  auto t_out_str_zero = pack.packet[30];
  const string str_tout_val{ &pack.packet[31], 1 };
  auto t_out_val_zero = pack.packet[32];

  //  Multicast address
  const string pack_mult_option{ &pack.packet[33], 9 };
  auto pack_mult_delim_1{ pack.packet[42] };
  const string pack_mult_addr{ &pack.packet[43], 39 };
  auto pack_mult_delim_2{ pack.packet[82] };
  const string pack_mult_port{ &pack.packet[83], 4 };
  auto pack_mult_delim_3{ pack.packet[87] };
  auto pack_mult_master{ pack.packet[88] };
  auto pack_mult_delim_4{ pack.packet[89] };

  EXPECT_EQ(op_code, ex_op_code);

  EXPECT_STREQ(pack_str_size.c_str(), str_size.c_str());
  EXPECT_EQ(pack_str_zero, zero);
  EXPECT_STREQ(str_size_val.c_str(), ex_str_size_val.c_str());
  EXPECT_EQ(pack_val_zero, zero);

  EXPECT_STREQ(pack_str_blk.c_str(), str_blk.c_str());
  EXPECT_EQ(blk_str_zero, zero);
  EXPECT_STREQ(str_blk_val.c_str(), ex_str_blk_val.c_str());
  EXPECT_EQ(blk_val_zero, zero);

  EXPECT_STREQ(pack_str_timeout.c_str(), str_timeout.c_str());
  EXPECT_EQ(t_out_str_zero, zero);
  EXPECT_STREQ(str_tout_val.c_str(), ex_str_tout_val.c_str());
  EXPECT_EQ(t_out_val_zero, zero);

  EXPECT_STREQ(pack_mult_option.c_str(), str_multicast.c_str());
  EXPECT_EQ(pack_mult_delim_1, zero);
  EXPECT_STREQ(pack_mult_addr.c_str(), ex_mult_addr.c_str());
  EXPECT_EQ(pack_mult_delim_2, mult_delim);
  EXPECT_STREQ(pack_mult_port.c_str(), ex_mult_port.c_str());
  EXPECT_EQ(pack_mult_delim_3, mult_delim);
  EXPECT_EQ(pack_mult_master, ex_mult_master);
  EXPECT_EQ(pack_mult_delim_4, zero);
}

GTEST_API_ int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
