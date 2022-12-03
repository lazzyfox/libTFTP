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
#include <deque>
#include <optional>
#include <variant>
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
#include <regex>


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

namespace TwinMapType {
  template <typename T1, typename T2>
  class TwinMap {
    public :
      TwinMap () : t1_key{std::make_unique<std::unordered_map<T1, T2>>()}, t2_key{std::make_unique<std::unordered_map<T2, T1>>()}{}
      void set(T1 t1, T2 t2) {
        auto ret1 = t1_key->try_emplace(t1, t2);
        auto ret2 = t2_key->try_emplace(t2, t1);
      }
      T2 get(T1 t1) {
        return t1_key->at(t1);
      }
      T1 get(T2 t2) {
        return t2_key->at(t2);
      }
    private :
      std::unique_ptr<std::unordered_map<T1, T2>> t1_key{};
      std::unique_ptr<std::unordered_map<T2, T1>> t2_key{};
  };
}
namespace {
  namespace ranges = std::ranges;
  namespace fs = std::filesystem;
  using namespace std::chrono;

  using std::string_view;
  using std::stoi;
  using std::unordered_map;
  using std::tuple;
  using std::make_tuple;
  using std::optional;
  using std::variant;
  using std::function;
  using std::string;
  using std::pair;
  using std::vector;
  using std::unique_ptr;
  using std::make_unique;
  using std::make_pair;
  using std::shared_ptr;
  using std::make_shared;
  using std::deque;
  using std::map;
  using std::jthread;
  using std::thread;
  using std::mutex;
  using std::condition_variable;
  using std::unique_lock;
  using std::lock_guard;
  using std::atomic;
  using std::atomic_flag;
  using std::byte;
  using std::ranges::transform;
  using std::back_inserter;
  template<std::size_t... Ints> using index_sequence = std::integer_sequence<std::size_t, Ints...>;


  constexpr string_view lib_ver{ "0.0.3" };
  constexpr string_view lib_hello{ "TFTP srv library ver - " };

  constexpr uint8_t DEFAULT_PORT{ 69 };
  constexpr uint16_t MULTICAST_DEFAULT_PORT{ 1758 };
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
  constexpr char DATA_REORDER_ERR[]{ "Wrong packets number received\0" };
  constexpr size_t DATA_REORDER_ERR_SIZE{ 30 };
  constexpr char FILE_EXISTS_ERR[]{ "File already exists\0" };
  constexpr size_t FILE_EXISTS_ERR_SIZE{ 25 };
  constexpr char MAX_PACK_NUMBER_ERR[]{ "Packet number exceeds\0" };
  constexpr size_t MAX_PACK_NUMBER_ERR_SIZE{ 27 };
  constexpr char BAD_FRAME_FORMAT_ERR[]{ "Bad request format\0" };
  constexpr size_t BAD_FRAME_FORMAT_ERR_SIZE{ 19 };
  constexpr char WRONG_REQUEST_ERR[]{ "Wrong request type - waiting for read or write\0" };
  constexpr size_t WRONG_REQUEST_ERR_SIZE{ 47 };
  constexpr char BUSY_ERR[]{ "Server busy\0" };
  constexpr size_t BUSY_ERR_SIZE{ 12 };


  //  RFC 1782 and above option extensions names
  constexpr char TSIZE_OPT_NAME[]{ "tsize" };
  constexpr char TIMEOUT_OPT_NAME[]{ "timeout" };
  constexpr char BLKSIZE_OPT_NAME[]{ "blksize" };
  constexpr char MULTICAST_OPT_NAME[]{ "multicast" };

  enum class OptExtent : uint8_t { tsize, timeout, blksize, multicast };

  template <typename T> concept TransType = std::same_as <T, byte> || std::same_as <T, char>;
  template <typename T, template<typename> typename Cont> concept ContConceptType = std::same_as <Cont<T>, vector<T>> || std::same_as <Cont<T>, deque<T>>;

  using FileMode = tuple<fs::path, // Read or write file path
    bool,  //  Read file operation - true for read
    bool,  //  Binary operation - true for bin
    optional<size_t>,  //  Port for Net IO
    optional<uint16_t>,  //  Buffer size
    optional<uint8_t>,  //  Timeout
    optional<size_t>,  //  File size for write operations
    optional<string>,  //  Multicast IP addr
    optional<uint16_t>,  //Multicast_port_number
    struct sockaddr_storage  //  Client address
    >;
  //  Parameters set for worker (transfer session)
  using ThrWorker = tuple<std::condition_variable*,  //  Worker start condition
    FileMode*,  //  Transfer settings
    atomic<bool>*, //  Terminate worker condition
    std::thread::id, //  Worker thread ID
    atomic<bool>*  //  Update transfer statistics
    >;
  //  Server status
  using SrvStat = tuple<unique_ptr<vector<std::thread::id>>, //  All workers list
    unique_ptr<vector<std::thread::id>>,  //  Active workers list 
    unique_ptr<vector<std::thread::id>>,  //  Idle workers list 
    unique_ptr<vector<fs::path>>,  //  Files in transfer state
    time_point<system_clock> // Timestamp
  >;
  //  Active worker statistic
  using TransferState = tuple<std::thread::id, //  Thread ID
    fs::path,  //  Transfer data
    time_point<system_clock>,  //  Transfer start time
    size_t,  //  Total size
    size_t*,  // Progress (received or transmitted)
    time_point<system_clock>*,  //  Request time
    atomic<bool>* //  Statistics data update request
  >;
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
    Options_are_not_supported
  };

  enum class LogSeverety : uint8_t { Error, Warning, Information, Debug };
  constexpr string_view hello{ "Hello from TFTP server V 0.1" };

  using PacketContent = tuple<TFTPOpeCode, optional<TFTPError>, optional<string_view>, optional<TFTPMode>, optional<uint16_t>>;
  using ReqParam = pair<OptExtent, uint16_t>;
  //  RFC2090 - multicast option parameters
  using MulticastOption = tuple<string, uint16_t, bool>;
  using OACKOption = tuple<optional<ReqParam>,  //  tsize
                           optional<ReqParam>,  //  blksize
                           optional<ReqParam>,  //  timeout
                           optional<MulticastOption>  //  Multicast parameters set
                           >;

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
                                                     {8, TFTPError::Options_are_not_supported} };
  const unordered_map<string, OptExtent> OptExtGet{ {"tsize", OptExtent::tsize},
                                                    {"timeout", OptExtent::timeout},
                                                    {"blksize", OptExtent::blksize},
                                                    {"multicast", OptExtent::multicast} };
  const unordered_map<OptExtent, string> OptExtVal{ {OptExtent::tsize, "tsize"},
                                                    {OptExtent::timeout, "timeout"},
                                                    {OptExtent::blksize, "blksize"},
                                                    {OptExtent::multicast, "multicast"} };
  

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
    bool setData(string_view str) {
      bool ret{false};
      if (str.size() > size) {
        return ret;
      }
      uint8_t dat_counter {0};
      for (auto str_counter : str) {
        data[dat_counter] = str_counter;
        ++dat_counter;
      }
      if (dat_counter == size) {
        ret = true;
      }
      return ret;
    }
  };
  // Base storage for fixed-size packets
  template <size_t packet_size, typename T> requires TransType<T>
  struct BasePacket {
    const size_t size{ packet_size };
    T packet[packet_size];
    void clear() {
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
    Packet() = default;
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
    optional<MulticastOption> multicast;
    FileMode trans_params;
    //  Clear data in struct 
    void reset(void) {
      clear();

      std::get<1>(packet_frame_structure).reset();
      std::get<2>(packet_frame_structure).reset();
      std::get<3>(packet_frame_structure).reset();
      std::get<4>(packet_frame_structure).reset();
      std::get<5>(packet_frame_structure).reset();
      std::get<6>(packet_frame_structure).reset();

      if (req_params) {
        req_params.reset();
      }
      if(multicast) {
        multicast.reset();
      }

      std::get<3>(trans_params).reset();
      std::get<4>(trans_params).reset();
      std::get<5>(trans_params).reset();
      std::get<6>(trans_params).reset();
      std::get<7>(trans_params).reset();
      std::get<8>(trans_params).reset();
    }
    //  Sorting data and creating data map
    //  TODO: Make exception handling!!!
    //  TODO: Add IP address range check!!!
    [[nodiscard]] bool makeFrameStruct(size_t pack_size = PACKET_MAX_SIZE) noexcept {
      bool ret{ false };
      uint16_t opcode;

      //  Network operation code format to host form
      auto netToHost = [](char* str_code) {
        uint16_t net_code, host_code;
        memcpy(&net_code, str_code, 2);
        host_code = ntohs(net_code);
        return host_code;
      };
      //  Check if multicast ip adders has valid format and belongs to correct multicast address range
      auto checkIPRange = [](string addr) {
        bool ret{false};
        size_t pos;
        string delim;
        string tmp;
        vector<string> ip_octets;
        vector<uint8_t> ip_numbers;
        //  Check IP version
        if (auto pos{addr.find(".")}; pos != string::npos) {
          delim = ".";
        } else {
          delim = ":";
        }
        //  Divide address to octets
        do {
          pos = addr.find(delim);
          if (string curr_pos{addr.at(0)}; pos == 1 && !curr_pos.compare(delim)) {
            continue;
          }
          ip_octets.emplace_back(addr.substr(0, pos));
          addr.erase(0, pos + 1);
        } while (pos != string::npos);
        if (ip_octets.empty()) {
          return ret;
        }
        //  Check if address has valid numbers inside
        //  V4
        if (delim == ".") {
          if (ip_octets.size() != 4) {
            return ret;
          }
          for (auto oct_count : ip_octets) {
            tmp = oct_count.at(0);
            if (auto digit {std::stoi(tmp, &pos, 10)}; digit < 0 || digit > 2) {
              return ret;
            }
            if (oct_count.size() > 1) {
              tmp = oct_count.at(1);
              if (auto digit {std::stoi(tmp, &pos, 10)}; digit < 0 || digit > 9) {
                return ret;
              }
            }
            if (oct_count.size() > 2) {
              tmp = oct_count.at(2);
              if (auto digit {std::stoi(tmp, &pos, 10)}; digit < 0 || digit > 9) {
                return ret;
              }
            }
            if (oct_count.size() > 3) {
              tmp = oct_count.at(3);
              if (auto digit {std::stoi(tmp, &pos, 10)}; digit < 0 || digit > 9) {
                return ret;
              }
            }
            if (auto digit {std::stoi(oct_count, &pos, 10)}; digit < 0 || digit > 255) {
              return ret;
            } else {
              ip_numbers.push_back(digit);
            }
          }
          //  Check IPV4 multicast address range
          pos = ip_numbers.at(0);
          if (pos >= 224 && pos <= 225 ) {
            ret = true;
          }
          if (pos >= 232 && pos <= 239 ) {
            ret = true;
          }
        }
        //  V6 check
        if (delim == ":") {
          bool check_terminated {false};
          const std::regex hex_check ("[0-9a-fA-F]*");
          if (ip_octets.size() < 1 || ip_octets.size() > 8) {
            return ret;
          }
          auto check_consistency = [&check_terminated, &hex_check] (string oct_count) {
            if (!std::regex_match(oct_count, hex_check)) {
              check_terminated = true;
            }
          };
          ranges::for_each(ip_octets, check_consistency);
          if (!check_terminated) {
            ret = true;
          }
        }
        return ret;
      };

      auto reqRW = [this, pack_size, checkIPRange](int opcode) ->bool {
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

          if (buffer[0] == '\0') {
            return false;
          }
          auto option_name{OptExtGet.at(string(transf_mode))};
          //  Set RFC-2090 in case of multicast request
          if (option_name == OptExtent::multicast) {
            const string delim{","};
            string val_array{buffer};
            size_t pos{0};
            vector<string> multicast_val;
            val_array.erase(remove_if(val_array.begin(), val_array.end(), isspace), val_array.end());
            transform(val_array.begin(), val_array.end(), val_array.begin(), ::tolower);

            do {
              pos = val_array.find(delim);
              multicast_val.push_back(val_array.substr(0, pos));
              val_array.erase(0, pos + delim.length());
            } while (pos != string::npos);
            if (checkIPRange(multicast_val.at(0))) {
              multicast = make_tuple(multicast_val.at(0), (uint16_t) stoi(multicast_val.at(1)), (bool)stoi(multicast_val.at(2)));
            } else {
              return false;
            }
            ++count_mode;  
            continue;
          }
          count_begin = stoi(buffer);
          options.emplace_back(std::make_pair(option_name, count_begin));
          ++count_mode;
        }

        if (!options.empty()) {
          req_params.emplace(options);
        }
        return ret;
      };

      auto getData = [this, pack_size](int opcode) ->bool {
        bool ret{ true };
        char blk_num[2];

        if (pack_size < DATA_MIN_SIZE) {
          return false;
        }

        //Block number
        memcpy(blk_num, &packet[2], sizeof(uint16_t));
        int block_number{std::stoi(blk_num)};
        std::get<0>(packet_frame_structure) = OptCode.at(opcode);
        std::get<1>(packet_frame_structure) = optional<TFTPError>{};
        std::get<2>(packet_frame_structure) = optional<TFTPMode>{};
        std::get<3>(packet_frame_structure) = optional<uint16_t>(block_number);
        std::get<4>(packet_frame_structure) = optional<uint16_t>(5);
        std::get<5>(packet_frame_structure) = optional<uint16_t>(pack_size - 1);
        return ret;
      };

      auto getACK = [this, pack_size](int opcode) -> bool {
        bool ret{ true };
        char blk_num[2];

        if (pack_size < ACK_MIN_SIZE) {
          return false;
        }

        //Block number
        memcpy(blk_num, &packet[2], sizeof(uint16_t));
        uint16_t block_number{(uint16_t) std::stoi(blk_num)};
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
        char blk_num[2];

        if (pack_size < ERROR_MIN_SIZE) {
          return false;
        }

        //Error number
        memcpy(blk_num, &packet[2], sizeof(uint16_t));
        uint16_t error_code{(uint16_t) std::stoi(blk_num)};
        std::get<0>(packet_frame_structure) = OptCode.at(opcode);
        std::get<1>(packet_frame_structure) = optional<TFTPError>{ ErrorCode.at(error_code) };
        std::get<2>(packet_frame_structure) = optional<TFTPMode>{};
        std::get<3>(packet_frame_structure) = optional<uint16_t>{};
        std::get<4>(packet_frame_structure) = optional<uint16_t>{ 5 };
        std::get<5>(packet_frame_structure) = optional<uint16_t>{ pack_size - 2 };
        return ret;
      };

      const unordered_map<int, function<bool(int)>> req_data{ {1, reqRW}, {2, reqRW}, {3, getData}, {4, getACK}, {5, getERROR} };
      opcode = netToHost(packet);
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
      try {
        auto dataLayOut{ req_data.at(opcode) };
        ret = dataLayOut(opcode);
      } catch (...) {
        ret = false;
      }
      return ret;
    }
    //  Get parameters for new clients request transfer session
    [[nodiscard]] bool getParams(struct sockaddr_storage addr_stor, optional<size_t> io_port) noexcept {
      bool ret {true};
      fs::path path;
      bool read_file{false}, bin_operation {false};
      optional<uint16_t> buffer{}, timeout{}, transfer_size{}, multicast_port{};
      optional<string> multicast_addr;
      optional<bool> multicast_master;

      try {
        //  Path from local root dir
        path = std::get<6> (packet_frame_structure).value();
        if (auto op_code {std::get<0>(packet_frame_structure)}; op_code == TFTPOpeCode::TFTP_OPCODE_READ) {
          read_file = true;
        }
        if (auto transfer_mode {std::get<2>(packet_frame_structure)}; transfer_mode == TFTPMode::octet) {
          bin_operation = true;
        }
        if (req_params) {
          for (auto &param : *req_params) {
            switch (auto param_name {param.first}; param_name) {
              case OptExtent::multicast : [[fallthrough]];
              case OptExtent::tsize : transfer_size = param.second; break;
              case OptExtent::timeout : timeout = param.second; break;
              case OptExtent::blksize : buffer = param.second; break;
            }
          }
        }
        if (multicast) {
          multicast_addr = std::get<0>(multicast.value());
          multicast_port = std::get<1>(multicast.value());
          multicast_master = std::get<2>(multicast.value());
        }
        trans_params = make_tuple(path, read_file, bin_operation, io_port, buffer, timeout, transfer_size, multicast_addr, multicast_port, addr_stor);
      } catch (...) {
        if (ret) {
          ret = false;
        }
      }
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
      memcpy(&Packet<T>::packet[0], &op_code, overhead_field_size);
    }
    bool setData(uint16_t pack_count, ReadFileData<T>* msg) {
      bool ret{ false };
      if (msg->size > Packet<T>::packet_size - overhead_field_size) {
        return ret;
      }
      const auto net_pack_code{htons(pack_count)};
      ret = memcpy(&Packet<T>::packet[pos], &net_pack_code, overhead_field_size);
      if (!ret) {
        return ret;
      }
      pos += overhead_field_size;
      ret = memcpy(&Packet<T>::packet[pos], msg->data, msg->size); 
      return ret;
    }
    ~SendData() = default;
  };
  //  TODO: Check data analysis process - PacketTools!!!
  //  Receive & analyze clients packet
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
      memcpy((char*)&BasePacket<PACKET_ACK_SIZE, char>::packet[0], &op_code, sizeof(op_code));
      memmove((char*)&BasePacket<PACKET_ACK_SIZE, char>::packet[2], &num, sizeof(pack_number));
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
  //  RFC 2347 and above parameters negotiation request support packet
  struct OACKPacket : Packet <char> {
    //  Set size of total packet length - opcode + param ID + divided zero + param value etc...
    OACKPacket(OACKOption* val) : Packet{} {
      const uint16_t opcode {htons(6)};
      char draft_packet[PACKET_MAX_SIZE];
      uint16_t pos{0};
      uint8_t param_size;
      string str_val;
      
      //  Converting parameters into packet sting format values 
      auto makeParam = [&draft_packet, &pos, &str_val, &param_size] (ReqParam *opt) {
        str_val = OptExtVal.at(opt->first);
        param_size = str_val.size();
        memcpy(&draft_packet[pos], str_val.c_str(), param_size);
        pos += param_size;
        draft_packet[pos] = '\0';
        ++pos;
        str_val = std::to_string(opt->second);
        param_size = str_val.size();
        memcpy(&draft_packet[pos], str_val.c_str(), param_size);
        pos += param_size;
        draft_packet[pos] = '\0';
        ++pos;
      };
      
      if (auto opt{std::get<0>(*val)}; opt) {
        makeParam (&opt.value());
      }
      if (auto opt{std::get<1>(*val)}; opt) {
        makeParam (&opt.value());
      }
      if (auto opt{std::get<2>(*val)}; opt) {
        makeParam (&opt.value());
      }
      //  Multicast parameters set
      if (auto opt{std::get<3>(*val)}; opt) {
        memcpy(&draft_packet[pos], "multicast", 9);
        pos += 9;
        draft_packet[pos] = '\0';
        ++pos;
        str_val = std::get<0>(opt.value());
        memcpy(&draft_packet[pos], str_val.c_str(), str_val.size());  
        pos += str_val.size();
        draft_packet[pos] = ',';
        ++pos;
        str_val = std::to_string(std::get<1>(opt.value()));
        memcpy(&draft_packet[pos], str_val.c_str(), str_val.size());
        pos += str_val.size();
        draft_packet[pos] = ',';
        ++pos;
        str_val = std::to_string(std::get<2>(opt.value()));
        memcpy(&draft_packet[pos], str_val.c_str(), str_val.size());
        pos += str_val.size();
        draft_packet[pos] = '\0';
      }

      packet_size = pos + 2;
      packet = new char[packet_size];
      memcpy(packet, &opcode, sizeof(opcode));
      memcpy(packet+2, draft_packet, pos + 1);
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
  template <typename PoolType, template <typename> typename PoolCont>
  class ShareResPool {
    public:
      explicit ShareResPool(size_t pull_size) : pool_max_size{ pull_size } {thr_pool = make_unique<PoolCont<PoolType>>();}
      ~ShareResPool() = default;

    ShareResPool (ShareResPool&) = delete;
    ShareResPool (ShareResPool&&) = delete;
    ShareResPool& operator = (ShareResPool&) = delete;
    ShareResPool& operator = (ShareResPool&&) = delete;

    [[nodiscard]] PoolType getRes(void) noexcept {
      PoolType ret {nullptr}; 
      lock_guard<std::mutex> pool_lock(pool_access);
      if (!thr_pool->size()) {
        return ret;
      }
      ret = thr_pool->front();
      thr_pool->pop_front();
      return ret;
    }
    [[nodiscard]] bool setRes(PoolType thr) noexcept {
      bool ret{ false };
      lock_guard<std::mutex> pool_lock(pool_access);
      if (thr_pool->size() >= pool_max_size) {
        return ret;
      }
      thr_pool->emplace_back(thr);
      return true;
    }
    [[nodiscard]] bool poolAvailable(void) const noexcept {
      bool ret{ false };
      if (thr_pool->size()) {
        ret = true;
      }
      return ret;
    }
    protected:
      unique_ptr<PoolCont<PoolType>> thr_pool;
    private:
      const size_t pool_max_size; // Number of resources in pool
      mutex pool_access;
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
    FileIO(const FileMode* const mode) : FileIO(std::get<0>(*mode), std::get<1>(*mode), std::get<2>(*mode)) {}
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

    //  Read from file, as usual is
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
    //  Read file part according requested size
    //  true - success
    //  false - EOF
    //  string - error text
    template <typename T> requires TransType<T>
    [[nodiscard]] variant<bool, string> readFile(ReadFileData<T>* buffer) noexcept {
      variant<bool, string> ret{true};
      if constexpr (std::is_same<T, byte>::value) {
        read_file.read((char*)buffer->data, buffer->size);
      }
      else {
        read_file.read(buffer->data, buffer->size);
      }
      if (!read_file) {
        ret = strerror(errno);
      }
      if (read_file.eof()) {
        ret = false;
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
    template <typename T> requires TransType<T>
    bool writeType(T* str) noexcept {
      bool ret{ true };
      write_file << str << std::flush;
      if (write_file.bad()) {
        ret = false;
      }
      return ret;
    }
    template <typename T> requires TransType<T>
    [[nodiscard]] variant<bool, string> writeFile(T* str) noexcept {
      variant<bool, string> ret{true};
      std::cout<<"Writing to file : " << str<<std::endl<<std::flush;
      write_file << str << std::flush;
      if (write_file.bad()) {
        ret = strerror(errno);
      }
      return ret;
    }
    [[nodiscard]] fs::path getFilePath(void) const noexcept {
      return file_name;
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


  // TODO : Change exceptions to throw all errors to upper level
  //  Base class for networking
  //  - creating sockets and a few general transfer network options
  class BaseNet {
  public:
    //  Transfer param
    const size_t buff_size{ 512 };
    const size_t timeout{ 3 };
    const size_t file_size{ 0 };
    
    BaseNet(size_t port) : port{ port } {
      if (!init(port)) {
        std::cerr << "Socket init problem" << std::endl;
      }
    }
    BaseNet() : BaseNet(DEFAULT_PORT) {}
    BaseNet(const size_t port, const size_t buff_size, const size_t timeout, const size_t file_size, struct sockaddr_storage cln_addr)
      : buff_size{ buff_size }, timeout{ timeout }, file_size{ file_size }, port{ port },  cliaddr{cln_addr}{
      try {
        if (!init(port)) {
          std::cerr << "Socket init problem" << std::endl;
        }

        //  Buffer size setup
        if (buff_size != 512) {
          if (setsockopt(sock_id, SOL_SOCKET, SO_RCVBUF, &buff_size, sizeof(buff_size))) {
            throw std::runtime_error("Socket SET RECEIVE BUFFER SIZE error");
          }
        }

        //  In/Out traffic timeout  setup
        if (timeout != 3) {
          if (setsockopt(sock_id, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout))) {
            throw std::runtime_error("Socket SET RECEIVE TIMEOUT error");
          }
        }
      }
      catch (const std::exception& e) {
        std::cerr << "BaseNet - ";
        std::cerr << e.what() << std::endl;
      }
    }
    BaseNet(const size_t buff_size, const size_t timeout, const size_t file_size, struct sockaddr_storage cln_addr)
      : BaseNet(DEFAULT_PORT, buff_size, timeout, file_size, cln_addr) {}
    BaseNet(const size_t port, struct sockaddr_storage cln_addr)
      : BaseNet(port, 0, 0, 0, cln_addr) {}
    //  Creating standard net IO socket class or multicast socket if in file mode multicast settings exists 
    BaseNet(const FileMode* const trans_mode) {
      try {
        if (auto mult_addr {std::get<7>(*trans_mode)}; mult_addr) {
          if (auto mult_port{std::get<8>(*trans_mode)}; mult_port) {
            port = mult_port.value();
          } else {
            port = MULTICAST_DEFAULT_PORT; 
          }
          multicast_address = mult_addr.value();
          if (!init_multicast()) {
            throw std::runtime_error ("Multicast socket CREATING error");
        }
        } else {
          if (auto ftp_port {std::get<3>(*trans_mode)}; ftp_port) {
            port = ftp_port.value();
             if (!init(port)) {
               std::cerr << "Socket init problem" << std::endl;
             }
          }
        }
        
        
        if (auto blk {std::get<4>(*trans_mode)}; blk) {
          auto blk_val{blk.value()};
          if (setsockopt(sock_id, SOL_SOCKET, SO_RCVTIMEO, &blk_val, sizeof(blk_val))) {
              throw std::runtime_error("Socket SET RECEIVE BLOCK SIZE error");
          }
        }

        if (auto timeout {std::get<3>(*trans_mode)}; timeout) {
          auto timeout_val{timeout.value()};
          if (setsockopt(sock_id, SOL_SOCKET, SO_RCVTIMEO, &timeout_val, sizeof(timeout_val))) {
              throw std::runtime_error("Socket SET RECEIVE TIMEOUT error");
          }
        }
      }
      catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
      }
      
    }
    
    virtual ~BaseNet() {
      if (sock_id) {
        close(sock_id);
      }
    }

    BaseNet(const BaseNet&) = delete;
    BaseNet(const BaseNet&&) = delete;
    BaseNet& operator = (const BaseNet&) = delete;
    BaseNet& operator = (const BaseNet&&) = delete;

    // Send packet to client
    template <typename T> requires TransType<T>
    [[nodiscard]] ssize_t sndMulticastData(DataPacket<T>* const data_pack) noexcept {
      ssize_t send_result;
      send_result = sendto(sock_id, data_pack->packet, data_pack->packet_size, MSG_CONFIRM, (const struct sockaddr*)&multicast_int, sizeof(multicast_int));
      return send_result;
    }
    //  Send to client OACK packet
    [[nodiscard]]bool sendOACK(optional<uint16_t> file_size, optional<uint16_t> blk_size, optional<uint16_t> timeout, optional<string> ip_addr, optional<uint16_t> port) noexcept {
      bool ret = true;
      optional<MulticastOption> mult;
      optional<ReqParam> t_size, b_size, t_out;
 
      if (file_size) {
        t_size = make_pair(OptExtent::tsize , file_size.value());
      }
      if (blk_size) {
        b_size = make_pair(OptExtent::blksize, blk_size.value());
      }
      if (timeout) {
        t_out = make_pair(OptExtent::timeout, timeout.value());
      }
      if (ip_addr) {
        auto val {mult.value()};
        std::get<0> (val) = ip_addr.value();
        std::get<1> (val) = port.value();
        std::get<2> (val) = true;
      }
      
      OACKOption val {make_tuple(t_size, b_size, t_out, mult)};
      OACKPacket data{&val};
      auto res = sendto(sock_id, &data.packet, data.packet_size, MSG_CONFIRM, (const struct sockaddr*)&cliaddr, cli_addr_size);
      if (res == SOCKET_ERR) {
        ret = false;
      }
      return ret;
    }
    [[nodiscard]]bool sendOACK(OACKOption* val) noexcept {
      bool ret {true};
      OACKPacket data{val};
      auto res = sendto(sock_id, &data.packet, data.packet_size, MSG_CONFIRM, (const struct sockaddr*)&cliaddr, cli_addr_size);
      if (res == SOCKET_ERR) {
        ret = false;
      }
      return ret;
    }
  protected:
    //  Socket params
    size_t port;
    int sock_id {0}; 
    struct sockaddr_in address;
    int opt {1};
    struct sockaddr_storage cliaddr;  //  Client connection address 
    int addrlen {sizeof(address)};
    socklen_t  cli_addr_size;
    string multicast_address;
    struct sockaddr_in multicast_int;
    struct in_addr local_int;
    

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
    [[nodiscard]] bool init(const size_t port = DEFAULT_PORT) noexcept {
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

      // Looking for suitable interface
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
    [[nodiscard]] bool init_multicast () noexcept {
      bool ret {false};
      struct  sockaddr_in addr;
      socklen_t len = sizeof addr;
      memset((char *) &multicast_int, 0, sizeof(multicast_int));
      multicast_int.sin_family = AF_INET;
      multicast_int.sin_addr.s_addr = inet_addr(multicast_address.c_str());
      multicast_int.sin_port = htons(port);
      ret = init(port);
      if (!ret) {
        return ret;
      }
      if (auto res{getsockname(sock_id, (struct sockaddr*)&addr, &len)}; res == -1) {
        return ret;
      }
      local_int.s_addr = inet_addr(inet_ntoa(addr.sin_addr));
      ret = setsockopt(sock_id, IPPROTO_IP, IP_MULTICAST_IF, (char *)&local_int, sizeof(local_int));
      
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
    //  Check if requested params (RFC 1782) are compatible with current settings
    bool checkParam(FileMode* const req_param, optional<size_t> file_size) {
      if (!req_param) {
        return false;
      }
      if (auto buff_size{std::get<4>(*req_param)}; buff_size) {
        if (buff_size.value() > max_buff_size) {
          std::get<4>(*req_param) = max_buff_size;
        }
      }
      if (auto timeout{std::get<5>(*req_param)}; timeout) {
        if (timeout.value() > max_time_out) {
          std::get<5>(*req_param) = max_time_out;
        }
      }
      if (auto req_file_size{std::get<6>(*req_param)}; !req_file_size && file_size) {
        std::get<6>(*req_param) = file_size.value();
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
    atomic<bool> upd_stat {false};  //  Flag to update transfer statistics
    size_t transfer_size{0};  //  Download/Upload data size
    time_point<system_clock> timestamp;  // Transfer update statistic last date
    
    //  Constructors for ordinary (point to point) data transfer
    NetSock(size_t port,
      const fs::path file_name,
      const bool read,
      const bool bin,
      atomic<bool>* const terminate,
      atomic<bool>* const terminate_local)
      : BaseNet{ port },
        FileIO{ file_name, read, bin }, 
        terminate_transfer{ terminate },
        terminate_local{terminate_local} {}
    NetSock(const size_t port,
      const size_t buff_size,
      const size_t timeout,
      const size_t file_size,
      struct sockaddr_storage cln_addr,
      const std::filesystem::path file_name,
      const bool read,
      const bool bin,
      atomic<bool>* terminate,
      atomic<bool>* const terminate_local)
      : BaseNet{ port, buff_size, timeout, file_size, cln_addr },
        FileIO{ file_name, read, bin },
        terminate_transfer{ terminate },
        terminate_local{terminate_local} {}
    NetSock(size_t port,
      const fs::path file_name,
      const bool read,
      const bool bin,
      atomic<bool>* terminate,
      atomic<bool>* const terminate_local,
      shared_ptr<Log> log)
      : NetSock{ port, file_name, read, bin, terminate, terminate_local} {
          this->log = log;
        }
    NetSock(const size_t port,
      const size_t buff_size,
      const size_t timeout,
      const size_t file_size,
      struct sockaddr_storage cln_addr,
      const std::filesystem::path file_name,
      const bool read,
      const bool bin,
      atomic<bool>* const terminate,
      atomic<bool>* const terminate_local,
      const shared_ptr<Log> log)
      : NetSock(port,
        buff_size,
        timeout,
        file_size,
        cln_addr,
        file_name,
        read,
        bin,
        terminate,
        terminate_local) {
          this->log = log;
        }
    //  Multicast constructor
    NetSock(const FileMode* const mode,
     atomic<bool>* const terminate,
     atomic<bool>* const terminate_local,
     const shared_ptr<Log> log)
       : BaseNet {mode},
         FileIO {mode},
         terminate_transfer{ terminate },
         terminate_local{terminate_local},
         log{log} {
           if (std::get<7>(*mode)) {
             mult_transfer = make_unique<BaseNet>(mode);
           }
         }

    ~NetSock() = default;

    NetSock(const NetSock&) = delete;
    NetSock(const NetSock&&) = delete;
    NetSock& operator = (const NetSock&) = delete;
    NetSock& operator = (const NetSock&&) = delete;

    template <typename T> requires TransType<T>
    [[nodiscard]]  bool readFile(void) noexcept {
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
      while (!terminate_transfer->load() && !terminate_local->load() && run_transfer) {
        //  Read data from disk
        read_result = readType<T>(&data);
        

        if (!read_result) {
          ConstErrorPacket<FILE_READ_ERR_SIZE> error(TFTPError::Access_Violation, (char*)&FILE_READ_ERR);
          sendto(sock_id, &error.packet, error.size, MSG_CONFIRM, (const struct sockaddr*)&cliaddr, cli_addr_size);
          if (log) {
            log->debugMsg(__PRETTY_FUNCTION__, "Read operation failed");
          }
          return false;
        }

        //  Upd transfer statistics
        transfer_size += read_result;
        if (upd_stat.load()) {
          timestamp = system_clock::now();
          upd_stat = false;
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
        } else {
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

        //  Send data packet 
        if (run_transfer) {
          if (mult_transfer) {
            send_result = mult_transfer->sndMulticastData(&data_pack);
          } else {
            send_result = sendto(sock_id, &data_pack.packet, data_pack.packet_size, MSG_CONFIRM, (const struct sockaddr*)&cliaddr, cli_addr_size);
          }
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
    [[nodiscard]] bool writeFile(bool delete_if_error = false) noexcept {
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
      while (run_transfer && !terminate_transfer->load() && !terminate_local->load()) {
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
        
        //  Upd transfer statistics
        transfer_size += data.getDataSize().value();
        if (upd_stat.load()) {
          timestamp = system_clock::now();
          upd_stat = false;
        }
      }

      //  Delete received data
      if (terminate_transfer->load() and delete_if_error) {
        if (std::filesystem::exists(file_name)) {
          std::filesystem::remove(file_name);
        }
      }
      return ret;
    }
  
  
  private:
    atomic<bool>* terminate_transfer, *terminate_local;
    ACKPacket ack{};
    shared_ptr<Log> log;
    unique_ptr<BaseNet> mult_transfer;
  };
}

namespace MemoryManager {
  //  Pool allocator
  class PoolAllocator {
    public :
      atomic<bool> buff_not_busy{false};
      unique_lock<std::mutex>  wait_thr_busy;
      condition_variable thr_copy_finish;

      explicit PoolAllocator(const size_t pool_size) : total_size{pool_size} {
        pool_point = malloc(pool_size);
        wait_thr_busy = std::unique_lock<std::mutex>(pool_mut, std::adopt_lock);
      }
      PoolAllocator(const size_t blk_size, const size_t blk_num) 
        : total_size {blk_size * blk_num}, block_size{make_unique<size_t>(blk_size)}, blocks_number {make_unique<size_t>(blk_num)} {
          pool_point = malloc(total_size);
          wait_thr_busy = std::unique_lock<std::mutex>(pool_mut, std::adopt_lock);
      }
      ~PoolAllocator() {
        if (pool_point) {
          free(pool_point);
        }
      }

      PoolAllocator(const PoolAllocator&) = delete;
      PoolAllocator(const PoolAllocator&&) = delete;
      PoolAllocator& operator = (const PoolAllocator&) = delete;
      PoolAllocator& operator = (const PoolAllocator&&) = delete;
      
      

      bool setRow(void* const data, const size_t size) noexcept {
        bool ret {false};
        if (!data || size < 0) {
          return ret;
        }
        auto free_space = total_size - used_size;
        if (size > free_space) {
          return ret;
        }
        
        auto res = memcpy(pool_point + used_size, data, size);
        if (!res) {
          return ret;
        } else {
          ret = true;
        }
        used_size += size;
        return ret;
      }
      bool setBlk(void* const data, const size_t blk_num) noexcept {
        bool ret{false};
        if (!data) {
          return ret;
        }
        if (!block_size || !blocks_number) {
          return ret;
        }
        auto request_size{*block_size * blk_num};
        if (auto free_space{total_size - used_size}; request_size > free_space) {
          return ret;
        }
        auto res = memcpy(pool_point + used_size, data, request_size);
        if (!res) {
          return ret;
        } else {
          ret = true;
        }
        used_size += request_size;
        *blocks_number += blk_num;
        return ret;
      }
      template <typename T> requires TransType<T>
      bool setDat(ReadFileData<T>* const data) noexcept {
        bool ret{false};
        if (!data) {
          return ret;
        }
        if (auto free_space{total_size - used_size}; data->size > free_space) {
          return ret;
        }
        auto res = memcpy(pool_point + used_size, data->data, data->size);
        if (!res) {
          return ret;
        } else {
          ret = true;
        }
        used_size += data->size;
        return ret;
      }
      bool getRow (void* const data, const size_t size) noexcept {
        bool ret {false};
        if (!data) {
          return ret;
        }
        if (size > used_size) {
          return ret;
        }
        auto request_point{pool_point + (used_size- size)};
        auto res = memcpy(data, request_point, size);
        if (!res) {
          return ret;
        } else {
          ret = true;
        }
        used_size -= size;
        return ret;
      }
      bool getBlk(void* const data, const size_t blk_num) noexcept {
        bool ret {false};
        if (!data || blk_num < 1) {
          return ret;
        }
        if (!blocks_number || !blocks_number) {
          return ret;
        }
        auto request_size{*block_size * blk_num};
        if ( request_size > used_size) {
          return ret;
        }
        auto request_point {pool_point + (used_size - request_size)};
        auto res = memcpy(data, request_point, request_size);
        if (!res) {
          return ret;
        } else {
          ret = true;
        }
        *blocks_number -= blk_num;
        used_size -= request_size;
        return ret;
      }
      template <typename T> requires TransType<T>
      bool getDat(ReadFileData<T>* const data) noexcept {
        bool ret {false};
        if (!data) {
          return ret;
        }
        if (data->size > used_size) {
          return ret;
        }
        auto request_point{pool_point + (used_size - data->size)};
        auto res = memcpy(data->data, request_point, data->size);
        if (!res) {
          return ret;
        } else {
          ret = true;
        }
        used_size -= data->size;
        return ret;
      }
      void clear (void) noexcept {
        used_size = 0;
      }
      bool reSet(const size_t blk_num, const size_t blk_size) {
        bool ret{false};
        auto req_size {blk_num * blk_size};
        if (req_size > total_size) {
          return ret;
        }
        if (!block_size || !blocks_number) {
          return ret;
        }
        *block_size = blk_size;
        *blocks_number = blk_num;
      }
      bool setReverseOrder(void* const source, size_t size, const size_t blk_size) noexcept {
        bool ret{false};
        void* local_buff = malloc(size);
        size_t local_buff_size{size};
        memcpy(local_buff, source, size);

        while (local_buff_size) {
          if (local_buff_size > blk_size) {
            local_buff_size -= blk_size;
            setRow(local_buff + local_buff_size, blk_size);
          } else {
            setRow(local_buff, local_buff_size);
            local_buff_size = 0;
          }
        }
        if (local_buff_size == 0) {
          ret = true;
        }
        string s {(char*)source, size};
        string ss {(char*) pool_point, used_size};
        std::cout<<"Buffer content " << ss<<std::endl<<std::flush;
        if (local_buff) {
          free (local_buff);
        }
        
        return ret;
      }
      [[nodiscard]] size_t getTotalSize (void) noexcept {
        return total_size;
      }
      [[nodiscard]] size_t getUsedSize (void) noexcept {
        return used_size;
      }
    private :
      size_t total_size;
      size_t used_size{0};
      unique_ptr<size_t> block_size;
      unique_ptr<size_t> blocks_number;
      void* pool_point{nullptr};
      mutex pool_mut;
  };

  //  Buffer for IO/Net operations
  class IOBuff {
    private :
      unique_ptr<PoolAllocator> first, second;
      unique_ptr<FileIO> file;
      size_t blk_size {0};
      size_t buff_size{0};
      size_t file_size{0};
      size_t current_download_size{0};
      unique_ptr<jthread> dskIOThr;
      mutex dsk_mut;
      unique_lock<std::mutex> wait_cash_operation;
      condition_variable continue_io;
      atomic<bool> stop_io{false};
      PoolAllocator *active_buff, *passive_buff;
      shared_ptr<Log> log;

      struct DskStopThrVisitor {
        jthread* const thr{nullptr};
        shared_ptr<Log> log;
        FileIO* const file{nullptr};
        bool break_thr{false};

        explicit DskStopThrVisitor(jthread* const thr) : thr{thr}{}
        DskStopThrVisitor(jthread* const thr, shared_ptr<Log> log, FileIO* const file) : thr{thr}, log{log}, file{file}{}
        void operator()(const bool &condition) {
          if (!condition) {
            thr->request_stop();
            break_thr = true;
          }
        }
        void operator()(const string& str){
          if (log) {
            string msg {"Operation with file"};
            msg += file->getFilePath().string();
            log->errMsg(msg, str);
          }
          thr->request_stop();
          break_thr = true;
        }
      };

      //  Write buffers cashed data to file (from passive one)
      template <typename T> requires TransType<T>
      void toDskThr(std::stop_token stop_token) {
        T buff_data[buff_size];
        bool ret {false};
        variant<bool, string> res_var;
        unique_ptr<DskStopThrVisitor> thr_stop_vis;

        if (log) {
          thr_stop_vis = make_unique<DskStopThrVisitor>(dskIOThr.get(), log, file.get());
        } else {
          thr_stop_vis = make_unique<DskStopThrVisitor>(dskIOThr.get());
        }
        
        while (!stop_token.stop_requested()) {
          passive_buff->clear();
          stop_io = false;
          passive_buff->thr_copy_finish.notify_all();
          passive_buff->buff_not_busy = true;
          continue_io.wait(wait_cash_operation, [this]{return stop_io.load();});
          passive_buff->buff_not_busy = false;
          if (stop_token.stop_requested()) {
            passive_buff->buff_not_busy = true;
            passive_buff->thr_copy_finish.notify_all();
            break;
          }
          if (auto current_dat{passive_buff->getUsedSize()}; current_dat < buff_size) {
            T last_data[current_dat];
            ret = passive_buff->getRow(&last_data, current_dat);
            res_var = file->writeFile<T>(last_data);
          } else {
            ret = passive_buff->getRow(&buff_data, buff_size);
            res_var = file->writeFile<T>(buff_data);
          }
          std::visit(*thr_stop_vis, res_var);
          if (thr_stop_vis->break_thr || !ret) {
            passive_buff->buff_not_busy = true;
            passive_buff->thr_copy_finish.notify_all();
          }
        }
      }
      //  Fill cash buffer (in reverse order) by requested file data
      template <typename T> requires TransType<T>
      void fromDskThr(std::stop_token stop_token) {
        size_t data_rest{file_size};
        ReadFileData<T> read_buff(buff_size);
        variant<bool, string> res_var;
        bool res;
        unique_ptr<DskStopThrVisitor> thr_stop_vis;

        if (log) {
          thr_stop_vis = make_unique<DskStopThrVisitor>(dskIOThr.get(), log, file.get());
        } else {
          thr_stop_vis = make_unique<DskStopThrVisitor>(dskIOThr.get());
        }
       
        while (!stop_token.stop_requested()) {
          passive_buff->buff_not_busy = false;
          passive_buff->clear();
          if(data_rest < buff_size) {
            ReadFileData<T> read_rest(data_rest);
            res_var = file->readFile<T>(&read_rest);
            res = passive_buff->setReverseOrder(read_rest.data, read_rest.size, blk_size);
          } else {
            res_var = file->readFile<T>(&read_buff);
            res = passive_buff->setReverseOrder(read_buff.data, read_buff.size, blk_size);
          }
          std::visit(*thr_stop_vis, res_var);
          if (thr_stop_vis->break_thr || !res) {
            passive_buff->buff_not_busy = true;
            passive_buff->thr_copy_finish.notify_all();
            continue;
          }
          data_rest -= buff_size;
          stop_io = false;
          passive_buff->buff_not_busy = true;
          passive_buff->thr_copy_finish.notify_all();
          continue_io.wait(wait_cash_operation, [this]{return stop_io.load();});
        }
        passive_buff->buff_not_busy = true;
        passive_buff->thr_copy_finish.notify_all();
      }
      void swapBuff(void) {
        auto tmp = passive_buff;
        passive_buff = active_buff;
        active_buff = tmp;
      }
      bool stopThr(void) {
        bool ret {false};
        stop_io = true;

        if (!wait_cash_operation.owns_lock()) {
          return true;
        }
        auto token = dskIOThr->get_stop_token();
        if (!token.stop_possible())  {
          return ret;
        }
        if (token.stop_requested()) {
          dskIOThr->join();
          return true;
        }
        ret = dskIOThr->request_stop();
        continue_io.notify_one();
        if (!ret) {
          return ret;
        }
        dskIOThr->join();
        if (!dskIOThr->joinable()){
          ret = true;
        }
        // swapBuff();
        // stop_io = true;
        // continue_io.notify_one();
        return ret;
      }
      void reStartThr(void) noexcept {
        swapBuff();
        stop_io = true;
        continue_io.notify_one();
      }
    public :
      explicit IOBuff (const size_t buff_size) 
      : first{make_unique<PoolAllocator>(buff_size)}, 
        second{make_unique<PoolAllocator>(buff_size)},
        buff_size{buff_size},
        wait_cash_operation{std::unique_lock<std::mutex>(dsk_mut, std::adopt_lock)}
        {}
      IOBuff (const size_t blk_size, const size_t blk_num) 
      : first{make_unique<PoolAllocator>(blk_size, blk_num)},
        second{make_unique<PoolAllocator>(blk_size, blk_num)},
        buff_size {blk_size * blk_num},
        wait_cash_operation {std::unique_lock<std::mutex>(dsk_mut, std::adopt_lock)}
      {}
      IOBuff (const size_t buff_size, std::shared_ptr<Log> log) 
      : first{make_unique<PoolAllocator>(buff_size)},
        second{make_unique<PoolAllocator>(buff_size)},
        buff_size{buff_size},
        wait_cash_operation {std::unique_lock<std::mutex>(dsk_mut, std::adopt_lock)},
        log{log}
      {}
      IOBuff (const size_t blk_size, const size_t blk_num, std::shared_ptr<Log> log) 
      : first{make_unique<PoolAllocator>(blk_size, blk_num)},
        second{make_unique<PoolAllocator>(blk_size, blk_num)},
        buff_size {blk_size * blk_num},
        wait_cash_operation {std::unique_lock<std::mutex>(dsk_mut, std::adopt_lock)},
        log{log}
      {}
      ~IOBuff() {
        if (dskIOThr) {
          stopThr();
        }
        if (active_buff) {
          active_buff->thr_copy_finish.notify_all();
        }
        if (passive_buff) {
          passive_buff->thr_copy_finish.notify_all();
        }
      }

      IOBuff(const IOBuff&) = delete;
      IOBuff(const IOBuff&&) = delete;
      IOBuff& operator = (const IOBuff&) = delete;
      IOBuff& operator = (const IOBuff&&) = delete;
      
      void reSetSession(const FileMode* const mode) {
        if (!file) {
          file = make_unique<FileIO>(mode);
        } else {
          file.reset(new FileIO(mode));
        }
        if (std::get<4>(*mode)) {
          this->blk_size = std::get<4>(*mode).value();
        } else {
          this->blk_size  = 512;
        }
        active_buff = first.get();
        passive_buff = second.get();
        current_download_size = 0;
        
        //  Read mode - read to buffer from file
        if (std::get<1>(*mode)) {
          file_size = fs::file_size(std::get<0>(*mode));
          if (std::get<2>(*mode)) {
            if (!dskIOThr) {
              dskIOThr = make_unique<jthread> (&IOBuff::fromDskThr<byte>, this);
            } else {
              stopThr();
              dskIOThr.reset (new jthread(&IOBuff::fromDskThr<byte>, this));
            }
          } else {
            if (!dskIOThr) {
              dskIOThr = make_unique<jthread> (&IOBuff::fromDskThr<char>, this);
            } else {
              stopThr();
              dskIOThr.reset (new jthread(&IOBuff::fromDskThr<char>, this));
            }
          }
          while (!passive_buff->buff_not_busy.load()) {
            passive_buff->thr_copy_finish.wait_for(passive_buff->wait_thr_busy, milliseconds(5), [this]{return passive_buff->buff_not_busy.load();});
          }
          reStartThr();
        } else {  //  Write mode - write from buffer to file
          
         
          file_size = std::get<6>(*mode).value();
          if (std::get<2>(*mode)) {
            if (!dskIOThr) {
              dskIOThr = make_unique<jthread> (&IOBuff::toDskThr<byte>, this);
            } else {
              while (!passive_buff->buff_not_busy.load()) {
                passive_buff->thr_copy_finish.wait_for(passive_buff->wait_thr_busy, milliseconds(5), [this]{return passive_buff->buff_not_busy.load();});
              }
              if (passive_buff) {
                passive_buff->buff_not_busy = true;
              }
              if (active_buff) {
                active_buff->buff_not_busy = true;
              }
              stopThr();
              dskIOThr.reset (new jthread(&IOBuff::toDskThr<byte>, this));
            }
          } else {
            if (!dskIOThr) {
              dskIOThr = make_unique<jthread> (&IOBuff::toDskThr<char>, this);
            } else {
              while (!passive_buff->buff_not_busy.load()) {
                passive_buff->thr_copy_finish.wait_for(passive_buff->wait_thr_busy, milliseconds(5), [this]{return passive_buff->buff_not_busy.load();});
              }
              if (passive_buff) {
                passive_buff->buff_not_busy = true;
              }
              if (active_buff) {
                active_buff->buff_not_busy = true;
              }
              stopThr();
              dskIOThr.reset (new jthread(&IOBuff::toDskThr<char>, this));
            }
          }
        }
      }
      template <typename T> requires TransType<T>
      [[nodiscard]] bool readData(ReadFileData<T>* const data) noexcept {
        bool ret{false};
        if (!data) {
          return ret;
        }
        ret = active_buff->getDat(data);
        if (!ret) {
          return ret;
        }
        if (!active_buff->getUsedSize()) {
          //  Check if buffer still in work (busy), waiting for finish
          if (stop_io.load()) {
            std::this_thread::sleep_for(milliseconds(1)); 
          }
          if (passive_buff) {
            while (!passive_buff->buff_not_busy.load()) {
              passive_buff->thr_copy_finish.wait_for(active_buff->wait_thr_busy, milliseconds(5), [this]{return active_buff->buff_not_busy.load();});
            }
          reStartThr();
          } else {
            return ret;
          }
          ret = true;
        }
        string str{data->data, data->size};
        return ret;
      }
      template <typename T> requires TransType<T>
      [[nodiscard]] bool writeData(ReadFileData<T>* const data) noexcept {
        bool ret{false};
        size_t free_buff_space{buff_size - active_buff->getUsedSize()};
        if (!data) {
          return ret;
        }
        if (data->size > active_buff->getTotalSize()) {
          return false;
        }
        ret = active_buff->setDat(data);
        free_buff_space -= data->size;
        if (!ret) {
          return ret;
        } else {  //  End of file check
          //  Stop uploading
          if (current_download_size == file_size) {
            while (!passive_buff->buff_not_busy.load()) {
              passive_buff->thr_copy_finish.wait_for(passive_buff->wait_thr_busy, milliseconds(1) ,[this]{return passive_buff->buff_not_busy.load();});
            }
            reStartThr();
            return true;
          }
        }
        //  Check if is a time to change buffer because current one is full already
        if (data->size > free_buff_space) {
          //  Check if buffer still busy, waiting for finish if it is
          while (!passive_buff->buff_not_busy.load()) {
            passive_buff->thr_copy_finish.wait_for(passive_buff->wait_thr_busy, milliseconds(1) ,[this]{return passive_buff->buff_not_busy.load();});
          }
          reStartThr();
          passive_buff->buff_not_busy = false;
          ret = true;
        }
        return ret;
      }
  };
  
  //  Buffer manager class - creating buffers (IOBuff) for everyone worker and assign each of them to by request
  class BuffMan {
    public :
      BuffMan(const size_t buff_quantity, const size_t buff_size) : buff_quantity{buff_quantity} {
        buff_set = make_unique<vector<shared_ptr<IOBuff>>>();
        workers_set = make_unique<map<thread::id, shared_ptr<IOBuff>>>();
        for (size_t count = 0; count < buff_quantity; ++count) {
          buff_set->emplace_back(make_shared<IOBuff>(buff_size));
        }
      }
      BuffMan(const size_t buff_quantity, const size_t buff_blk_size, const size_t buff_blk_number) 
      : BuffMan{buff_quantity, buff_blk_size * buff_blk_number}{}
      ~BuffMan() {
        workers_set->clear();
        buff_set->clear();
      }
      
      BuffMan(const BuffMan&) = delete;
      BuffMan(const BuffMan&&) = delete;
      BuffMan& operator = (const BuffMan&) = delete;
      BuffMan& operator = (const BuffMan&&) = delete;
      
      [[nodiscard]] variant<shared_ptr<IOBuff>, bool> getBuffer(const thread::id id) noexcept {
        variant<shared_ptr<IOBuff>, bool> ret{false};
        lock_guard<mutex> lck(assign_lock);
        if (workers_set->size() >= buff_quantity) {
          return ret;
        }
        if (workers_set->find(id) != workers_set->end()) {
          return ret;
        };
        for (auto vec : *buff_set) {
          if (ranges::find_if(*workers_set, [vec](auto work_id){ if (work_id.second == vec) return true; return false;}) == workers_set->end()){
            workers_set->emplace(make_pair(id, vec));
            ret = vec;
            break;
          }
        }
        return ret;
      }
    private :
      const size_t buff_quantity;
      unique_ptr<vector<shared_ptr<IOBuff>>> buff_set;
      unique_ptr<map<thread::id, shared_ptr<IOBuff>>> workers_set;
      mutex assign_lock;
  };
}

namespace TFTPSrvLib {
  //  TFTP server - main server process
  class TFTPSrv final : public SrvNet, public ShareResPool<ThrWorker*, deque>, public ShareResPool<TransferState*, vector> {
  public:
    TFTPSrv(const string_view path)
      : SrvNet{},
       ShareResPool<ThrWorker*, deque>(std::thread::hardware_concurrency()),
       ShareResPool<TransferState*, vector> (std::thread::hardware_concurrency()),
       base_dir{ path } {}
    TFTPSrv(const string_view path, const size_t core_mult)
      : SrvNet{}, 
      ShareResPool<ThrWorker*, deque>(std::thread::hardware_concurrency()* core_mult), 
      ShareResPool<TransferState*, vector> (std::thread::hardware_concurrency()* core_mult),
      base_dir{ path } {max_threads = std::thread::hardware_concurrency() * core_mult;}
    TFTPSrv(const string_view path, const size_t core_mult, const size_t port_number)
      : SrvNet{ port_number }, 
      ShareResPool<ThrWorker*, deque>(std::thread::hardware_concurrency()* core_mult),
      ShareResPool<TransferState*, vector> (std::thread::hardware_concurrency()* core_mult),
      base_dir{ path } {max_threads = std::thread::hardware_concurrency() * core_mult;}
    TFTPSrv(const string_view path, std::shared_ptr<Log> log) : TFTPSrv(path) { this->log = log; }
    TFTPSrv(const string_view path, const size_t core_mult, std::shared_ptr<Log> log) : TFTPSrv(path, core_mult) { this->log = log; }
    TFTPSrv(const string_view path, const size_t core_mult, const size_t port_number, std::shared_ptr<Log> log)
      : TFTPSrv(path, core_mult, port_number) {
      this->log = log;
    }

    TFTPSrv(const TFTPSrv&) = delete;
    TFTPSrv(const TFTPSrv&&) = delete;
    TFTPSrv& operator = (const TFTPSrv&) = delete;
    TFTPSrv& operator = (const TFTPSrv&&) = delete;
    
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

        //  Loop through all the results, make a socket and send termination message
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
    //  Terminate all active transfers and stop server
    bool srvTerminate(const size_t iteration_number = 60, const size_t port_number = DEFAULT_PORT) {
      bool ret{ true };
      stop_worker = true;
      term_worker = true;
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

        //  Loop through all the results, make a socket and send terminating message
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
    //  Terminate current transfer (Transfer only, not worker)
    bool transferTerminate(std::thread::id id) {
      bool ret{true}; 
      if (ranges::find_if(active_workers, [id](const auto &thr){if(thr.get_id() == id) return true;}) == active_workers.end()) {
        return ret;
      }
      auto work_stop = [id](const auto proc_set){
        if (auto thr_id{std::get<3>(*proc_set)}; thr_id == id) {
          *(std::get<2>(*proc_set)) = true;
          return true;
        } else {
          return false;
        }
      };
      if (auto res = ranges::find_if(*ShareResPool<ThrWorker*, deque>::thr_pool, work_stop); res != ShareResPool<ThrWorker*, deque>::thr_pool->end()) {
        ret = true;
      } else {
        ret = false;
      }
      return ret;
    }
    //  Local dir path + requested
    bool transferTerminate(std::filesystem::path path) {
      bool ret;
      auto work_stop = [path](const auto proc_set){
        auto file_mode = std::get<1>(*proc_set);
        auto current_path = std::get<0>(*file_mode);
        if (current_path == path) {
          return true;
        } else {
          return false;
        }
      };
      if (auto res = ranges::find_if(*ShareResPool<ThrWorker*, deque>::thr_pool, work_stop); res != ShareResPool<ThrWorker*, deque>::thr_pool->end()) {
        ret = true;
      } else {
        ret  =false;
      }
      return ret;
    }
    //  Get current server status - total number of workers, running workers number, running workers file names
    [[nodiscard]] unique_ptr<SrvStat> srvStatus(void) noexcept {
      auto thread_lst {make_unique<vector<std::thread::id>>()};
      unique_ptr<vector<std::thread::id>> active_lst, idle_lst;
      unique_ptr<vector<fs::path>> file_lst;
      ranges::transform(active_workers, std::back_inserter(*thread_lst), [](const auto &thr){return thr.get_id();});

      for (const auto& work_count : *ShareResPool<ThrWorker*, deque>::thr_pool) {
        if (!active_lst) {
          active_lst = make_unique<vector<std::thread::id>>();
        }
        if (!file_lst) {
          file_lst = make_unique<vector<fs::path>>();
        }
        active_lst->emplace_back(std::get<3>(*work_count));
        auto fl_mode = *std::get<1>(*work_count);
        file_lst->emplace_back(std::get<0>(fl_mode));
      }
      if (active_lst) {
        if (active_lst->size() < active_workers.size()) {
          idle_lst = make_unique<vector<std::thread::id>>();
          auto find_idle = [&active_lst](const auto& thr) {
            auto thr_id = thr.get_id();
            if (ranges::find(*active_lst, thr_id) == active_lst->end()) {
              return thr_id;
            } 
          };
          ranges::transform(active_workers, std::back_inserter(*idle_lst), find_idle);
        }
      }
      auto timestamp = system_clock::now();
      return make_unique<SrvStat>(make_tuple(std::move(thread_lst), std::move(active_lst), std::move(idle_lst), std::move(file_lst), timestamp));
    }
    // //  Get information about selected worker
    [[nodiscard]] TransferState* procStat(std::thread::id id) noexcept{
      TransferState* ret{nullptr};
      auto thrCompare = [id] (const auto& thr) {
        if (thr.get_id() == id) {
          return true;
        } else {
          return false;
        }
      };
      auto findActiveID = [id, &ret] (const auto worker) {
        auto work_id = std::get<std::thread::id>(*worker);
        if (work_id == id) {
          *std::get<atomic<bool>*> (*worker)= true;
          ret = worker;
          return true;
        }
        return false;
      };
      if (auto res = ranges::find_if(active_workers, thrCompare); res == active_workers.end()) {
        return ret;
      }
      ranges::for_each(*ShareResPool<TransferState*, vector>::thr_pool, findActiveID);
      return ret;
    }

  private:
    size_t max_threads{ 8 };
    const std::filesystem::path base_dir;
    size_t file_size;
    atomic<bool> stop_worker{ false }, term_worker{ false }, stop_server{ false };
    vector<jthread> active_workers;
    unique_ptr<vector<pair<std::thread::id, fs::path>>> workers_pool; //  List of all 
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
            log->debugMsg(__PRETTY_FUNCTION__, "One thread could not be created");
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
    //  Transfer worker - clients IO session
    //  TODO: Add statistic update to base net class methods
    void worker(void) {
      bool ret;
      mutex mtx;
      condition_variable cv;
      atomic<bool> current_terminate{false}, upd_stat{false};
      unique_lock<std::mutex> lck(mtx);
      FileMode file_mode;
      auto thr_worker = make_unique<ThrWorker>(make_tuple(&cv, &file_mode, &current_terminate, std::this_thread::get_id(), &upd_stat)); 
      auto thr_state{make_unique<TransferState>()};
      //auto thr_state = make_unique<TransferState>(make_tuple(std::this_thread::get_id(), base_dir, system_clock::now(), 0, nullptr, nullptr));
      unique_ptr<NetSock> transfer{};
      std::ostringstream thr_convert;
      thr_convert << std::this_thread::get_id();
      const string thr_id{ thr_convert.str() };
      string request_params;
      OACKOption oack_opt;
      optional<MulticastOption> mult_opt;
      ReqParam oack_req;

      if (log) {
        log->debugMsg("Thread ID - " + thr_id, " Starting");
      }
      std::get<std::thread::id>(*thr_state) = std::this_thread::get_id();
      ShareResPool<TransferState*, vector>::setRes(thr_state.get());
      cv.wait(lck);

      while (!stop_worker.load() || !term_worker.load() || !current_terminate.load()) {
        transfer.reset(new NetSock{ &file_mode, &term_worker, &current_terminate, log });
        //  TODO: Add non negotiation scenario!!!
        //  Send request confirmation
        if (std::get<7>(file_mode)) {
          mult_opt = make_tuple(std::get<7>(file_mode).value(), std::get<8>(file_mode).value(), true);
        }
        if (std::get<6>(file_mode)) {
          oack_req = make_pair(OptExtent::tsize, std::get<6>(file_mode).value());
          std::get<0>(oack_opt) = oack_req;
        }
        if (std::get<4>(file_mode)) {
          oack_req = make_pair(OptExtent::blksize, std::get<4>(file_mode).value());
          std::get<1>(oack_opt) = oack_req;
        }
        if (std::get<5>(file_mode)) {
          oack_req = make_pair(OptExtent::timeout, std::get<5>(file_mode).value());
          std::get<2>(oack_opt) = oack_req;
        }
        std::get<3>(oack_opt) = mult_opt;
        ret = transfer->sendOACK(&oack_opt);
        //  Check response and log
        if (!ret) {
          if (log) {
            log->infoMsg("Thread ID" + thr_id, "Can't send parameters confirmation (OACK) message");
          }
          continue;
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
          if (auto port{std::get<3>(file_mode)}; port) {
            request_params += "Port - " + std::to_string(port.value()) + "; ";
          }
          if (auto buff_size{std::get<4>(file_mode)}; buff_size) {
            request_params += "Buffer size - " + std::to_string(buff_size.value()) + "; ";
          }
          if (auto timeout{std::get<5>(file_mode)}; timeout) {
            request_params += "Timeout - " + std::to_string(timeout.value()) + "; ";
          }
          if (auto fs_size{std::get<6>(file_mode)}; fs_size) {
            request_params += "File size - " + std::to_string(fs_size.value()) + "; ";
          }
          if (auto mult_addr{std::get<7>(file_mode)};  mult_addr) {
            request_params += "File size - " +  mult_addr.value() + "; ";
          }
          if (auto mult_port{std::get<8>(file_mode)}; mult_port) {
            request_params += "File size - " + std::to_string(mult_port.value()) + "; ";
          }

          log->debugMsg("Thread ID" + thr_id, " Started request with params - " + request_params);
        }
        
        //  Creating transfer statistics
        std::get<fs::path>(*thr_state) = std::get<fs::path>(file_mode);
        std::get<2>(*thr_state) = system_clock::now();
        std::get<size_t*>(*thr_state) = &transfer->transfer_size;
        std::get<time_point<system_clock>*>(*thr_state) = &transfer->timestamp;

        if (auto fs_size{std::get<6>(file_mode)}; fs_size) {
          std::get<3>(*thr_state) = fs_size.value();
        } else {
          std::get<3>(*thr_state) = fs::file_size(std::get<fs::path>(file_mode));
        }

    
        //  Start transfer
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
          log->debugMsg("Thread ID" + thr_id, "File transfer " + std::get<0>(file_mode).string() + "finished");
        }

        //  Reset optional parameters
        resetFileModeTup(file_mode, index_sequence<3,4,5,6,7,8>{});
        
        auto rem = [](auto vec){if (std::get<std::thread::id>(*vec) == std::this_thread::get_id()) return true;};
        ranges::remove_if(*ShareResPool<TransferState*, vector>::thr_pool, rem);
        ShareResPool<ThrWorker*, deque>::setRes(thr_worker.get());
        cv.wait(lck);
      }
      std::lock_guard<mutex> stop_mtx(stop_worker_mtx);
      ranges::remove_if(active_workers.begin(), active_workers.end(), [](auto& thr) {if (thr.get_id() == std::this_thread::get_id()) return true; return false;});
      if (log) {
        log->debugMsg("Thread ID" + thr_id, "Finished");
      }
    }
    //  Client connections manager
    void sessionMgr(void) noexcept {
      bool valread;
      ReadPacket data;
      fs::path requested_file;
      TFTPOpeCode request_code;
      optional<size_t> fl_size;
      
      ThrWorker current_worker;
      FileMode* worker_set;

      if (log) {
        log->infoMsg("Main thread", "Session manager started");
      }

      while (!stop_worker.load() && !term_worker.load()) {
        data.clear();
        if (fl_size) {
          fl_size.reset();
        }
        valread = waitData(&data);
        if (!valread) {
          continue;
        }
        //  Request analysis, and making data format, convenient for working
        valread = data.makeFrameStruct();
        if (!valread) {
          ConstErrorPacket<BAD_FRAME_FORMAT_ERR_SIZE> error(TFTPError::Options_are_not_supported, (char*)&BAD_FRAME_FORMAT_ERR);
          sendto(sock_id, &error.packet, error.size, MSG_CONFIRM, (const struct sockaddr*)&cliaddr, cli_addr_size);
          continue;
        }
        valread = data.getParams(cliaddr, 0);
        if (!valread) {
          ConstErrorPacket<BAD_FRAME_FORMAT_ERR_SIZE> error(TFTPError::Options_are_not_supported, (char*)&BAD_FRAME_FORMAT_ERR);
          sendto(sock_id, &error.packet, error.size, MSG_CONFIRM, (const struct sockaddr*)&cliaddr, cli_addr_size);
          continue;
        }

        request_code = std::get<0>(data.packet_frame_structure);

        if(request_code != TFTPOpeCode::TFTP_OPCODE_READ || request_code != TFTPOpeCode::TFTP_OPCODE_WRITE) {
          ConstErrorPacket<WRONG_REQUEST_ERR_SIZE> error(TFTPError::Options_are_not_supported, (char*)&WRONG_REQUEST_ERR);
          sendto(sock_id, &error.packet, error.size, MSG_CONFIRM, (const struct sockaddr*)&cliaddr, cli_addr_size);
          continue;
        }
        
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
          fl_size = fs::file_size(requested_file);
          r_file.close();
          std::get<6>(data.trans_params) = fl_size.value();
        }
        
        //  Check and correct (if necessary) if requested options are supported
        valread = checkParam(&data.trans_params, fl_size);
        if (!valread) {
          ConstErrorPacket<BAD_FRAME_FORMAT_ERR_SIZE> error(TFTPError::Options_are_not_supported, (char*)&BAD_FRAME_FORMAT_ERR);
          sendto(sock_id, &error.packet, error.size, MSG_CONFIRM, (const struct sockaddr*)&cliaddr, cli_addr_size);
          continue;
        }

        auto work{ ShareResPool<ThrWorker*, deque>::getRes() };
        if (!work) {
          ConstErrorPacket<BUSY_ERR_SIZE> error(TFTPError::Options_are_not_supported, (char*)&BUSY_ERR);
          sendto(sock_id, &error.packet, error.size, MSG_CONFIRM, (const struct sockaddr*)&cliaddr, cli_addr_size);
          continue;
        }
        worker_set = std::get<FileMode*>(*work);
        std::get<0>(*worker_set) = std::get<0>(data.trans_params);
        std::get<1>(*worker_set) = std::get<1>(data.trans_params);
        std::get<2>(*worker_set) = std::get<2>(data.trans_params);
        std::get<3>(*worker_set) = std::get<3>(data.trans_params).value();
        std::get<4>(*worker_set) = std::get<4>(data.trans_params).value();
        std::get<5>(*worker_set) = std::get<5>(data.trans_params).value();
        std::get<6>(*worker_set) = std::get<6>(data.trans_params).value();
        std::get<7>(*worker_set) = std::get<7>(data.trans_params).value();
        std::get<8>(*worker_set) = std::get<8>(data.trans_params).value();
        std::get<9>(*worker_set) = std::get<9>(data.trans_params);
        //  Start new transfer in worker
        std::get<std::condition_variable*>(current_worker)->notify_one();
      }
      stop_server = false;
      if (log) {
        log->infoMsg("Main thread", "Session manager finished");
      }
    }

    //  Reset FileMode tuple elements
    template <typename T> void resetTuple(T& x) {
      x.reset();
    }
    template <typename TupleT, std::size_t... Is>
    void resetFileModeTup(TupleT& tp, std::index_sequence<Is...>) {
      (resetTuple(std::get<Is>(tp)), ...);
    }

  };
}


#endif // LIBTFTP_HPP
