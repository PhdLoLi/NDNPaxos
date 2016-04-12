/**
 * Created on Dec 14, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#include "view.hpp"
#include "commo.hpp"
#include "captain.hpp"

//#include <boost/thread/mutex.hpp>
//#include <boost/bind.hpp>
#include <fstream>
//#include <boost/filesystem.hpp>
#include <chrono>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include <boost/bind.hpp>
#include <fstream>
//#include <thread>

namespace ndnpaxos {

using namespace std;
  
class Master {
 public:
  Master(node_id_t my_id, int node_num, int total, int win_size, int local) 
    : my_id_(my_id), node_num_(node_num), 
      total_(total), win_size_(win_size),
      commit_counter_(0), thr_counter_(0), 
      recording_(false), done_(false), local_(local) {

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

    // init callback
    callback_latency_t call_latency = boost::bind(&Master::count_latency, this, _1, _2, _3);
//    callback_full_t callback_full = bind(&Master::count_exe_latency, this, _1, _2, _3);
    captain_ = new Captain(*view_, win_size_);

    captain_->set_callback(call_latency);
//    captain_->set_callback(callback);
//    captain_->set_callback(callback_full);

    commo_ = new Commo(captain_, *view_, 0);
    captain_->set_commo(commo_);
    pool_ = new pool(win_size);

  }

  ~Master() {
  }

  void start() {
    commo_->start();
  }
  void start_commit() {
    

    for (int i = 0; i < win_size_; i++) {
      counter_mut_.lock();
      commit_counter_++;
      counter_mut_.unlock();
      std::string value = "Commiting Value Time_" + std::to_string(commit_counter_) + " from " + view_->hostname();
      LOG_INFO(" +++++++++++ ZERO Init Commit Value: %s +++++++++++", value.c_str());
      captain_->commit(value);
      LOG_INFO(" +++++++++++ ZERO FINISH Commit Value: %s +++++++++++", value.c_str());

//      LOG_INFO("COMMIT DONE***********************************************************************");
    }

 }


  void count_exe_latency(slot_id_t slot_id, PropValue& prop_value, node_id_t node_id) {
  
  }
  
  void count_latency(slot_id_t slot_id, PropValue& prop_value, int try_time) {
  
    if (prop_value.has_cmd_type()) {
      counter_mut_.lock();
      commit_counter_++;
      counter_mut_.unlock();
      LOG_INFO("count_latency triggered! but this is a command slot_id : %llu commit_counter_ : %llu ", slot_id, commit_counter_);
      return;
    }
//    LOG_INFO("count_latency triggered! slot_id : %llu", slot_id);

    thr_mut_.lock();
    if (recording_) {
//      LOG_INFO("periods_.size() : %d thr_counter_ : %d periods_[thr_counter_] :%llu \n", periods_.size(), thr_counter_, periods_[thr_counter_]);
      thr_counter_++;
    }
    thr_mut_.unlock();

    if (commit_counter_ < total_) {
      counter_mut_.lock();
      std::string value = "Commiting Value Time_" + std::to_string(commit_counter_) + " from " + my_name_;
  //    std::cout << "Start commit +++++++++++ " << value << std::endl;
      commit_counter_++;
      counter_mut_.unlock();
      captain_->commit(value);
    }

  }
  
  std::string my_name_;
  node_id_t my_id_;
  node_id_t node_num_;
  int total_;
  int win_size_;
  
  Captain *captain_;
  View *view_;
  Commo *commo_;
  pool *pool_;

  slot_id_t commit_counter_;
  slot_id_t thr_counter_;

  boost::mutex counter_mut_;
  boost::mutex thr_mut_;
  
  std::vector<uint64_t> periods_;
  std::vector<uint64_t> throughputs_;
  std::vector<int> trytimes_;
  std::vector<std::chrono::high_resolution_clock::time_point> starts_;
  
  bool recording_;
  bool done_;
  int local_;
};



static void sig_int(int num) {
  std::cout << "Control + C triggered! " << std::endl;
  exit(num);
}  

int main(int argc, char** argv) {
  signal(SIGINT, sig_int);
 

  if (argc < 6) {
    std::cerr << "Usage: Node_ID Node_Num Total_Num Window_Size Local_orNot(0/1)" << std::endl;
    return 0;
  }

  node_id_t my_id = stoul(argv[1]); 
  int node_num = stoi(argv[2]);
  int total = stoi(argv[3]);
  int win_size = stoi(argv[4]);
  int local = stoi(argv[5]);
  
  Master master(my_id, node_num, total, win_size, local);
  master.start_commit();
  master.start();

  return 0;
}



} // namespace ndnpaxos

int main(int argc, char** argv) {
  return ndnpaxos::main(argc, argv);
}
