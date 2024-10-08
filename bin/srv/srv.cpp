#include <future>

#include "../../src/libTFTP.hpp"


using namespace TFTPSrvLib;

/*
Command line parameters :
-p port number
-v IP version (4 or 6)
-a IP address
-m Transmission threads multiplication number (max_threads = core_number*threads_multiplexer)
-d working directory
-l log file name
-? parameters list
*/

static constexpr std::string_view hlp {"Possible values for command line :\
                                 \n -p port number,\
                                 \n -v IP version (4 or 6),\
                                 \n -a server IP address to bind a service for,\
                                 \n -m core multiplication number (0 - default value -x : number of treads, positive number is multiplication coefficient),\
                                 \n -d server working directory,\
                                 \n -l path to log file"};


int main(int argc, char *argv[]) {
  size_t port_id {8099};
  int ip_ver {AF_INET};
  std::string_view ip_addr;
  int16_t thr_mult {-1};
  auto local_dir {std::filesystem::current_path()};
  std::filesystem::path work_dir{local_dir};
  auto log_file {std::make_shared<TFTPTools::Log>(std::filesystem::path(local_dir/="tftp_log.txt"), true, true, true)};
  
  //  Check IP version
  const auto&& ver_check = [](char* ver) {
    int ret;
    std::string ver_str {ver};
    auto dig_ver {stoi(ver_str)};
    if (dig_ver == 4) {
      ret = AF_INET;
    }
    if (dig_ver == 6) {
      ret = AF_INET6;
    }
    return ret;
  };
  //  Reading options from CLI
  if (argc > 1) { 
    int opt;
    char* pEnd;
    while ((opt = getopt(argc, argv, "p:v:a:d:l:m:?")) != -1) {
      switch (opt) {
        case 'p' : port_id = strtol(optarg, &pEnd, 10); break;
        case 'v' : ip_ver = ver_check(optarg); break;
        case 'a' : ip_addr = optarg; break;
        case 'm' : thr_mult = std::atoll(optarg); break;
        case 'd' : work_dir = optarg; break;
        case 'l' : log_file = std::make_shared<TFTPTools::Log>(optarg, true, true, true); break;
        case '?' : std::cout<< hlp<<std::endl; exit(EXIT_FAILURE);
        default :;
      }
    }
  }
  auto hello_msg = std::async(std::launch::deferred,[&]() {std::cout << TFTPShortNames::lib_hello<<TFTPShortNames::lib_ver<<std::endl<<std::flush;});
  
  //  Run server  
  TFTPSrv srv{std::move(work_dir), std::move(ip_ver), std::move(ip_addr), std::move(port_id), std::move(thr_mult), log_file};
  const auto stat = srv.srvStart();
  if (!stat) {
    return 1;
  }
  return 0;
}
