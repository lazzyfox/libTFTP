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

constexpr std::string_view hlp {"Possible values for command line : \n -p port number,\n -v IP version (4 or 6),\n -a server IP address to bind a service for,\n -m core multiplication number,\n -d server working directory,\n -l path to log file"};


int main(int argc, char *argv[]) {
  int port_id {5001};
  int ip_ver {AF_INET};
  std::string_view ip_addr {"192.168.1.3"};
  size_t thr_mult {1};
  auto log_file = std::make_shared<TFTPTools::Log>("/home/fox/tmp/tftp_dir/srv/tftp_log.txt", true, true, true);
  std::filesystem::path work_dir{"/home/fox/tmp/tftp_dir/srv"};

  auto ver_check = [](char* ver){
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
 
  if (argc > 1) { //  Reading options from CLI
    int opt;
    char * pEnd;
    while ((opt = getopt(argc, argv, "p:v:a:d:l:?")) != -1) {
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
  TFTPSrv srv{std::move(work_dir), std::move(ip_ver), std::move(ip_addr), std::move(port_id), std::move(thr_mult), log_file};
  auto stat = srv.srvStart();
  // TFTPSrv srv{"/home/fox/tmp/tftp_dir/srv", 1, SERVICE_PORT, log_file};
  // auto stat = srv.srvStart();
  return 0;
}
