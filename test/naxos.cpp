/**
 * Created on Mar 29, 2016
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#include "view.hpp"
#include "commo.hpp"
#include "captain.hpp"

#include <fstream>
#include <chrono>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include <boost/bind.hpp>

namespace ndnpaxos {

using namespace std;
  
class NaxosD {
 public:
  NaxosD(node_id_t my_id, int node_num, int win_size, int local, int log_size) 
    : my_id_(my_id), node_num_(node_num), win_size_(win_size), log_size_(log_size) {

    std::string tag;
    if (local_ == 0)
      tag = "localhost-";
    else 
      tag = "nodes-";

    std::string config_file = "config/" + tag + to_string(node_num_) + ".yaml";

    // init view_ for one captain_
    view_ = new View(my_id_, config_file);
    view_->print_host_nodes();
    my_name_ = view_->hostname();

    // init callback
    captain_ = new Captain(*view_, win_size);
    commo_ = new Commo(captain_, *view_);
    captain_->set_commo(commo_);

  }

  ~NaxosD() {
  }

  void start() {
    if (!view_->if_quorum()) {
      LOG_INFO("Start Cosuming for Logs, becasue the node is not quorum");
      commo_->consume_log(log_size_);
    }
    commo_->start();
  }

  std::string my_name_;
  node_id_t my_id_;
  node_id_t node_num_;
  int win_size_;
  int log_size_;

  Captain *captain_;
  View *view_;
  Commo *commo_;
  int local_;
};

static void sig_int(int num) {
  std::cout << "Control + C triggered! " << std::endl;
  exit(num);
}  

int main(int argc, char** argv) {
  signal(SIGINT, sig_int);
 

  if (argc < 6) {
    std::cerr << "Usage: Node_ID Node_Num Win_Size LocalorNot Log_Win_Size" << std::endl;
    return 0;
  }

  node_id_t my_id = stoul(argv[1]); 
  int node_num = stoi(argv[2]);
  int win_size = stoi(argv[3]);
  int local = stoi(argv[4]);
  int log_size = stoi(argv[5]);
  
  NaxosD naxos(my_id, node_num, win_size, local, log_size);
  naxos.start();

  return 0;
}


} // namespace ndnpaxos

int main(int argc, char** argv) {
  return ndnpaxos::main(argc, argv);
}
