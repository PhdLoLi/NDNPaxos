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
  Master(node_id_t my_id, int node_num, int value_size, int win_size, int local) 
    : my_id_(my_id), node_num_(node_num), 
      value_size_(value_size), win_size_(win_size),
      commit_counter_(0), thr_counter_(0), starts_(500000),
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

  void attach() {
    commo_->start();
  }

  void commit_thread(std::string &value) {
    captain_->commit(value);
  } 

  void start_commit() {
    
    int warming = 2;
    int interval = 3;
    for (int i = 0; i < warming; i++) {
      LOG_INFO("Warming Counting %d", i + 1);
      sleep(1);
    }

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

    for (int i = 0; i < interval; i++) {
      LOG_INFO("Not Recording Counting %d", i + 1);
      sleep(1);
    }
    LOG_INFO("%d s passed start punching", interval);

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

      if (periods_.size() > 0) {
        LOG_INFO("PUNCH!  -- counter:%lu second:1 throughput:%lu latency:%lu ns", thr_counter_, throughput, periods_[periods_.size() - 1]);
      }
      else {
        LOG_INFO("PUNCH! -- counter:%lu second:1 throughput:%lu periods_.size() == 0", thr_counter_, throughput);
      }

      thr_mut_.unlock();
      throughputs_.push_back(throughput);
    }
    
    thr_mut_.lock();
    recording_ = false;
    done_ = true;
    thr_mut_.unlock();

    LOG_INFO("Last %d s period", interval);
    for (int i = interval; i > 0; i--) {
      LOG_INFO("Stop Committing Counting %d", i);
      sleep(1);
    }

    commo_->stop();

    std::ofstream file_throughput_;
    std::ofstream file_latency_;
    std::ofstream file_trytime_;
    
    LOG_INFO("Writing File Now!");

    std::string thr_name = "results/ndnpaxos/t_" + std::to_string(node_num_) + "_" + std::to_string(win_size_) + ".txt";
    file_throughput_.open(thr_name);

    std::string lat_name = "results/ndnpaxos/l_" + std::to_string(node_num_) + "_" + std::to_string(win_size_) + ".txt";
    file_latency_.open(lat_name);

    std::string try_name = "results/ndnpaxos/t_" + std::to_string(node_num_) + "_" + std::to_string(win_size_) + ".txt";
    file_trytime_.open(try_name);

    for (int i = 0; i < throughputs_.size(); i++) {
      file_throughput_ << throughputs_[i] << "\n";
    }

    file_throughput_.close();

    for (int j = 0; j < periods_.size(); j++) {
      file_latency_ << periods_[j] << "\n";
    }
    file_latency_.close();

    for (int j = 0; j < trytimes_.size(); j++) {
      file_trytime_ << trytimes_[j] << "\n";
    }
    file_trytime_.close();

    LOG_INFO("Writing File Finished!");

    LOG_INFO("Last Last %d s period", warming);
    for (int i = warming ; i > 0; i--) {
      LOG_INFO("Cooling Counting %d", i);
      sleep(1);
    }

    LOG_INFO("Over!!!");
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
      value_id_t value_id = prop_value.id() >> 16;
      auto finish = std::chrono::high_resolution_clock::now();
      periods_.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>
                        (finish-starts_[value_id - 1]).count());
      trytimes_.push_back(try_time);
//      LOG_INFO("periods_.size() : %d thr_counter_ : %d periods_[thr_counter_] :%llu \n", periods_.size(), thr_counter_, periods_[thr_counter_]);
      thr_counter_++;
    }
    thr_mut_.unlock();

    if (done_ == false) {
      counter_mut_.lock();
      std::string value = "Commiting Value Time_" + std::to_string(commit_counter_) + " from " + my_name_;
      starts_[commit_counter_] = std::chrono::high_resolution_clock::now();
  //    std::cout << "Start commit +++++++++++ " << value << std::endl;
      commit_counter_++;
      counter_mut_.unlock();
      captain_->commit(value);
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
    std::cerr << "Usage: Node_ID Node_Num Value_Size Window_Size Local_orNot(0/1)" << std::endl;
    return 0;
  }

  node_id_t my_id = stoul(argv[1]); 
  int node_num = stoi(argv[2]);
  int value_size = stoi(argv[3]);
  int win_size = stoi(argv[4]);
  int local = stoi(argv[5]);
  
  Master master(my_id, node_num, value_size, win_size, local);
  master.start_commit();


  return 0;
}



} // namespace ndnpaxos

int main(int argc, char** argv) {
  return ndnpaxos::main(argc, argv);
}
