#include "../../src/libTFTP.hpp"


using namespace TFTPClnLib;

/*
Command line parameters :
-p port number
-a server IP address
-u upload file name
-d download file name
-l path to local dir
-f local file name
-b packet size
-t timeout
-m transfer mode (a - ASCII || o - OCTET)
-q to exit application
-? parameters list
*/

constexpr std::string_view hlp {"Possible values for command line : \n -p port number,\
                                \n -a server IP address,\
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
  int port_id {5001};
  std::string ip_addr;
  std::filesystem::path local_dir {std::filesystem::current_path()};
  std::string rem_file;
  std::string local_file;
  std::filesystem::path path;
  std::string input_line;
  std::optional<size_t> buff_size, timeout;
  std::optional<bool> download;
  bool transfer_mode;
  std::unique_ptr<TFTPCln> cln;
  std::variant<size_t, std::string_view> transmission_res;
  auto checkTransferMode = [] (char* mode) {
    bool ret;
    if (*mode == 'o' || *mode == 'O') {
      ret = true;
    } else {
      ret = false;
    }
    return ret;
  };

  //  Reading options from CLI
  if (argc > 1) { 
    int opt;
    char* pEnd;
    while ((opt = getopt(argc, argv, "p:a:l:u:d:f:b:t:m:q:?")) != -1) {
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
  cln = std::make_unique<TFTPCln> (buff_size, timeout);
  if (download.has_value()) {
    if (download.value()) {
      transmission_res = cln->downLoad(ip_addr, port_id, rem_file, path, buff_size, timeout, transfer_mode, false);
    } else {
      transmission_res = cln->upLoad(ip_addr, port_id, rem_file, path, buff_size, timeout, transfer_mode);
    }
    if (auto transmission_err {std::get_if<std::string_view>(&transmission_res)}; transmission_err) {
      std::cout<< "Transmission error - "<<*transmission_err;
    } else {
      std::cout<< "Transmission finished. Transferred -  " << std::get<size_t>(transmission_res);
    }
  }
  return 0;
}
