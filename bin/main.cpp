#include "../src/libTFTP.hpp"


using namespace TFTPSrvLib;

int main(int argc, char** argv)
{
 if (argc > 1) { //  Reading options from CLI
  int opt;
  while ((opt = getopt(argc, argv, "c:d")) != -1) {
    switch (opt) {
      // case 'c' : conf_path = optarg; break;
      // case 'd' : db_create = true; break;
      // case '?' :
      //   std::cout << hlp_start_key << std::endl << std::flush;
      //   exit(EXIT_FAILURE);
      //   break;
      // default : std::cout << hlp_start_key << std::endl << std::flush; exit(EXIT_FAILURE);
      }
    }
  }
  return 0;
}
