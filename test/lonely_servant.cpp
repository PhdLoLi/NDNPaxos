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
  Servant(node_id_t my_id, int node_num, int win_size, int local) 
    : my_id_(my_id), node_num_(node_num), win_size_(win_size), 
      thr_counter_(0), recording_(false), local_(local) {

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
//    callback_latency_t call_latency = boost::bind(&Servant::count_latency, this, _1, _2, _3);
    callback_full_t callback_full = bind(&Servant::count_exe_latency, this, _1, _2, _3);
    captain_ = new Captain(*view_, 1);
    commo_ = new Commo(captain_, *view_, 1);
    captain_->set_commo(commo_);

//    captain_->set_callback(call_latency);
//    captain_->set_callback(callback);
    captain_->set_callback(callback_full);

//    my_pool_ = new pool(win_size);

  }

  ~Servant() {
  }

  void attach() {
    commo_->start();
  }

  void count_exe_latency(slot_id_t slot_id, PropValue& prop_value, node_id_t node_id) {
    thr_mut_.lock();
    if (recording_) {
      thr_counter_++;
    }
    thr_mut_.unlock();
  }
  
  void count_latency(slot_id_t slot_id, PropValue& prop_value, int try_time) {
//    LOG_INFO("count_latency triggered! slot_id : %llu", slot_id);
  }

  void recording() {
    int warming = 2;
    int interval = 3;
    for (int i = 0; i < warming + interval; i++) {
      LOG_INFO("Warming & Not Recording Counting %d", i + 1);
      sleep(1);
    }
    LOG_INFO("%d s passed start punching", warming + interval);

    // add for consume logs only needs for non-quorum
//    if (!view_->if_quorum()) {
//      LOG_INFO("Start Cosuming for Logs, becasue the node is not quorum");
//      commo_->consume_log(win_size_);
//    }

    int punch = 1;

    thr_mut_.lock();
    recording_ = true;
    thr_mut_.unlock();

    uint64_t before = 0;
    uint64_t throughput = 0;

    for (int j = 0; j < interval * 4; j++) {
      LOG_INFO("Time %d", j + 1);

      thr_mut_.lock();
      before = thr_counter_;
      thr_mut_.unlock();

      sleep(1);

      thr_mut_.lock();
      throughput = thr_counter_ - before; 

      LOG_INFO("PUNCH!  -- counter:%lu second:1 throughput:%lu", thr_counter_, throughput);

      thr_mut_.unlock();
//      throughputs_.push_back(throughput);
    }
    
    thr_mut_.lock();
    recording_ = false;
    thr_mut_.unlock();

    LOG_INFO("Last %d s period", interval);
    for (int i = interval ; i > 0; i--) {
      LOG_INFO("Stopping Recording Counting %d", i);
      sleep(1);
    }

    commo_->stop();

    LOG_INFO("Last Last %d s period", warming);
    for (int i = warming * 2 ; i > 0; i--) {
      LOG_INFO("Cooling Counting %d", i);
      sleep(1);
    }
    LOG_INFO("Over!!!");
  }

  std::string my_name_;
  node_id_t my_id_;
  node_id_t node_num_;
  int win_size_;

  Captain *captain_;
  View *view_;
  Commo *commo_;
  
  slot_id_t thr_counter_;
  boost::mutex thr_mut_;
  
//  std::vector<uint64_t> throughputs_;
  
  bool recording_;
  int local_;
};

static void sig_int(int num) {
  std::cout << "Control + C triggered! " << std::endl;
  exit(num);
}  

int main(int argc, char** argv) {
  signal(SIGINT, sig_int);
 

  if (argc < 5) {
    std::cerr << "Usage: Node_ID Node_Num Consume_Log_Win_Size LocalorNot" << std::endl;
    return 0;
  }

  node_id_t my_id = stoul(argv[1]); 
  int node_num = stoi(argv[2]);
  int win_size = stoi(argv[3]);
  int local = stoi(argv[4]);
  
  Servant servant(my_id, node_num, win_size, local);
  servant.recording();

  return 0;
}


} // namespace ndnpaxos

int main(int argc, char** argv) {
  return ndnpaxos::main(argc, argv);
}
