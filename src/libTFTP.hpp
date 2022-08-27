#ifndef TFTPLIB_HPP
#define TFTPLIB_HPP

/*
Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Andrey Fokin lazzyfox@gmail.com.
Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


/*! \mainpage Project TFTP server library Index Page
 *
 * \section intro_sec Introduction
 *
 *  Easy, single c++ file TFTP server library
 *
 * \section description TFTP library
 *
 *   RFC 1350 - The TFTP Protocol (Revision 2)
 *   RFC 2347 - TFTP Option Extension
 *   RFC 2348 - TFTP Blocksize Option
 *   RFC 2349 - TFTP Timeout Interval and Transfer Size Options
 *   RFC 2090 - TFTP Multicast Option
 *
 *
 * \section lib_using How to use library
 *
 *
 *
 * \section install Compilation/installation
 *  Could be used a c++ header file or installed as internal library:
 *    cmake .. -DCMAKE_INSTALL_PREFIX:PATH=/your/installation/path
 *    cmake --build . --config Release --target install -- -j $(nproc)
 *
 *
 */

#include <string_view>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <utility>
#include <ranges>
#include <unordered_map>
#include <tuple>
#include <queue>
#include <optional>
#include <functional>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <algorithm>
#include <ios>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stddef.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>


namespace {
  namespace ranges = std::ranges;
  namespace fs = std::filesystem;
  using namespace std::chrono;

  using std::string_view;
  using std::unordered_map;
  using std::tuple;
  using std::make_tuple;
  using std::optional;
  using std::function;
  using std::string;
  using std::pair;
  using std::vector;
  using std::unique_ptr;
  using std::make_unique;
  using std::make_pair;
  using std::shared_ptr;
  using std::make_shared;
  using std::queue;
  using std::jthread;
  using std::thread;
  using std::mutex;
  using std::condition_variable;
  using std::unique_lock;
  using std::lock_guard;
  using std::atomic;
  using std::byte;
  using std::ranges::transform;
  using std::back_inserter;


  constexpr string_view lib_ver{ "0.0.3" };
  constexpr string_view lib_hello{ "TFTP srv library ver - " };

  constexpr uint8_t DEFAULT_PORT{ 69 };
  constexpr uint16_t SERVICE_PORT{ 8099 };

  constexpr uint16_t PACKET_MAX_SIZE{ 1024 };
  constexpr uint16_t PACKET_DATA_SIZE{ 512 };
  constexpr uint16_t PACKET_SIZE{ 516 };
  constexpr uint8_t PACKET_ACK_SIZE{ 4 };
  constexpr uint8_t PACKET_DATA_OVERHEAD{ 5 };
  constexpr int8_t SOCKET_ERR{ -1 };

  //  Packet structure
  constexpr uint8_t DATA_OPCODE_FIELD{ 1 };
  constexpr uint8_t DATA_PACKET_NUMBER_FIELD{ 3 };
  constexpr uint8_t DATA_PACKET_FIELD{ 4 };

  //  Packet minimum size to check consistence
  constexpr uint8_t READ_MIN_SIZE{ 6 };
  constexpr uint8_t WRITE_MIN_SIZE{ 7 };
  constexpr uint8_t DATA_MIN_SIZE{ 5 };
  constexpr uint8_t ACK_MIN_SIZE{ 4 };
  constexpr uint8_t ERROR_MIN_SIZE{ 6 };
  constexpr uint8_t OACK_MIN_SIZE{ 6 };

  constexpr char FILE_OPENEN_ERR[]{ "Can't open file\0" };
  constexpr size_t FILE_OPENEN_ERR_SIZE{ 22 };
  constexpr char FILE_READ_ERR[]{ "Can't read file\0" };
  constexpr size_t FILE_READ_ERR_SIZE{ 22 };
  constexpr char DATA_REORDER_ERR[]{ "Can't read file\0" };
  constexpr size_t DATA_REORDER_ERR_SIZE{ 26 };
  constexpr char FILE_EXISTS_ERR[]{ "File already exists\0" };
  constexpr size_t FILE_EXISTS_ERR_SIZE{ 25 };
  constexpr char MAX_PACK_NUMBER_ERR[]{ "Packet number exceeds\0" };
  constexpr size_t MAX_PACK_NUMBER_ERR_SIZE{ 27 };


  //  RFC 1782 and above option extensions names
  constexpr char TSIZE_OPT_NAME[]{ "tsize" };
  constexpr char TIMEOUT_OPT_NAME[]{ "timeout" };
  constexpr char BLKSIZE_OPT_NAME[]{ "blksize" };
  enum class OptExtent : uint8_t { tsize, timeout, blksize };

  template <typename T> concept TransType = std::same_as <T, byte> || std::same_as <T, char>;

  using FileMode = tuple<fs::path, // Read or write file path
    bool,  //  Read file operation - true
    bool,  //  Binary operation - true
    size_t,  //  Port for Net IO
    uint16_t,  //  Buffer size
    uint8_t,  //  Timeout
    size_t,  //  Fille size for write operations
    struct sockaddr_storage  //  Client address
  >;
  using ThrWorker = pair<std::condition_variable*, FileMode*>;

  enum class TFTPMode : uint8_t { netascii = 1, octet = 2, mail = 3 };
  enum class TFTPOpeCode : uint16_t {
    TFTP_OPCODE_READ = 1,
    TFTP_OPCODE_WRITE = 2,
    TFTP_OPCODE_DATA = 3,
    TFTP_OPCODE_ACK = 4,
    TFTP_OPCODE_ERROR = 5,
    TFTP_OPCODE_OACK = 6
  };

  enum class TFTPError : uint8_t {
    Not_defined,
    File_not_found,
    Access_Violation,
    Disk_full_or_Quota_exceeded,
    Illegal_TFTP_operation,
    Unknown_port_number,
    File_exists,
    No_such_user,
    Optins_are_not_supported
  };

  enum class LogSeverety : uint8_t { Error, Warning, Information, Debug };
  constexpr string_view hello{ "Hello from TFTP server V 0.1" };

  using PacketContent = tuple<TFTPOpeCode, optional<TFTPError>, optional<string_view>, optional<TFTPMode>, optional<uint16_t>>;
  using ReqParam = pair<string, uint16_t>;

  const unordered_map<int, TFTPOpeCode> OptCode{ {1, TFTPOpeCode::TFTP_OPCODE_READ},
                                                 {2, TFTPOpeCode::TFTP_OPCODE_WRITE},
                                                 {3, TFTPOpeCode::TFTP_OPCODE_DATA},
                                                 {4, TFTPOpeCode::TFTP_OPCODE_ACK},
                                                 {5, TFTPOpeCode::TFTP_OPCODE_ERROR} };
  const unordered_map<TFTPOpeCode, char> OpCodeChar{ {TFTPOpeCode::TFTP_OPCODE_READ, '1'},
                                                     {TFTPOpeCode::TFTP_OPCODE_WRITE, '2'},
                                                     {TFTPOpeCode::TFTP_OPCODE_DATA, '3'},
                                                     {TFTPOpeCode::TFTP_OPCODE_ACK, '4'},
                                                     {TFTPOpeCode::TFTP_OPCODE_ERROR, '5'} };
  const unordered_map<string, TFTPMode> ModeCode{ {"netascii", TFTPMode::netascii}, {"octet", TFTPMode::octet}, {"mail", TFTPMode::mail} };
  const unordered_map<uint8_t, TFTPError> ErrorCode{ {0 , TFTPError::Not_defined},
                                                     {1, TFTPError::File_not_found},
                                                     {2, TFTPError::Access_Violation},
                                                     {3, TFTPError::Disk_full_or_Quota_exceeded},
                                                     {4, TFTPError::Illegal_TFTP_operation},
                                                     {5, TFTPError::Unknown_port_number},
                                                     {6, TFTPError::File_exists},
                                                     {7, TFTPError::No_such_user},
                                                     {8, TFTPError::Optins_are_not_supported} };


  //  Read data from file to send as a packet to client
  template <typename T> requires TransType<T>
  struct ReadFileData {
    size_t size;
    T* data{ nullptr };
    ReadFileData(size_t data_size) : size{ data_size } { data = new T[data_size]; }
    ~ReadFileData() {
      if (data) {
        delete[] data;
      }
    }
  };

  //  Data transfer types
  //  Oldfashinal TFTP data packet of 512B data block size
  //  TODO: Should be removed later
  template <typename T> requires TransType<T>
  struct DefaultDataPack {
    struct Data {
      uint16_t op_code;
      uint16_t packet_number;
      T str[PACKET_DATA_SIZE];
    } data;
    DefaultDataPack() { data.op_code = htons((uint8_t)TFTPOpeCode::TFTP_OPCODE_DATA); }
    bool setData(uint16_t number, ReadFileData<T>* data_in) noexcept {
      bool ret{ false };
      if (number > std::numeric_limits<uint16_t>::max()) {
        return ret;
      }
      data.packet_number = htons(number);
      if (data_in->size > PACKET_DATA_SIZE) {
        return ret;
      }
      ret = memcopy(data.str, data_in->data, data_in->size);
      return ret;
    }
  };
  // Base storage for fixed-size packets
  template <size_t packet_size, typename T> requires TransType<T>
  struct BasePacket {
    const size_t size{ packet_size };
    T packet[packet_size];
    void clear(void) {
      if constexpr (std::is_same<T, char>::value) {
        memset(packet, '\0', packet_size);
      }
      else {
        memset(packet, 0, packet_size);
      }
    }
  };
  //  Variable size packets
  template <typename T> requires TransType<T>
  struct Packet {
    size_t packet_size;
    T* packet = nullptr;

    Packet(const size_t size) : packet_size{ size } {
      packet = new T[size];
    }

    void clear(void) {
      if constexpr (std::is_same<T, char>::value) {
        memset(packet, '\0', packet_size);
      }
      else {
        memset(packet, 0, packet_size);
      }
    }
    virtual ~Packet() {
      if (packet) {
        delete[] packet;
      }
    }
  };
  //  Set of methods to analyze or create packets
  // TODO: Check htons/ntohs methods
  template <typename T> requires TransType<T>
  struct PacketTools {
    unsigned char hi, lo;
    tuple<TFTPOpeCode,         //  Operation Code
      optional<TFTPError>, //  Error ID
      optional<TFTPMode>,  //  Transfer mode
      optional<uint16_t>,  //  Block number
      optional<uint16_t>,  //  Data begin address
      optional<uint16_t>   //  Data end address
    > packet_frame_structure;

    std::function<void(int, T*)> getData = [this](int opcode, T* pack) {
      //Block number
//       hi = pack[2];
//       lo = pack[3];
//       auto block_number {(hi<<8)|lo};
      auto block_number{ ntohs((uint16_t)pack[2]) };
      std::get<0>(packet_frame_structure) = OptCode.at(opcode);
      std::get<1>(packet_frame_structure) = optional<TFTPError>{};
      std::get<2>(packet_frame_structure) = optional<TFTPMode>{};
      std::get<3>(packet_frame_structure) = optional<uint16_t>(block_number);
      std::get<4>(packet_frame_structure) = optional<uint16_t>(PACKET_DATA_OVERHEAD);
      std::get<5>(packet_frame_structure) = optional<uint16_t>(sizeof(pack) - PACKET_DATA_OVERHEAD);
    };
    std::function<void(int, T*)> getACK = [this](int opcode, T* pack) {
      //Block number
//       hi = pack[2];
//       lo = pack[3];
//       auto block_number {(hi<<8)|lo};
      auto block_number{ ntohl((uint16_t)pack[2]) };
      std::get<0>(packet_frame_structure) = OptCode.at(opcode);
      std::get<1>(packet_frame_structure) = optional<TFTPError>{};
      std::get<2>(packet_frame_structure) = optional<TFTPMode>{};
      std::get<3>(packet_frame_structure) = optional<uint16_t>(block_number);
      std::get<4>(packet_frame_structure) = optional<uint16_t>{};
      std::get<5>(packet_frame_structure) = optional<uint16_t>{};
    };
    std::function<void(int, T*)> getERROR = [this](int opcode, T* pack) {
      //Error number
//       hi = pack[2];
//       lo = pack[3];
//       auto error_code {(hi<<8)|lo};
      uint8_t error_code{ (uint8_t)pack[2] };
      std::get<0>(packet_frame_structure) = OptCode.at(opcode);
      std::get<1>(packet_frame_structure) = optional<TFTPError>{ ErrorCode.at(error_code) };
      std::get<2>(packet_frame_structure) = optional<TFTPMode>{};
      std::get<3>(packet_frame_structure) = optional<uint16_t>{};
      std::get<4>(packet_frame_structure) = optional<uint16_t>{ 5 };
      std::get<5>(packet_frame_structure) = optional<uint16_t>{ sizeof(pack) - 5 };
    };
    const unordered_map<int, function<void(int, T*)>> req_data{ {3, getData}, {4, getACK}, {5, getERROR} };
    [[nodiscard]] TFTPOpeCode getOpCode(void) const noexcept { return std::get<0>(packet_frame_structure); }
    [[nodiscard]] optional<TFTPError> getErrCode(void) const noexcept { return std::get<1>(packet_frame_structure); }
    [[nodiscard]] optional<uint16_t> getBlockNumber(void) const noexcept { return std::get<3>(packet_frame_structure); }
    [[nodiscard]] T* getDataFrame(T* pack) const noexcept {
      T* ret = nullptr;
      if (!std::get<3>(packet_frame_structure)) {
        return ret;
      }
      const size_t data_size{ sizeof(pack) - PACKET_DATA_OVERHEAD + 1 };
      ret = new T[data_size];
      memmove(ret, &pack[std::get<3>(packet_frame_structure).value()], data_size);
      return ret;
    }
    [[nodiscard]] optional<uint16_t> getDataSize(void) const noexcept { return std::get<5>(packet_frame_structure); }
    [[nodiscard]] optional<uint16_t> getDataAddr(void) const noexcept { return std::get<3>(packet_frame_structure); }

    //  Set data to packet to send to client
    void setDataFrame(T* data_frame, uint16_t packet_number, ReadFileData<T>* data_in) {
      uint16_t op_code{ 3 };
      uint16_t pacl_num{htons(packet_number)};
      memmove(&data_frame[0], &op_code, sizeof(op_code));
      memmove(&data_frame[2], &pacl_num, sizeof(pacl_num));
      memmove(&data_frame[DATA_PACKET_FIELD], data_in->data, sizeof(T) * data_in->size);
    }
  };
  //  Connection (transfer) request from client 
  struct ReadPacket final : BasePacket <PACKET_MAX_SIZE, char> {
    tuple<TFTPOpeCode, //  Operation Code
      optional<TFTPError>, //  Error ID
      optional<TFTPMode>, //  Transfer mode
      optional<uint16_t>, //  Block number
      optional<uint16_t>, //  Data begin address
      optional<uint16_t>, //  Data end address
      optional<string> //  File name
    > packet_frame_structure;
    optional<vector<ReqParam>> req_params;

    //  Sorting data and creating data map
    bool makeFrameStruct(size_t pack_size) {
      bool ret{ false };
      unsigned char hi, lo;

      auto reqRW = [this, pack_size](int opcode) ->bool {
        bool ret{ true };
        uint16_t count_begin{ 2 }, count_end{ 2 }, count_mode;
        string transf_mode, buffer, file_name;
        vector <ReqParam> options{};

        //  Message content end position count
        while (packet[count_end] != '\0') {
          ++count_end;
          if (count_end > pack_size) {
            return false;
          }
        }

        //  Transfer mode check
        count_mode = count_end + 1;
        do {
          if (count_mode > pack_size) {
            return false;
          }
          transf_mode += packet[count_mode];
          ++count_mode;
        } while (packet[count_mode] != '\0');

        std::get<0>(packet_frame_structure) = OptCode.at(opcode);
        std::get<1>(packet_frame_structure) = optional<TFTPError>{};
        std::get<2>(packet_frame_structure) = optional<TFTPMode>(ModeCode.at(transf_mode));
        std::get<3>(packet_frame_structure) = optional<uint16_t>{};
        std::get<4>(packet_frame_structure) = optional<uint16_t>(count_begin);
        std::get<5>(packet_frame_structure) = optional<uint16_t>(count_end);

        //  Get file name
        for (auto fl_name_count = count_begin; fl_name_count < count_end; ++fl_name_count) {
          if (fl_name_count > pack_size) {
            return false;
          }
          file_name += packet[fl_name_count];
        }
        std::get<6>(packet_frame_structure) = optional<string>(file_name);

        //  Check additional options according RFC 1782 and above
        ++count_mode;
        if (count_mode > pack_size) {
          return false;
        }
        while (count_mode < pack_size) {
          transf_mode.clear();

          do {
            if (packet[count_mode] == '\0') {
              continue;
            }
            transf_mode += packet[count_mode];
            ++count_mode;
            if (count_mode > pack_size) {
              return false;
            }
          } while (packet[count_mode] != '\0');

          buffer.clear();

          do {
            if (count_mode > pack_size) {
              return false;
            }
            buffer += packet[++count_mode];
          } while (packet[count_mode] != '\0');

          count_begin = stoi(buffer);
          options.emplace_back(std::make_pair(transf_mode, count_begin));
          ++count_mode;
        }

        if (!options.empty()) {
          req_params.emplace(options);
        }
        return ret;
      };

      auto getData = [this, pack_size](int opcode) ->bool {
        bool ret{ true };
        unsigned char hi, lo;

        if (pack_size < DATA_MIN_SIZE) {
          return false;
        }

        //Block number
        hi = packet[2];
        lo = packet[3];
        auto block_number{ (hi << 8) | lo };
        std::get<0>(packet_frame_structure) = OptCode.at(opcode);
        std::get<1>(packet_frame_structure) = optional<TFTPError>{};
        std::get<2>(packet_frame_structure) = optional<TFTPMode>{};
        std::get<3>(packet_frame_structure) = optional<uint16_t>(block_number);
        std::get<4>(packet_frame_structure) = optional<uint16_t>(5);
        std::get<5>(packet_frame_structure) = optional<uint16_t>(sizeof(packet) - 5);
        return ret;
      };

      auto getACK = [this, pack_size](int opcode) -> bool {
        bool ret{ true };
        unsigned char hi, lo;

        if (pack_size < ACK_MIN_SIZE) {
          return false;
        }

        //Block number
        hi = packet[2];
        lo = packet[3];
        auto block_number{ (hi << 8) | lo };
        std::get<0>(packet_frame_structure) = OptCode.at(opcode);
        std::get<1>(packet_frame_structure) = optional<TFTPError>{};
        std::get<2>(packet_frame_structure) = optional<TFTPMode>{};
        std::get<3>(packet_frame_structure) = optional<uint16_t>(block_number);
        std::get<4>(packet_frame_structure) = optional<uint16_t>{};
        std::get<5>(packet_frame_structure) = optional<uint16_t>{};
        return ret;
      };

      auto getERROR = [this, pack_size](int opcode) ->bool {
        bool ret{ true };
        unsigned char hi, lo;

        if (pack_size < ERROR_MIN_SIZE) {
          return false;
        }

        //Error number
        hi = packet[2];
        lo = packet[3];
        auto error_code{ (hi << 8) | lo };
        std::get<0>(packet_frame_structure) = OptCode.at(opcode);
        std::get<1>(packet_frame_structure) = optional<TFTPError>{ ErrorCode.at(error_code) };
        std::get<2>(packet_frame_structure) = optional<TFTPMode>{};
        std::get<3>(packet_frame_structure) = optional<uint16_t>{};
        std::get<4>(packet_frame_structure) = optional<uint16_t>{ 5 };
        std::get<5>(packet_frame_structure) = optional<uint16_t>{ sizeof(packet) - 5 };
        return ret;
      };


      const unordered_map<int, function<bool(int)>> req_data{ {1, reqRW}, {2, reqRW}, {3, getData}, {4, getACK}, {5, getERROR} };
      hi = packet[0];
      lo = packet[1];
      auto opcode{ (hi << 8) | lo };

      //  Check packet consistence
      switch (opcode) {
      case (int)TFTPOpeCode::TFTP_OPCODE_READ: ret = (pack_size < READ_MIN_SIZE) ? false : true; break;
      case (int)TFTPOpeCode::TFTP_OPCODE_WRITE: ret = (pack_size < WRITE_MIN_SIZE) ? false : true; break;
      case (int)TFTPOpeCode::TFTP_OPCODE_DATA: ret = (pack_size < DATA_MIN_SIZE) ? false : true; break;
      case (int)TFTPOpeCode::TFTP_OPCODE_ACK: ret = (pack_size < ACK_MIN_SIZE) ? false : true; break;
      case (int)TFTPOpeCode::TFTP_OPCODE_ERROR: ret = (pack_size < ERROR_MIN_SIZE) ? false : true; break;
      case (int)TFTPOpeCode::TFTP_OPCODE_OACK: ret = (pack_size < OACK_MIN_SIZE) ? false : true; break;
      default: ret = false;
      }
      if (!ret) {
        return ret;
      }
      auto dataLayOut{ req_data.at(opcode) };
      ret = dataLayOut(opcode);
      return ret;
    }
  };
  // Send data to client (according clients download request)
  template <typename T> requires TransType<T>
  struct SendData final : public Packet<T> {
    size_t pos{ 2 };
    const uint16_t op_code{ htons((uint16_t)TFTPOpeCode::TFTP_OPCODE_DATA) };
    const uint16_t overhead_field_size{ sizeof(op_code) };

    SendData(size_t msg_size) : Packet<T>{msg_size + 2 * sizeof(uint16_t)} {
      memcpy(Packet<T>::packet, &op_code, overhead_field_size);
      // Packet<T>::packet[0] = '0';
      // Packet<T>::packet[1] = '3';
    }
    bool setData(uint16_t pack_count, ReadFileData<T>* msg) {
      bool ret{ false };
      const auto net_pack_code{htons(pack_count)};
      ret = memcpy(Packet<T>::packet + pos, &net_pack_code, overhead_field_size);
      // Packet<T>::packet[2] = '0';
      // Packet<T>::packet[3] = pack_count;
      pos += overhead_field_size;
      ret = memcpy(Packet<T>::packet + pos, msg->data, msg->size); 
      return ret;
    }
    ~SendData() = default;
  };

  template <typename T> requires TransType<T>
  struct DataPacket final : Packet<T>, PacketTools<T> {
    DataPacket(size_t size) : Packet<T>{ size * 2 * sizeof(uint16_t) }, PacketTools<T>{} {}
    void makeFrameStruct(void) noexcept {
      //      hi = packet[0];
      //      lo = packet[1];
          //  auto opcode {(hi<<8)|lo};
      uint16_t net_code;
      memcpy(&net_code, Packet<T>::packet, sizeof(uint16_t));
      const auto opcode{ ntohs(net_code) };
      auto dataLayOut{ PacketTools<T>::req_data.at(opcode) };
      dataLayOut(opcode, Packet<T>::packet);
    }
    void setData(uint16_t packet_number, ReadFileData<T>* data) {
      Packet<T>::clear();
      PacketTools<T>::setDataFrame(Packet<T>::packet, packet_number, data);
    }
    ~DataPacket() = default;

  };
  // RFC 783-1350 (Oldfashinal) request acknowledgement packet 
  struct ACKPacket final : BasePacket <PACKET_ACK_SIZE, char> {

    const uint16_t op_code{ htons(4) };
    ACKPacket() = default;
    ACKPacket(const uint16_t pack_number) { setNumber(pack_number); };
    void setNumber(const uint16_t pack_number) {
      const uint16_t num{ htons(pack_number) };
      BasePacket<PACKET_ACK_SIZE, char>::clear();
      memcpy(&BasePacket<PACKET_ACK_SIZE, char>::packet[0], &op_code, sizeof(op_code));
      memmove(&BasePacket<PACKET_ACK_SIZE, char>::packet[2], &num, sizeof(pack_number));
    }
  };

  struct ErrorPacket final : Packet<char> {
    const unordered_map<TFTPError, uint16_t> CodeConv{ {TFTPError::Not_defined, 0},
                                                       {TFTPError::File_not_found, 1},
                                                       {TFTPError::Access_Violation, 2},
                                                       {TFTPError::Disk_full_or_Quota_exceeded, 3},
                                                       {TFTPError::Illegal_TFTP_operation, 4},
                                                       {TFTPError::Unknown_port_number, 5},
                                                       {TFTPError::File_exists, 6},
                                                       {TFTPError::No_such_user, 7} };
    ErrorPacket(const uint8_t size, const TFTPError code, const char* msg) : Packet{ size } {
      constexpr uint16_t op_code{ 5 };
      const ushort err_code{ CodeConv.at(code) };
      clear();
      memcpy(&packet[1], &op_code, sizeof(op_code));
      memmove(&packet[3], &err_code, sizeof(err_code));
      memmove(&packet[4], msg, strlen(msg));
    }
  };

  template <size_t err_size> struct ConstErrorPacket final : BasePacket <err_size, char> {
    const unordered_map<TFTPError, uint16_t> CodeConv{ {TFTPError::Not_defined, 0},
                                                       {TFTPError::File_not_found, 1},
                                                       {TFTPError::Access_Violation, 2},
                                                       {TFTPError::Disk_full_or_Quota_exceeded, 3},
                                                       {TFTPError::Illegal_TFTP_operation, 4},
                                                       {TFTPError::Unknown_port_number, 5},
                                                       {TFTPError::File_exists, 6},
                                                       {TFTPError::No_such_user, 7} };
    ConstErrorPacket(const TFTPError code, char* msg) {
      constexpr uint16_t op_code{ 5 };
      const ushort err_code{ CodeConv.at(code) };
      BasePacket<err_size, char>::clear();
      memcpy(&BasePacket<err_size, char>::packet[1], &op_code, sizeof(op_code));
      memmove(&BasePacket<err_size, char>::packet[3], &err_code, sizeof(err_code));
      memmove(&BasePacket<err_size, char>::packet[4], msg, strlen(msg));
    }
  };

  struct OACKPacket : Packet <char> {
    //  Set size of total packet length - opcode + param ID + divided zero + param value rtc...
    OACKPacket(size_t size) : Packet{ size} {}
    const uint16_t op_code{ htons((uint16_t)TFTPOpeCode::TFTP_OPCODE_OACK) };
    bool makeData(vector<ReqParam>* val) {
      bool ret{ true };
      uint16_t pos_count{ 2 };
      uint16_t opt_size;
      uint16_t net_val;
      const char zero_code{'0'};
      clear();
      // TODO: Uncomment after debug!!!
      memcpy(&packet[0], &op_code, sizeof(op_code));
      //  TODO: Delete after debuf!!!
      //packet[0]='1';
      //packet[1]='2';

      for (auto& option : *val) {
        opt_size = option.first.size();
        memcpy(&packet[pos_count], option.first.c_str(), opt_size);
        pos_count += opt_size + 1;
        memcpy(&packet[pos_count], &zero_code, sizeof(zero_code));
        ++pos_count;
        net_val = htons(option.second);
        memcpy(&packet[pos_count], &net_val, sizeof(net_val));
        ++pos_count;
        memcpy(&packet[pos_count], &zero_code, sizeof(zero_code));
        ++pos_count;
        // pos_count += 2;
      }
      return ret;
    }
  };




  //  IO buffer
  template <size_t buff_size> class IO_BUFFER {
  public:
    IO_BUFFER() { clear(); }
    IO_BUFFER(const IO_BUFFER&) = delete;
    IO_BUFFER(const IO_BUFFER&&) = delete;
    IO_BUFFER& operator = (IO_BUFFER&) = delete;

    void clear(void) noexcept {
      curr_size = 0;
      memset(buffer, '\0', size);
    }

    [[nodiscard]] bool add(const char* const new_io) noexcept {
      bool ret{ false };
      if (strlen(new_io) + curr_size > size) {
        return ret;
      }
      else {
        size_t io_size{ strlen(new_io) };
        memmove(buffer, new_io, io_size);
        curr_size += io_size;
      }
      return ret;
    }

  private:
    const size_t size{ buff_size };
    size_t curr_size;
    char buffer[buff_size];
  };

  //  Storage for TFTP sessions threads or cash buffers
  template <typename PoolType> class ResPool {
  public:
    explicit ResPool(size_t pull_size) : pool_max_size{ pull_size } {}
    ~ResPool() = default;

    ResPool(ResPool&) = delete;
    ResPool(ResPool&&) = delete;
    ResPool& operator = (ResPool&) = delete;
    ResPool& operator = (ResPool&&) = delete;

    [[nodiscard]] PoolType getRes(void) noexcept {
      lock_guard<std::mutex> pool_lock(pool_access);
      PoolType ret{ thr_pool.front() };
      thr_pool.pop();
      return ret;
    }
    bool setRes(PoolType thr) noexcept {
      bool ret{ false };
      lock_guard<std::mutex> pool_lock(pool_access);
      if (thr_pool.size() >= pool_max_size) {
        return ret;
      }
      thr_pool.emplace(thr);
      return true;
    }
    [[nodiscard]] bool poolAvailable(void) const noexcept {
      bool ret{ false };
      if (thr_pool.size()) {
        ret = true;
      }
      return ret;
    }
  private:
    const size_t pool_max_size; // Number of resources in pool
    mutex pool_access;
    queue<PoolType> thr_pool;
  };

  //  File IO operations
  class FileIO {
  public:
    bool file_is_open{ false };
    bool bin_file{ false };
    //  Log file constructors
    explicit FileIO(const fs::path file_name) : file_name{ file_name } {
      write_file.open(file_name.c_str(), std::ios::in | std::ios::app | std::ios::ate);
    }
    FileIO(const fs::path file_name, const bool reset_file) {
      if (reset_file) {
        write_file.open(file_name.c_str(), std::ios::in | std::ios::trunc);
      }
      else {
        write_file.open(file_name.c_str(), std::ios::in | std::ios::app | std::ios::ate);
      }
    }
    //  Data transfer file operations constructors
    FileIO(const fs::path file_name, const bool read, const bool bin) : file_name{ file_name } {
      if (fs::exists(file_name)) {
        if (read) {
          if (bin) {
            read_file.open(file_name.c_str());
            bin_file = true;
            file_is_open = read_file.is_open();
          }
          else {
            read_file.open(file_name.c_str());
            file_is_open = read_file.is_open();
          }
        }
        else {
          if (bin) {
            write_file.open(file_name.c_str(), std::ios::binary | std::ios::trunc);
            write_file.close();
            write_file.open(file_name.c_str(), std::ios::binary | std::ios::ate);
            bin_file = true;
            file_is_open = write_file.is_open();
          }
          else {
            write_file.open(file_name.c_str(), std::ios::trunc);
            write_file.close();
            write_file.open(file_name.c_str(), std::ios::ate);
            file_is_open = write_file.is_open();
          }
        }
      }
      else {
        if (!read) {
          if (bin) {
            write_file.open(file_name.c_str(), std::ios::binary | std::ios::ate);
            file_is_open = write_file.is_open();
          }
          else {
            write_file.open(file_name.c_str(), std::ios::ate);
            file_is_open = write_file.is_open();
          }
        }
      }
    }
    FileIO(FileMode mode) : FileIO(std::get<0>(mode), std::get<1>(mode), std::get<2>(mode)) {}
    virtual ~FileIO() {
      if (write_file.is_open()) {
        write_file.close();
      }
      if (read_file.is_open()) {
        read_file.close();
      }
    }

    FileIO(const FileIO&) = delete;
    FileIO(const FileIO&&) = delete;
    FileIO& operator = (const FileIO&) = delete;
    FileIO& operator = (const FileIO&&) = delete;

    //  Read from file, as usal is
    template <typename T> requires TransType<T>
    [[nodiscard]] size_t readType(ReadFileData<T>* buffer) noexcept {
      size_t ret{ buffer->size };
      if constexpr (std::is_same<T, byte>::value) {
        read_file.read((char*)buffer->data, buffer->size);
      }
      else {
        read_file.read(buffer->data, buffer->size);
      }
      if (read_file.eof()) {
        ret = read_file.gcount();
        buffer->size = ret;
      }
      if (!read_file) {
        ret = 0;
      }

      return ret;
    }

    bool write(string_view str) noexcept {
      bool ret{ true };
      write_file << str << std::flush;
      if (write_file.bad()) {
        ret = false;
      }
      return ret;
    }
    bool write(char* str, size_t data_size) noexcept {
      bool ret{ true };
      write_file.write(str, data_size);
      if (write_file.bad()) {
        ret = false;
      }
      return ret;
    }
    template <typename T>
      requires TransType<T>
    bool writeType(T* str) noexcept {
      bool ret{ true };
      write_file << str << std::flush;
      if (write_file.bad()) {
        ret = false;
      }
      return ret;
    }

  protected:
    const fs::path file_name;
  private:
    std::ofstream write_file;
    std::ifstream read_file;



  };

  //  Log to file
  class Log final : public FileIO {
  public:
    Log(const fs::path log_fl, const bool debug) : FileIO(log_fl), debug{ debug } {}
    Log(const fs::path log_fl, const bool renew_log, const bool debug)
      : FileIO{ log_fl, renew_log }, debug{ debug } {}
    Log(const fs::path log_fl, const bool renew_log, const bool debug, const bool terminal)
      : Log{ log_fl ,renew_log , debug } {
      this->terminal = terminal;
    }

    Log(const Log&) = delete;
    Log(const Log&&) = delete;
    Log& operator = (const Log&) = delete;
    Log& operator = (const Log&&) = delete;

    bool errMsg(const string_view source, const string_view msg) noexcept {
      bool ret;
      ret = write(source, msg, LogSeverety::Error);
      return ret;
    }
    bool warningMsg(const string_view source, const string_view msg) noexcept {
      bool ret;
      ret = write(source, msg, LogSeverety::Warning);
      return ret;
    }
    bool infoMsg(const string_view source, const string_view msg) noexcept {
      bool ret;
      ret = write(source, msg, LogSeverety::Information);
      return ret;
    }
    bool debugMsg(const string_view source, const string_view msg) noexcept {
      bool ret{ false };
      if (debug) {
        ret = write(source, msg, LogSeverety::Debug);
      }
      return ret;
    }
  protected:
    bool write(const string_view msg, const string_view source, const LogSeverety severety) noexcept {
      bool ret{ false };
      string msg_severety;
      time_t ttime = time(0);
      string log_msg{ ctime(&ttime) };

      switch (severety) {
      case LogSeverety::Error: msg_severety = "Error"; break;
      case LogSeverety::Warning: msg_severety = "Warning"; break;
      case LogSeverety::Information: msg_severety = "Information"; break;
      case LogSeverety::Debug: msg_severety = "Debug"; break;
      default: msg_severety = "Information";
      }

      log_msg.pop_back();
      log_msg += " : ";
      log_msg += msg_severety;
      log_msg += " : ";
      log_msg += source;
      log_msg += " : ";
      log_msg += msg;
      log_msg += ";";
      log_msg += "\n";

      if (terminal) {
        std::cout << log_msg << std::endl << std::flush;
        ret = true;
      }
      else {
        ret = FileIO::write(log_msg);
      }

      return ret;
    }

  private:
    const bool debug;
    bool terminal{ false };
  };

  //  Base class for networking
  class BaseNet {
  public:
    //  Transfer param
    const size_t buff_size{ 512 };
    const size_t timeout{ 3 };
    const size_t file_size{ 0 };
    struct sockaddr_storage cliaddr;  //  Client connection address //  TODO Remove after debug
    socklen_t  cli_addr_size{ sizeof(cliaddr) };  //  TODO Remove after debug
    int sock_id{ 0 };  //  TODO Remove after debug

    BaseNet(size_t port) : port{ port } {
      if (!init(port)) {
        std::cerr << "Socket init problem" << std::endl;
      }
    }
    BaseNet() : BaseNet(DEFAULT_PORT) {}
    BaseNet(const size_t port, const size_t buff_size, const size_t timeout, const size_t file_size, struct sockaddr_storage cln_addr)
      : buff_size{ buff_size }, timeout{ timeout }, file_size{ file_size }, port{ port } {
      try {
        if (!init(port)) {
          std::cerr << "Socket init problem" << std::endl;
        }

        //  Buffer size setup
        if (buff_size != 512) {
          if (setsockopt(sock_id, SOL_SOCKET, SO_RCVBUF, &buff_size, sizeof(buff_size))) {
            throw std::runtime_error("Socket SET RECEIVE BUFFER SIZE error");
          }
          if (setsockopt(sock_id, SOL_SOCKET, SO_SNDBUF, &buff_size, sizeof(buff_size))) {
            throw std::runtime_error("Socket SET SEND BUFFER error");
          }
        }

        //  In/Out traffic timeout  setup
        if (timeout != 3) {
          if (setsockopt(sock_id, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout))) {
            throw std::runtime_error("Socket SET RECEIVE TIMEOUT error");
          }
          if (setsockopt(sock_id, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout))) {
            throw std::runtime_error("Socket SET SEND TIMEOUT error");
          }
        }
      }
      catch (const std::exception& e) {
        std::cerr << "BaseNet - ";
        std::cerr << e.what() << std::endl;
      }
    }
    //      BaseNet (const size_t port, const size_t buff_size, const size_t timeout, const size_t file_size, struct sockaddr_storage cln_addr)
    //        : buff_size{buff_size}, timeout{timeout}, file_size{file_size}, port{port} {
    //          timeval tm_out;
    //          try {
    //            sock_id = socket(AF_UNSPEC, SOCK_DGRAM, 0);
    //            if (!sock_id) {
    //              throw std::runtime_error ("Socket CREATING error");
    //            }
    //            if (setsockopt(sock_id, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
    //              throw std::runtime_error ("Socket SET OPTIONS error");
    //            }
    //            if (buff_size != 512) {
    //              if (setsockopt(sock_id, SOL_SOCKET, SO_RCVBUF, &buff_size, sizeof(buff_size))) {
    //                throw std::runtime_error ("Socket SET RECEIVE BUFFER SIZE error");
    //              }
    //              if (setsockopt(sock_id, SOL_SOCKET, SO_SNDBUF, &buff_size, sizeof(buff_size))) {
    //                throw std::runtime_error ("Socket SET SEND BUFFER error");
    //              }
    //            }
    //            if (timeout != 3) {
    //              if (setsockopt(sock_id, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout))) {
    //                throw std::runtime_error ("Socket SET RECEIVE TIMEOUT error");
    //              }
    //              if (setsockopt(sock_id, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout))) {
    //                throw std::runtime_error ("Socket SET SEND TIMEOUT error");
    //              }
    //            }
    //            address.sin_family = AF_UNSPEC;
    //            address.sin_addr.s_addr = INADDR_ANY;
    //            address.sin_port = htons(port);
    //            if (bind(sock_id, (struct sockaddr *)&address, sizeof(address)) < 0) {
    //                throw std::runtime_error ("Socket BIND error");
    //            }
    //            memmove(&cliaddr, &cln_addr, sizeof(cln_addr));
    //            cli_addr_size = sizeof(cliaddr);
    //            tm_out.tv_sec = timeout;
    //            auto set_sock = setsockopt(sock_id, SOL_SOCKET, SO_SNDTIMEO, &tm_out, sizeof(tm_out));
    //            if (!set_sock) {
    //              throw std::runtime_error ("Socket CREATING error");
    //            }

    //         } catch (const std::exception& e) {
    //           std::cerr<< "BaseNet - ";
    //           std::cerr << e.what()<< std::endl;
    //         }
    //      }
    BaseNet(const size_t buff_size, const size_t timeout, const size_t file_size, struct sockaddr_storage cln_addr)
      : BaseNet(DEFAULT_PORT, buff_size, timeout, file_size, cln_addr) {}
    BaseNet(const size_t port, struct sockaddr_storage cln_addr)
      : BaseNet(port, 0, 0, 0, cln_addr) {}

    virtual ~BaseNet() {
      if (sock_id) {
        close(sock_id);
      }
    }

    //  Send to client OACK packet
    bool sendOACK(uint16_t buff_size = 0, uint8_t timeout = 0, size_t file_size = 0) noexcept {
      bool ret = true;
      uint16_t packet_size;
      vector<ReqParam> val;

      if (buff_size > 0) {
        val.emplace_back(make_pair(BLKSIZE_OPT_NAME, buff_size));
        packet_size = sizeof(buff_size);
        packet_size += strlen(BLKSIZE_OPT_NAME);
        ++packet_size;
      }

      if (timeout > 0) {
        val.emplace_back(make_pair(TIMEOUT_OPT_NAME, timeout));
        packet_size += sizeof(timeout);
        packet_size += strlen(TIMEOUT_OPT_NAME);
        ++packet_size;
      }

      if (file_size > 0) {
        val.emplace_back(make_pair(TSIZE_OPT_NAME, file_size));
        packet_size += sizeof(file_size);
        packet_size += strlen(TSIZE_OPT_NAME);
        ++packet_size;
      }

      packet_size += 2;  //  OpCode size
      OACKPacket data{ packet_size };
      data.makeData(&val);
      auto res = sendto(sock_id, &data.packet, data.packet_size, MSG_CONFIRM, (const struct sockaddr*)&cliaddr, cli_addr_size);
      if (res == SOCKET_ERR) {
        ret = false;
      }
      return ret;
    }
  protected:
    //  Socket params
    const size_t port;
    //int sock_id{0}; //TODO: remove comments after debug
    struct sockaddr_in address, servaddr;
    int opt = 1;
    int addrlen = sizeof(address);
    //      socklen_t  cli_addr_size;  //TODO: remove comments after debug

    string getERRNO(void) {
      string err_str;
      switch (errno) {
      case EBADF: err_str = "The argument sock_id is an invalid descriptor"; break;
      case EFAULT: err_str = "The receive buffer pointer(s) point outside the process's address space"; break;
      case EINTR: err_str = "The receive was interrupted by delivery of a signal before any data were available"; break;
      case EINVAL: err_str = "Invalid argument passed"; break;
      case ENOMEM: err_str = "Could not allocate memory for recvmsg()"; break;
      case ENOTSOCK: err_str = "The argument sock_id does not refer to a socket"; break;
      default: err_str = "Error not in socket errors list";
      }
      return err_str;
    }
  private:
    bool init(const size_t port = DEFAULT_PORT) noexcept {
      bool ret{ false };
      struct addrinfo hints, * servinfo = nullptr;
      memset(&hints, 0, sizeof hints);
      hints.ai_family = AF_UNSPEC;
      hints.ai_socktype = SOCK_DGRAM;
      hints.ai_flags = AI_PASSIVE;

      if (auto rv = getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &servinfo); rv != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(rv);
        return ret;
      }

      // loop through all the results and bind to the first we can
      for (auto addr_p = servinfo; addr_p != NULL; addr_p = addr_p->ai_next) {
        sock_id = socket(addr_p->ai_family, addr_p->ai_socktype, addr_p->ai_protocol);
        if (sock_id == -1) {
          continue;
        }
        if (bind(sock_id, addr_p->ai_addr, addr_p->ai_addrlen) == -1) {
          close(sock_id);
          continue;
        }
        ret = true;
        break;
      }
      freeaddrinfo(servinfo);
      return ret;
    }
  };

  //  Networking for main server
  class SrvNet : public BaseNet {
  public:
    explicit SrvNet(size_t port) : BaseNet{ port } {}
    SrvNet() : BaseNet{} {}
    SrvNet(size_t port, size_t max_file_size) : BaseNet{ port }, max_file_size{ max_file_size } {}
    SrvNet(size_t port, size_t max_file_size, uint8_t max_time_out, uint16_t max_buff_size)
      : BaseNet{ port }, max_file_size{ max_file_size }, max_time_out{ max_time_out }, max_buff_size{ max_buff_size } {}
    virtual ~SrvNet() = default;

    SrvNet(const SrvNet&) = delete;
    SrvNet(const SrvNet&&) = delete;
    SrvNet& operator = (const SrvNet&) = delete;
    SrvNet& operator = (const SrvNet&&) = delete;

    //  Open socket and waiting for clients connections
    bool waitData(ReadPacket* data = nullptr) noexcept {
      bool ret{ true };
      int valread;

      if (!data) {
        return false;
      }

      data->clear();
      valread = recvfrom(sock_id, (char*)data->packet, PACKET_MAX_SIZE, MSG_WAITALL, (struct sockaddr*)&cliaddr, &cli_addr_size);
      if (valread == SOCKET_ERR) {
        return false;
      }
      ret = data->makeFrameStruct(valread);
      return ret;
    }
    //  Send error to client
    bool sendErr(const TFTPError err_id, const string& err_msg) noexcept {
      bool ret{ true };
      uint8_t err_size{ 5 };
      err_size += err_msg.size();
      ErrorPacket error(err_size, err_id, err_msg.c_str());
      sendto(sock_id, &error.packet, error.packet_size, MSG_CONFIRM, (const struct sockaddr*)&cliaddr, cli_addr_size);
      return ret;
    }
    //  Check RFC 1782 and above option extensions parameters
    bool checkParam(vector<ReqParam>* val = nullptr, size_t file_size = 0) noexcept {
      if (!val) {
        return false;
      }
      for (auto& param_set : *val) {
        std::transform(param_set.first.begin(), param_set.first.end(), param_set.first.begin(), ::tolower);

        if (!param_set.first.compare(TSIZE_OPT_NAME)) {
          if (file_size) {
            param_set.second = file_size;
          }
          else {
            if (param_set.second > max_file_size) {
              param_set.second = max_file_size;
            }
          }
        }
        if (!param_set.first.compare(TIMEOUT_OPT_NAME)) {
          if (param_set.second > max_time_out) {
            param_set.second = max_time_out;
          }
        }
        if (!param_set.first.compare(BLKSIZE_OPT_NAME)) {
          if (param_set.second > max_buff_size) {
            param_set.second = max_buff_size;
          }
        }
      }
      return true;
    }
  protected:
    size_t max_file_size{2199023255552}; // 2 TB in bytes
    uint8_t max_time_out{255};
    uint16_t max_buff_size{65464};

  };

  //  Data transfer session manager (socket, file to disk IO)
  class NetSock final : public BaseNet, public FileIO {
  public:
    NetSock(size_t port, const fs::path file_name, const bool read, const bool bin, atomic<bool>* terminate)
      : BaseNet{ port }, FileIO{ file_name, read, bin }, terminate_transfer{ terminate } {}
    NetSock(const size_t port, const size_t buff_size, const size_t timeout, const size_t file_size, struct sockaddr_storage cln_addr, const std::filesystem::path file_name, const bool read, const bool bin, atomic<bool>* terminate)
      : BaseNet{ port, buff_size, timeout, file_size, cln_addr }, FileIO{ file_name, read, bin }, terminate_transfer{ terminate } {}
    NetSock(const FileMode* mode, atomic<bool>* terminate)
      : NetSock{ std::get<3>(*mode), std::get<4>(*mode), std::get<5>(*mode), std::get<6>(*mode), std::get<7>(*mode), std::get<0>(*mode), std::get<1>(*mode), std::get<2>(*mode), terminate } {}
    NetSock(size_t port, const fs::path file_name, const bool read, const bool bin, atomic<bool>* terminate, shared_ptr<Log> log)
      : NetSock{ port, file_name, read, bin, terminate } {
      this->log = log;
    }
    NetSock(const size_t port, const size_t buff_size, const size_t timeout, const size_t file_size, struct sockaddr_storage cln_addr, const std::filesystem::path file_name, const bool read, const bool bin, atomic<bool>* terminate, shared_ptr<Log> log)
      : NetSock(port, buff_size, timeout, file_size, cln_addr, file_name, read, bin, terminate) {
      this->log = log;
    }
    NetSock(const FileMode* mode, atomic<bool>* terminate, shared_ptr<Log> log)
      : NetSock(mode, terminate) {
      this->log = log;
    }

    ~NetSock() = default;

    NetSock(const NetSock&) = delete;
    NetSock(const NetSock&&) = delete;
    NetSock& operator = (const NetSock&) = delete;
    NetSock& operator = (const NetSock&&) = delete;

    template <typename T> requires TransType<T>
    bool readFile(void) {
      bool ret{ true };
      bool run_transfer{ true };
      size_t read_result{ 0 };
      size_t dat_size{ buff_size - PACKET_DATA_OVERHEAD };
      DataPacket<T> data_pack{ buff_size };  // Message size + 4 byte for TFTP data packet overhead
      ReadFileData<T> data{ dat_size }; // packet_count
      int valread{ 1 };
      TFTPOpeCode op_code;
      uint16_t block_number;
      ssize_t send_result;
      uint16_t packet_order_number{ 1 }; //  First response block number
      const uint16_t max_order_num{ std::numeric_limits<uint16_t>::max() };

      //  Check if file exists and accessible
      if (!std::filesystem::exists(file_name)) {
        ConstErrorPacket<FILE_OPENEN_ERR_SIZE> error(TFTPError::File_not_found, (char*)FILE_OPENEN_ERR);
        sendto(sock_id, &error.packet, error.size, MSG_CONFIRM, (const struct sockaddr*)&cliaddr, cli_addr_size);
        if (log) {
          log->debugMsg(__PRETTY_FUNCTION__, "Can't read requested file");
        }
        return false;
      }

      if (log) {
        log->debugMsg(__PRETTY_FUNCTION__, "Start to read a requested file");
      }

      //  Reding and sending file until it's end
      while (!terminate_transfer->load() && run_transfer) {
        read_result = readType<T>(&data);

        if (!read_result) {
          ConstErrorPacket<FILE_READ_ERR_SIZE> error(TFTPError::Access_Violation, (char*)&FILE_READ_ERR);
          sendto(sock_id, &error.packet, error.size, MSG_CONFIRM, (const struct sockaddr*)&cliaddr, cli_addr_size);
          if (log) {
            log->debugMsg(__PRETTY_FUNCTION__, "Read operation failed");
          }
          return false;
        }

        //  Check if file is finished and terminate transfer
        if (read_result != dat_size) {
          dat_size = read_result;
          run_transfer = false;
          if (log) {
            log->debugMsg(__PRETTY_FUNCTION__, "Read finished");
          }

        }

        if (log) {
          string msg;
          for (size_t count = 0; count < buff_size; ++count) {
            msg += (char)data.data[count];
          }
          log->debugMsg(__PRETTY_FUNCTION__, "Read data - " + msg);
        }
        if (run_transfer) {
          data_pack.setData(packet_order_number, &data);
        }

        if (packet_order_number < max_order_num) {
          ++packet_order_number;
        }
        else {
          ConstErrorPacket<MAX_PACK_NUMBER_ERR_SIZE> error(TFTPError::Illegal_TFTP_operation, (char*)&MAX_PACK_NUMBER_ERR);
          sendto(sock_id, &error.packet, error.size, MSG_CONFIRM, (const struct sockaddr*)&cliaddr, cli_addr_size);
          if (log) {
            log->debugMsg(__PRETTY_FUNCTION__, "Read operation failed");
          }
          return false;
        }

        if (log) {
          string msg;
          for (size_t count = 0; count < dat_size; ++count) {
            msg += (char)data_pack.packet[count];
          }
          log->debugMsg(__PRETTY_FUNCTION__, "Ready to send data - " + msg);
        }
        if (run_transfer) {
          send_result = sendto(sock_id, &data_pack.packet, data_pack.packet_size, MSG_CONFIRM, (const struct sockaddr*)&cliaddr, cli_addr_size);
        }
        else {
          DataPacket<T> snd_data{ dat_size + PACKET_DATA_OVERHEAD };
          snd_data.setData(packet_order_number, &data);
          send_result = sendto(sock_id, &snd_data.packet, snd_data.packet_size, MSG_CONFIRM, (const struct sockaddr*)&cliaddr, cli_addr_size);
        }
        if (send_result == -1) {
          std::cout << "Send result error - " + getERRNO() << std::flush;
          if (log) {
            log->debugMsg(__PRETTY_FUNCTION__, "Send result error" + getERRNO());
          }
          throw std::runtime_error(getERRNO());
        }

        //  Check return receipt
        data_pack.clear();
        valread = recvfrom(sock_id, (char*)&data_pack.packet, buff_size, MSG_WAITALL, (struct sockaddr*)&cliaddr, &cli_addr_size);
        if (valread == SOCKET_ERR) {
          throw std::runtime_error(getERRNO());
        }

        //  Replay data analysis
        data_pack.makeFrameStruct();
        op_code = data_pack.getOpCode();
        if (op_code != TFTPOpeCode::TFTP_OPCODE_DATA) {

          //  Check if a data transfer is an error
          if (op_code == TFTPOpeCode::TFTP_OPCODE_ERROR) {
            throw std::runtime_error(getERRNO());
          }
        }

        block_number = data_pack.getBlockNumber().value();
        if (block_number != packet_order_number) {
          ConstErrorPacket<DATA_REORDER_ERR_SIZE> error(TFTPError::Illegal_TFTP_operation, (char*)DATA_REORDER_ERR);
          sendto(sock_id, &error.packet, error.size, MSG_CONFIRM, (const struct sockaddr*)&cliaddr, cli_addr_size);
          return false;
        }
      }

      return ret;
    }

    template <typename T> requires TransType<T>
    bool writeFile(bool delete_if_error = false) {
      bool ret{ true };
      bool run_transfer{ true };
      TFTPOpeCode op_code;
      uint16_t block_number;
      DataPacket<T> data{ buff_size };
      int valread{ 1 };

      //  Check if file already exists
      if (std::filesystem::exists(file_name)) {
        ConstErrorPacket<FILE_EXISTS_ERR_SIZE> error(TFTPError::Illegal_TFTP_operation, (char*)&FILE_EXISTS_ERR);
        sendto(sock_id, &error.packet, error.size, MSG_CONFIRM, (const struct sockaddr*)&cliaddr, cli_addr_size);
        return false;
      }

      //  Receiving data from net
      while (run_transfer && !terminate_transfer->load()) {
        data.clear();
        valread = recvfrom(sock_id, (char*)&data.packet, buff_size, MSG_WAITALL, (struct sockaddr*)&cliaddr, &cli_addr_size);
        if (valread == SOCKET_ERR) {
          throw std::runtime_error(getERRNO());
        }
        if ((size_t)valread < buff_size) {
          run_transfer = false;
        }

        //  Received data analysis
        data.makeFrameStruct();
        op_code = data.getOpCode();
        if (op_code != TFTPOpeCode::TFTP_OPCODE_DATA) {

          //  Check data transfer error
          if (op_code == TFTPOpeCode::TFTP_OPCODE_ERROR) {
            throw std::runtime_error(getERRNO());
          }
        }
        block_number = data.getBlockNumber().value();
        ack.setNumber(block_number);
        sendto(sock_id, &ack, PACKET_ACK_SIZE, MSG_CONFIRM, (const struct sockaddr*)&cliaddr, cli_addr_size);
        //ret = write(&data.packet[data.getDataAddr().value()], data.getDataSize().value());
        ret = writeType<T>(&data.packet[data.getDataAddr().value()]);
      }

      //  Delete recieved data
      if (terminate_transfer->load() and delete_if_error) {
        if (std::filesystem::exists(file_name)) {
          std::filesystem::remove(file_name);
        }
      }
      return ret;
    }
  private:
    atomic<bool>* terminate_transfer;
    ACKPacket ack{};
    shared_ptr<Log> log;
  };
}

namespace TFTPSrvLib {
  //  TFTP server - main process
  class TFTPSrv final : public SrvNet, public ResPool<ThrWorker> {
  public:
    TFTPSrv(const string_view path)
      : SrvNet{}, ResPool<ThrWorker>(std::thread::hardware_concurrency()), base_dir{ path } {}
    TFTPSrv(const string_view path, const size_t core_mult)
      : SrvNet{}, ResPool<ThrWorker>(std::thread::hardware_concurrency()* core_mult), base_dir{ path } {max_threads = std::thread::hardware_concurrency() * core_mult;}
    TFTPSrv(const string_view path, const size_t core_mult, const size_t port_number)
      : SrvNet{ port_number }, ResPool<ThrWorker>(std::thread::hardware_concurrency()* core_mult), base_dir{ path } {max_threads = std::thread::hardware_concurrency() * core_mult;}
    TFTPSrv(const string_view path, std::shared_ptr<Log> log) : TFTPSrv(path) { this->log = log; }
    TFTPSrv(const string_view path, const size_t core_mult, std::shared_ptr<Log> log) : TFTPSrv(path, core_mult) { this->log = log; }
    TFTPSrv(const string_view path, const size_t core_mult, const size_t port_number, std::shared_ptr<Log> log)
      : TFTPSrv(path, core_mult, port_number) {
      this->log = log;
    }

    //  Starting server - running up session manager to wait incoming clients connections
    bool srvStart(void) noexcept {
      bool ret{ false };
      if (log) {
        string msg{ "Starting server" };
        msg += lib_hello;
        msg += lib_ver;
        log->infoMsg(__PRETTY_FUNCTION__, msg);
      }
      auto res = init();
      if (!res) {
        if (log) {
          log->warningMsg(__PRETTY_FUNCTION__, "Probably not all requested workers started");
        }
      }
      connect_mgr = thread(&TFTPSrv::sessionMgr, this);
      if (connect_mgr.joinable()) {
        connect_mgr.detach();
        ret = true;
      }
      if (log) {
        log->infoMsg(__PRETTY_FUNCTION__, "Server started");
      }
      return ret;
    }
    //  Graceful server shutdown
    bool srvStop(const size_t iteration_number = 60, const size_t port_number = DEFAULT_PORT) noexcept {
      bool ret{ true };
      stop_worker = true;
      size_t iteration_count{ 0 };

      int sock_id;
      struct addrinfo hints, * servinfo;

      while (!active_workers.empty()) {
        std::this_thread::sleep_for(milliseconds(5000));
        ++iteration_count;
        if (iteration_count >= iteration_number) {
          break;
        }
      }
      stop_server = true;
      std::this_thread::sleep_for(milliseconds(3000));

      //  If server still running - send a message to to wake socket up and force to check state condition
      if (stop_server.load()) {
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        if (auto rv = getaddrinfo("127.0.0.1", std::to_string(port_number).c_str(), &hints, &servinfo); rv != 0) {
          return ret;
        }

        //  Loop through all the results, make a socket and send terminationg message
        for (auto p = servinfo; p != NULL; p = p->ai_next) {
          if (sock_id = socket(p->ai_family, p->ai_socktype, p->ai_protocol); sock_id == -1) {
            continue;
          }
          sendto(sock_id, "stop", strlen("stop"), 0, p->ai_addr, p->ai_addrlen);
          break;
        }
      }
      return ret;
    }
    bool srvTerminate(void) {bool ret{true}; return ret;}
    bool transferStop(std::thread::id id) {bool ret{true}; return ret;}
    bool transferTerminate(std::thread::id id) {bool ret{true}; return ret;}
    bool srvStatus(void) {bool ret{true}; return ret;}
    bool procStat(std::thread::id id) {bool ret{true}; return ret;}

  private:
    size_t max_threads{ 8 };
    const std::filesystem::path base_dir;
    size_t file_size;
    atomic<bool> stop_worker{ false }, term_worker{ false }, stop_server{ false };
    vector<jthread> active_workers;
    thread connect_mgr;
    mutex stop_worker_mtx;
    std::shared_ptr<Log> log;

    //  Creating transfers workers(threads)
    bool init(void) {
      bool ret{ true };

      for (size_t worker_count{ 0 }; worker_count < max_threads; ++worker_count) {
        active_workers.emplace_back(jthread(&TFTPSrv::worker, this));
        if (!active_workers.back().joinable()) {
          ret = false;
          active_workers.pop_back();
          if (log) {
            log->debugMsg(__PRETTY_FUNCTION__, "One thread coud not be created");
          }
        }
        else {
          if (log) {
            std::ostringstream thr_convert;
            thr_convert << std::this_thread::get_id();
            const string thr_id{ thr_convert.str() };
            log->debugMsg(__PRETTY_FUNCTION__, "Thread ID - " + thr_id + " created");
          }
        }
      }
      return ret;
    }
    //  Transfer worker
    void worker(void) {
      bool ret;
      mutex mtx;
      condition_variable cv;
      unique_lock<std::mutex> lck(mtx);
      FileMode file_mode;
      ThrWorker thr_worcker{ &cv, &file_mode };
      unique_ptr<NetSock> transfer{};
      std::ostringstream thr_convert;
      thr_convert << std::this_thread::get_id();
      const string thr_id{ thr_convert.str() };
      string request_params;

      if (log) {
        log->debugMsg("Thread ID - " + thr_id, " Starting");
      }

      setRes(thr_worcker);
      cv.wait(lck);

      while (!stop_worker.load() || !term_worker.load()) {
        transfer.reset(new NetSock{ &file_mode, &term_worker });
        ret = transfer->sendOACK(std::get<4>(file_mode), std::get<5>(file_mode), std::get<6>(file_mode));
        if (!ret) {
          if (log) {
            log->infoMsg("Thread ID" + thr_id, "Can't send parametrs confirmation (OACK) message");
          }
          return;
        }

        if (log) {
          request_params += "File - " + std::get<0>(file_mode).string() + "; ";
          if (std::get<1>(file_mode)) {
            request_params += "Operation - READ; ";
          }
          else {
            request_params += "Operation - WRITE; ";
          }
          if (std::get<2>(file_mode)) {
            request_params += "Mode - BINARY; ";
          }
          else {
            request_params += "Mode - ASCII; ";
          }
          request_params += "Port - " + std::to_string(std::get<3>(file_mode)) + "; ";
          request_params += "Buffer size - " + std::to_string(std::get<4>(file_mode)) + "; ";
          request_params += "Timeout - " + std::to_string(std::get<5>(file_mode)) + "; ";
          request_params += "File size - " + std::to_string(std::get<6>(file_mode)) + "; ";

          log->debugMsg("Thread ID" + thr_id, " Started request with params - " + request_params);
        }

        if (auto read_mode{ std::get<1>(file_mode) }; read_mode) {
          if (std::get<2>(file_mode)) {
            transfer->readFile<byte>();
          }
          else {
            transfer->readFile<char>();
          }
        }
        else {
          if (std::get<2>(file_mode)) {
            transfer->writeFile<byte>();
          }
          else {
            transfer->writeFile<char>();
          }
        }
        if (log) {
          log->debugMsg("Thread ID" + thr_id, "File transger " + std::get<0>(file_mode).string() + "finished");
        }

        //  Reset optional parametrs
        std::get<4>(file_mode) = 0;
        std::get<5>(file_mode) = 0;
        std::get<6>(file_mode) = 0;
        setRes(thr_worcker);
        cv.wait(lck);
      }
      std::lock_guard<mutex> stop_mtx(stop_worker_mtx);
      ranges::remove_if(active_workers.begin(), active_workers.end(), [](auto& thr) {if (thr.get_id() == std::this_thread::get_id()) return true; return false;});
      if (log) {
        log->debugMsg("Thread ID" + thr_id, "Finished");
      }
    }
    //  Client connections manager
    void sessionMgr(void) {
      bool valread;
      ReadPacket data;
      fs::path requested_file;
      TFTPOpeCode request_code;

      //  Getting transfer parameters from client request
      auto getSockParam = [](vector<ReqParam>* param_vec, OptExtent param_type) {
        size_t param_val{ 0 };
        string param_name;

        switch (param_type) {
        case OptExtent::tsize: param_name = TSIZE_OPT_NAME; break;
        case OptExtent::timeout: param_name = TIMEOUT_OPT_NAME; break;
        case OptExtent::blksize: param_name = BLKSIZE_OPT_NAME; break;
        default:;
        }

        transform(param_name, back_inserter(param_name), ::tolower);
        for (auto& param_set : *param_vec) {
          if (!param_name.compare(param_set.first)) {
            param_val = param_set.second;
            break;
          }
        }

        return param_val;
      };

      if (log) {
        log->infoMsg("Main thread", "Session manager started");
      }

      while (!stop_worker.load() && !term_worker.load()) {
        valread = waitData(&data);
        if (!valread) {
          continue;
        }

        request_code = std::get<0>(data.packet_frame_structure);

        //  Read request processing
        if (request_code == TFTPOpeCode::TFTP_OPCODE_READ) {

          //  Check if file exists and accessible
          requested_file = base_dir / std::get<6>(data.packet_frame_structure).value();
          std::ifstream r_file{ requested_file };
          if (!r_file) {
            ConstErrorPacket<FILE_READ_ERR_SIZE> error(TFTPError::Access_Violation, (char*)&FILE_READ_ERR);
            sendto(sock_id, &error.packet, error.size, MSG_CONFIRM, (const struct sockaddr*)&cliaddr, cli_addr_size);
            continue;
          }
          file_size = fs::file_size(requested_file);
          r_file.close();
          if (data.req_params) {
            checkParam(&data.req_params.value(), file_size);
          }

          //  Create parameters set to start transfer session
          auto work{ getRes() };
          std::get<0>(*work.second) = base_dir / std::get<6>(data.packet_frame_structure).value();
          std::get<1>(*work.second) = true;
          if (TFTPMode mode{ std::get<2>(data.packet_frame_structure).value() }; mode == TFTPMode::netascii) {
            std::get<2>(*work.second) = false;
          }
          else {
            std::get<2>(*work.second) = true;
          }
          std::get<3>(*work.second) = 0;

          //  Check if additional parameters (RFC 1782) are present
          if (data.req_params.has_value()) {
            auto add_param_set{ data.req_params.value() };
            if (auto param{ getSockParam(&add_param_set, OptExtent::blksize) }; param) {
              std::get<4>(*work.second) = param;
            }
            else {
              std::get<4>(*work.second) = buff_size;
            }
            if (auto param{ getSockParam(&add_param_set, OptExtent::timeout) }; param) {
              std::get<5>(*work.second) = param;
            }
            else {
              std::get<5>(*work.second) = timeout;
            }
            std::get<6>(*work.second) = 0;
          }
          else {
            std::get<4>(*work.second) = buff_size;
            std::get<5>(*work.second) = timeout;
            std::get<6>(*work.second) = 0;
          }
          std::get<7>(*work.second) = cliaddr;
          work.first->notify_one();
        }

        //  Write request processing
        if (request_code == TFTPOpeCode::TFTP_OPCODE_WRITE) {

          //  Check if file already exists
          requested_file = base_dir / std::get<6>(data.packet_frame_structure).value();
          if (fs::exists(requested_file)) {
            ConstErrorPacket<FILE_READ_ERR_SIZE> error(TFTPError::Access_Violation, (char*)&FILE_READ_ERR);
            sendto(sock_id, &error.packet, error.size, MSG_CONFIRM, (const struct sockaddr*)&cliaddr, cli_addr_size);
            continue;
          }
          if (data.req_params) {
            checkParam(&data.req_params.value(), file_size);
          }

          //  Create parameters set to start transfer session
          auto work{ getRes() };
          std::get<0>(*work.second) = base_dir / std::get<6>(data.packet_frame_structure).value();
          std::get<1>(*work.second) = true;
          if (TFTPMode mode{ std::get<2>(data.packet_frame_structure).value() }; mode == TFTPMode::netascii) {
            std::get<2>(*work.second) = false;
          }
          else {
            std::get<2>(*work.second) = true;
          }
          std::get<3>(*work.second) = 0;

          //  Check if additional parameters (RFC 1782) are present
          if (data.req_params.has_value()) {
            if (auto param{ getSockParam(&data.req_params.value(), OptExtent::blksize) }; param) {
              std::get<4>(*work.second) = param;
            }
            else {
              std::get<4>(*work.second) = buff_size;
            }
            if (auto param{ getSockParam(&data.req_params.value(), OptExtent::timeout) }; param) {
              std::get<5>(*work.second) = param;
            }
            else {
              std::get<5>(*work.second) = timeout;
            }
            std::get<6>(*work.second) = 0;
          }
          else {
            std::get<4>(*work.second) = buff_size;
            std::get<5>(*work.second) = timeout;
            std::get<6>(*work.second) = 0;
          }
          std::get<7>(*work.second) = cliaddr;
          work.first->notify_one();
        }
      }
      stop_server = false;
      if (log) {
        log->infoMsg("Main thread", "Session manager finished");
      }
    }
  };
}


#endif // LIBTFTP_HPP
