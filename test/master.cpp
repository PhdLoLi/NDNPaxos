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
//#include <thread>

namespace ndnpaxos {

using namespace std;
  
class Master {
 public:
  Master(node_id_t my_id, int node_num, int value_size, int win_size, int total) 
    : my_id_(my_id), node_num_(node_num), 
      value_size_(value_size), win_size_(win_size), total_(total),
      commit_counter_(0), starts_(total) {

//    std::string config_file = "/Users/lijing/NDNPaxos/config/localhost-" + to_string(node_num_) + ".yaml";
    std::string config_file = "config/localhost-" + to_string(node_num_) + ".yaml";

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

  void attach() {
    commo_->start();
  }

  void commit_thread(std::string &value) {
    captain_->commit(value);
  } 

  void start_commit() {

    start_ = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < win_size_; i++) {
      counter_mut_.lock();
      commit_counter_++;
      starts_[commit_counter_] = std::chrono::high_resolution_clock::now(); 
      counter_mut_.unlock();
//      std::string value = "Commiting Value Time_" + std::to_string(i) + " from " + view_->hostname();
      std::string value = "Commiting Value Time_" + std::to_string(commit_counter_) + " from " + view_->hostname();
      LOG_INFO(" +++++++++++ ZERO Init Commit Value: %s +++++++++++", value.c_str());
      captain_->commit(value);
//      pool_->schedule(boost::bind(&Master::commit_thread, this, value));
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

    auto finish = std::chrono::high_resolution_clock::now();
    counter_mut_.lock();
    commit_counter_++;
    value_id_t value_id = prop_value.id() >> 16;
    periods_.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>
                     (finish-starts_[value_id % total_]).count());
    trytimes_.push_back(try_time);
  
//    LOG_INFO("periods[%d] = %d", periods_.size() - 1, periods_[periods_.size() - 1]);
//    LOG_INFO("trytimes[%d] = %d", trytimes_.size() - 1, trytimes_[trytimes_.size() - 1]);
  
    std::string value = "Commiting Value Time_" + std::to_string(commit_counter_) + " from " + my_name_;
    starts_[commit_counter_ % total_] = std::chrono::high_resolution_clock::now();
    slot_id_t counter_tmp = commit_counter_;
    counter_mut_.unlock();

    if (counter_tmp <= total_) {
  //    LOG_INFO("++++ I just Commit Value: %s ++++", value.c_str());
      if (counter_tmp % 10000 == 0) {
        auto finish = std::chrono::high_resolution_clock::now();
        uint64_t period = std::chrono::duration_cast<std::chrono::milliseconds>(finish-start_).count();
        start_ = std::chrono::high_resolution_clock::now();
        int throughput = 10000 * 1000 / period;
        LOG_INFO("Last_commit -- counter:%d milliseconds:%llu throughput:%d", counter_tmp, period, throughput);
        LOG_INFO("value_id = %d periods[%d] = %d", value_id, periods_.size() - 1, periods_[periods_.size() - 1]);
        throughputs_.push_back(throughput);
      }
//      std::cout << "master want to commit Value: " << value << std::endl;
//      boost::thread commit_first(bind(&Master::commit_thread, this, value));
//      LOG_INFO(" +++++++++++ Init Commit Value: %s +++++++++++", value.c_str());
      captain_->commit(value);
//      LOG_INFO(" +++++++++++ FINISH Commit Value: %s +++++++++++", value.c_str());
//      std::cout << "master want to commit Value Finish: " << value << std::endl;
    }

  }
  
  std::string my_name_;
  node_id_t my_id_;
  node_id_t node_num_;
  int value_size_;
  int win_size_;
  
  Captain *captain_;
  View *view_;
  Commo *commo_;
  pool *pool_;

  int total_;
  slot_id_t commit_counter_;

  boost::mutex counter_mut_;
  
  std::vector<uint64_t> periods_;
  std::vector<uint64_t> throughputs_;
  std::vector<int> trytimes_;
  std::vector<std::chrono::high_resolution_clock::time_point> starts_;
  
  std::chrono::high_resolution_clock::time_point start_;
};



static void sig_int(int num) {
  std::cout << "Control + C triggered! " << std::endl;
  exit(num);
}  

int main(int argc, char** argv) {
  signal(SIGINT, sig_int);
 

  if (argc < 6) {
    std::cerr << "Usage: Node_ID Node_Num Value_Size Window_Size Total_time" << std::endl;
    return 0;
  }

  node_id_t my_id = stoul(argv[1]); 
  int node_num = stoi(argv[2]);
  int value_size = stoi(argv[3]);
  int win_size = stoi(argv[4]);
  int total = stoi(argv[5]);
  
  Master master(my_id, node_num, value_size, win_size, total);
  sleep(2);
  LOG_INFO("Start Committing");
  master.start_commit();
//  master.attach();
//  master.commo_->start();

  LOG_INFO("I'm sleeping for 10000");
  sleep(100000000);
  LOG_INFO("Master ALL DONE!");

  return 0;
}



} // namespace ndnpaxos

int main(int argc, char** argv) {
  return ndnpaxos::main(argc, argv);
}
