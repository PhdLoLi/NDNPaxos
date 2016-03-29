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
#include <fstream>

namespace ndnpaxos {

using namespace std;
  
class Master {
 public:
  Master(node_id_t my_id, int node_num, int win_size, int local) 
    : my_id_(my_id), node_num_(node_num), win_size_(win_size) {

//    std::string config_file = "/Users/lijing/NDNPaxos/config/localhost-" + to_string(node_num_) + ".yaml";

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

    captain_ = new Captain(*view_, win_size_);
    commo_ = new Commo(captain_, *view_, 0);
    captain_->set_commo(commo_);

  }

  ~Master() {
  }

  void start() {
    commo_->start();
  }

  std::string my_name_;
  node_id_t my_id_;
  node_id_t node_num_;
  int win_size_;
  
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
 

  if (argc < 5) {
    std::cerr << "Usage: Node_ID Node_Num Window_Size Local_orNot(0/1)" << std::endl;
    return 0;
  }

  node_id_t my_id = stoul(argv[1]); 
  int node_num = stoi(argv[2]);
  int win_size = stoi(argv[3]);
  int local = stoi(argv[4]);
  
  Master master(my_id, node_num, win_size, local);
  master.start();

  return 0;
}

} // namespace ndnpaxos

int main(int argc, char** argv) {
  return ndnpaxos::main(argc, argv);
}
