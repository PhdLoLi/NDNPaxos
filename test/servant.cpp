/**
 * Created on Dec 14, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#include "view.hpp"
//#include <boost/bind.hpp>

#include "commo.hpp"
#include "captain.hpp"

//#include "threadpool.hpp" 
//#include <boost/thread/mutex.hpp>
#include <fstream>
//#include <boost/filesystem.hpp>
#include <chrono>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
//#include <thread>
#include <boost/bind.hpp>

namespace ndnpaxos {

//using namespace std::placeholders;
using namespace std;
//using namespace boost::filesystem;
//using namespace boost::threadpool;


  
class Servant {
 public:
  Servant(node_id_t my_id, int node_num) 
    : my_id_(my_id), node_num_(node_num) {

    std::string config_file = "config/localhost-" + to_string(node_num_) + ".yaml";

    // init view_ for one captain_
    view_ = new View(my_id_, config_file);
    view_->print_host_nodes();
    
    my_name_ = view_->hostname();

    // init callback
    callback_latency_t call_latency = boost::bind(&Servant::count_latency, this, _1, _2, _3);
//    callback_full_t callback_full = bind(&Servant::count_exe_latency, this, _1, _2, _3);
    captain_ = new Captain(*view_, 1);
    commo_ = new Commo(captain_, *view_);
    captain_->set_commo(commo_);

    captain_->set_callback(call_latency);
//    captain_->set_callback(callback);
//    captain_->set_callback(callback_full);

//    my_pool_ = new pool(win_size);

  }

  ~Servant() {
  }

  void attach() {
    commo_->start();
  }

  void count_exe_latency(slot_id_t slot_id, PropValue& prop_value, node_id_t node_id) {
  
  }
  
  void count_latency(slot_id_t slot_id, PropValue& prop_value, int try_time) {
//    LOG_INFO("count_latency triggered! slot_id : %llu", slot_id);
  }

  std::string my_name_;
  node_id_t my_id_;
  node_id_t node_num_;

  Captain *captain_;
  View *view_;
  Commo *commo_;
  
};



static void sig_int(int num) {
  std::cout << "Control + C triggered! " << std::endl;
  exit(num);
}  

int main(int argc, char** argv) {
  signal(SIGINT, sig_int);
 

  if (argc < 3) {
    std::cerr << "Usage: Node_ID Node_Num" << std::endl;
    return 0;
  }

  node_id_t my_id = stoul(argv[1]); 
  int node_num = stoi(argv[2]);
  
  Servant servant(my_id, node_num);


  LOG_INFO("I'm sleeping for 10000");
  sleep(100000000);
  LOG_INFO("Servant ALL DONE!");

  return 0;
}


} // namespace ndnpaxos

int main(int argc, char** argv) {
  return ndnpaxos::main(argc, argv);
}
