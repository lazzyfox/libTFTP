#include <future>
#include "../../src/libTFTP.hpp"


using namespace TFTPClnLib;

/*
Command line parameters :
-p port number,
-a server address,
-u upload file name,
-d download file name,
-l path to local directory,
-f local file name,
-b packet sie,
-t timeout,
-m transfer mode (a - ASCII || o - OCTET),
-q to exit application,
-? help
*/



static constexpr std::string_view hlp {"Possible values for command line :\
                                \n -p port number,\
                                \n -a server address,\
                                \n -u upload file name,\
                                \n -d download file name,\
                                \n -l path to local directory,\
                                \n -f local file name,\
                                \n -b packet sie,\
                                \n -t timeout,\
                                \n -m transfer mode (a - ASCII || o - OCTET),\
                                \n -q to exit application,\
                                \n -? help"};



int main(int argc, char* argv[]) {
  int port_id {8099};
  std::string_view ip_addr {};
  std::filesystem::path local_dir {std::filesystem::current_path()};
  std::string rem_file {};
  std::string local_file {};

  std::filesystem::path path;
  std::string input_line;
  size_t buff_size {512}, timeout {6};
  std::optional<bool> download {};
  bool transfer_mode {false};
  std::unique_ptr<TFTPCln> cln;
  std::variant<size_t, std::string> transmission_res;

  enum class ParmVal : uint8_t {PortID, ServerIP, UplFileName, DownFileName, LocalDirPath, LocalFileName, PackSize, TimeOut, TransMode, Quit, Hlp};
  
  const std::unordered_map<char, ParmVal> in_str_val {{'p', ParmVal::PortID},
                                                      {'a', ParmVal::ServerIP},
                                                      {'u', ParmVal::UplFileName},
                                                      {'d', ParmVal::DownFileName},
                                                      {'l', ParmVal::LocalDirPath},
                                                      {'f', ParmVal::LocalFileName},
                                                      {'b', ParmVal::PackSize},
                                                      {'t', ParmVal::TimeOut},
                                                      {'m', ParmVal::TransMode},
                                                      {'q', ParmVal::Quit},
                                                      {'?', ParmVal::Hlp}};
  
  auto checkTransferMode = [] (char* mode) {
    bool ret;
    if (*mode == 'o' || *mode == 'O') {
      ret = true;
    } else {
      ret = false;
    }
    return ret;
  };
  auto hello_msg = std::async(std::launch::deferred,[&]() {std::cout << TFTPClnLib::lib_hello<<TFTPClnLib::lib_ver<<std::endl<<std::flush;});
  if (argc > 1) { 
    int opt;
    char* pEnd;
    while ((opt = getopt(argc, argv, "p:a:u:d:l:f:b:t:m:q:?")) != -1) {
      switch (opt) {
        case 'p' : port_id = strtol(optarg, &pEnd, 10); break;
        case 'a' : ip_addr = optarg; break;
        case 'l' : local_dir = optarg; break;
        case 'u' : rem_file = optarg; download = false; break;
        case 'd' : rem_file = optarg; download = true; break;
        case 'f' : local_file = optarg; break;
        case 'b' : buff_size = std::atoll(optarg); break;
        case 't' : timeout = std::atoll(optarg); break;
        case 'm' : transfer_mode = checkTransferMode(optarg); break;
        case 'q' : std::cout<< "Bye!"<< std::endl; exit(EXIT_SUCCESS);
        case '?' : std::cout<< hlp<< std::endl; break;
        default : exit(EXIT_FAILURE);
      }
    }
  }
  //  Starting transfer

  path = local_dir /= local_file;
  hello_msg.get();
  cln = std::make_unique<TFTPCln> (ip_addr, port_id, buff_size, timeout);
  if (download.has_value()) {
    if (download.value()) {
      transmission_res = cln->downLoad(rem_file, path, buff_size, timeout, transfer_mode, false);
    } else {
      transmission_res = cln->upLoad(rem_file, path, buff_size, timeout, transfer_mode);
    }
    if (auto transmission_err {std::get_if<std::string>(&transmission_res)}; transmission_err) {
      std::cout<< "Transmission error - "<<*transmission_err<<std::flush;
    } else {
      std::cout<< "Transmission finished. Transferred -  " << std::get<size_t>(transmission_res)<<std::flush;
    }
  }
  return 0;
}
