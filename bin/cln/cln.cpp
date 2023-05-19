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

  enum class ParmVal : uint8_t {PortID, ServerIP, UplFileName, DownFileName, LocalDirPath, LocalFileName, PackSize, TimeOut, TransMode, Quit, Hlp};
  
  const std::vector<std::string> in_str_key {"-p", "-a", "-u", "-d", "-l", "-f", "-b", "-t", "-m", "-q", "-?"};  
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
  auto parseInOpt = [&port_id, 
                     &ip_addr,
                     &local_dir,
                     &rem_file,
                     &local_file,
                     &path,
                     &buff_size,
                     &timeout,
                     &download,
                     &transfer_mode,
                     &checkTransferMode] (const std::pair<ParmVal, std::string>& opt_pair) {
    std::from_chars_result char_to_int;
    int res;
    char trans_mode;
    switch (opt_pair.first) {
      case ParmVal::PortID : char_to_int = std::from_chars(opt_pair.second.data(), opt_pair.second.data() + opt_pair.second.size(), port_id); 
        if (char_to_int.ec == std::errc::invalid_argument) {
          std::cout << "Wrong port ID.\n";
          exit(EXIT_FAILURE);
        } else if (char_to_int.ec == std::errc::result_out_of_range) {
          std::cout << "Port ID number is larger than an int.\n";
          exit(EXIT_FAILURE);
        }
        break;
        case ParmVal::ServerIP : ip_addr = opt_pair.second.data(); break;
        case ParmVal::LocalDirPath : local_dir = opt_pair.second.data(); break;
        case ParmVal::UplFileName : rem_file = opt_pair.second.data(); download = false; break;
        case ParmVal::DownFileName : rem_file = opt_pair.second.data(); download = true; break;
        case ParmVal::LocalFileName : local_file = opt_pair.second.data(); break;
        case ParmVal::PackSize : char_to_int = std::from_chars(opt_pair.second.data(), opt_pair.second.data() + opt_pair.second.size(), res); 
          if (char_to_int.ec == std::errc::invalid_argument) {
            std::cout << "Packet size parameter isn't a number.\n";
            exit(EXIT_FAILURE);
          } else if (char_to_int.ec == std::errc::result_out_of_range) {
            std::cout << "Packet size is larger than an int.\n";
            exit(EXIT_FAILURE);
          }
          buff_size=res;
          break;
        case ParmVal::TimeOut : char_to_int = std::from_chars(opt_pair.second.data(), opt_pair.second.data() + opt_pair.second.size(), res); 
          if (char_to_int.ec == std::errc::invalid_argument) {
            std::cout << "Timeout size parameter isn't a number.\n";
            exit(EXIT_FAILURE);
          } else if (char_to_int.ec == std::errc::result_out_of_range) {
            std::cout << "Timeout size number is larger than an int.\n";
            exit(EXIT_FAILURE);
          }
          timeout = res;
          break;
        case ParmVal::TransMode : trans_mode = opt_pair.second.front();
          transfer_mode = checkTransferMode(&trans_mode); 
          break;
        case ParmVal::Quit : std::cout<< "Bye!"<< std::endl; exit(EXIT_SUCCESS);
        case ParmVal::Hlp : std::cout<< hlp<< std::endl; break;
        default : exit(EXIT_FAILURE);
    }
  };
  auto parseInStr = [&parseInOpt, &in_str_val, &rem_file, &local_file, &download] (std::string& in_str) {
    //  Clear previous session values
    std::vector<std::string> string_tokens, str_pair;
    std::string str_key, str_val;
    std::vector<std::pair<ParmVal, std::string>> val_vec;

    rem_file.clear();
    local_file.clear();
    download.reset();
    // Parsing key's
    
    std::transform(in_str.begin(), in_str.end(), in_str.begin(), [](unsigned char c){return std::tolower(c);});
    std::stringstream ss(in_str);
    std::string str;
    while (getline(ss, str, '-')) {
      string_tokens.push_back(str);
    }
    string_tokens.erase(std::remove(string_tokens.begin(), string_tokens.end(), ""), string_tokens.end());
    for (auto pair : string_tokens) {
      std::stringstream ss(pair);
      char key_str {str_key.front()};
      if (in_str_val.contains(key_str)) {
        auto key {in_str_val.at(key_str)};
        val_vec.emplace_back(std::make_pair(key, str_val));
      }
    }
    std::ranges::for_each(val_vec, parseInOpt);
  };
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
  while (true) {
    std::cin>> input_line;
    parseInStr(input_line);
    //  Starting transmission
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
  }
  return 0;
}
