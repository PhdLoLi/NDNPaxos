/**
 * Created on Dec 25, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#include "dbmanager.hpp"
#include <iostream>

namespace ndnpaxos {
using namespace std;

int main(int argc, char** argv) {

  std::string config_file;
  if (argc < 3) {
    std::cerr << "Usage: Node_ID Node_Num" << std::endl;
    return 0;
  }

  node_id_t my_id = stoul(argv[1]); 
  int node_num = stoi(argv[2]);
  int win_size = 1;
  if (argc == 4) {
    win_size = stoi(argv[3]);
  }

  config_file = "config/localhost-" + to_string(node_num) + ".yaml";

  // init view for one captain
  DBManager db_m(my_id, node_num);
  
  db_m.start();

  LOG_INFO("I'm sleeping for 10000");
  sleep(10000);
  LOG_INFO("Master ALL DONE!");

  return 0;
}

} // namespace ndnpaxos

int main(int argc, char** argv) {
  return ndnpaxos::main(argc, argv);
}
