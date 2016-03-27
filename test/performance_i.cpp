/**
 * Created on Mar,27 2016
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>

#include <fstream>
#include <unistd.h>
#include <string>
#include <stdlib.h>
#include <chrono>
#include "threadpool.hpp" 

using namespace boost::threadpool;

class CP {
 public:
  CP(ndn::Name my_prefix, ndn::Name to_prefix)
    : my_prefix_(my_prefix), to_prefix_(to_prefix),
      commit_counter_(0), thr_counter_(0), 
      starts_(500000), before_(0), 
      recording_(false), done_(false) {

//    prefix_.append("node_" + std::to_string(to_node_id));
    face_ = ndn::make_shared<ndn::Face>();
    face_->setInterestFilter(my_prefix,
               bind(&CP::onInterest, this, _1, _2),
               bind(&CP::onRegisterSucceed, this, _1),
               bind(&CP::onRegisterFailed, this, _1, _2));
//    pool_ = new pool(win_size);
    boost::thread listen(boost::bind(&CP::attach, this));
  }

  ~CP() {
    face_->shutdown();
  }

  void attach() {
    printf("CP attached!\n");
    face_->processEvents();
//    face_->getIoService().run();
    printf("CP attach Finished!\n");
  }

  void punch() {
    thr_mut_.lock();
    recording_ = ~recording_;
    thr_mut_.unlock();
  }

  void stop_punch() {
    thr_mut_.lock();
    recording_ = false;
    done_ = true;
    thr_mut_.unlock();
  }

  void before() {
    thr_mut_.lock();
    before_ = thr_counter_;
    thr_mut_.unlock();
  } 

  void after() {
    thr_mut_.lock();
    uint64_t throughput = thr_counter_ - before_; 
    if (periods_.size() > 0)
      printf("PUNCH! to prefix:%s -- counter:%lu second:1 throughput:%lu latency:%lu ns\n", to_prefix_.toUri().c_str(), thr_counter_, throughput, periods_[periods_.size() - 1]);
    else 
      printf("PUNCH! -- counter:%lu second:1 throughput:%lu periods_.size() == 0\n", thr_counter_, throughput);

    thr_mut_.unlock();
    throughputs_.push_back(throughput);
  }

  void consume() {
    ndn::Name new_name(to_prefix_);
    counter_mut_.lock();
    std::cout << " +++++++++++ ZERO Init Commit Value:  +++++++++++ Number" << commit_counter_ << std::endl;
    new_name.append(std::to_string(commit_counter_));
    starts_[commit_counter_] = std::chrono::high_resolution_clock::now(); 
    commit_counter_++;
    counter_mut_.unlock();
    consume(new_name);
  }

  void consume(ndn::Name name) {
    ndn::Interest interest(name);
    interest.setInterestLifetime(ndn::time::milliseconds(1000));
    interest.setMustBeFresh(true);
    face_->expressInterest(interest,
                           bind(&CP::onData, this,  _1, _2),
                           bind(&CP::onTimeout, this, _1));
//    std::cout << "Sending " << interest << std::endl;
  }

  std::vector<uint64_t> periods_;
  std::vector<uint64_t> throughputs_;

 private:

   void onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest) {
//    std::cout << "Producer I: " << interest.getName() << std::endl;

    // Create new name, based on Interest's name
    thr_mut_.lock();
    if (recording_) {
      int req_num = std::stoi(interest.getName().get(-1).toUri());
      auto finish = std::chrono::high_resolution_clock::now();
      periods_.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>
                        (finish-starts_[req_num]).count());
//      printf("periods_.size() : %d thr_counter_ : %d periods_[thr_counter_] :%llu \n", periods_.size(), thr_counter_, periods_[thr_counter_]);
      thr_counter_++;
    }
    thr_mut_.unlock();

    if (done_ == false) {
      ndn::Name new_name(to_prefix_);
      counter_mut_.lock();
      starts_[commit_counter_] = std::chrono::high_resolution_clock::now();
      std::string value = std::to_string(commit_counter_);
  //    std::cout << "Start commit +++++++++++ " << value << std::endl;
  //    new_name.appendNumber(commit_counter_);
      new_name.append(std::to_string(commit_counter_));
      commit_counter_++;
      counter_mut_.unlock();
      consume(new_name);
    }

//    sign_thread(data);
  }
 
  void onData(const ndn::Interest& interest, const ndn::Data& data) {
  }
  
  void onTimeout(const ndn::Interest& interest) {
//    LOG_DEBUG_COM("CP Timeout %s", interest.getName().toUri().c_str());
  }

  void onRegisterSucceed(const ndn::InterestFilter& filter) {
    printf("onRegisterSucceed! %s\n", filter.getPrefix().toUri().c_str());
  }

  void onRegisterFailed(const ndn::Name& prefix, const std::string& reason) {
    std::cerr << "ERROR: Failed to register prefix \""
              << prefix << "\" in local hub's daemon (" << reason << ")"
              << std::endl;
    face_->shutdown();
  }

  ndn::shared_ptr<ndn::Face> face_;
  ndn::Name my_prefix_;
  ndn::Name to_prefix_;

  uint64_t commit_counter_;
  uint64_t thr_counter_;

  boost::mutex counter_mut_;
  boost::mutex thr_mut_;
  
  std::vector<std::chrono::high_resolution_clock::time_point> starts_;
  
  std::chrono::high_resolution_clock::time_point start_;
  uint64_t before_;
  bool recording_;
  bool done_;

};

int main(int argc, char** argv) {

  if (argc < 4) {
    std::cout << "Usage: my_node_id to_node_id win_size" << std::endl;
    return 0;
  }

  int my_node_id = std::stoi(argv[1]);
  int to_node_id = std::stoi(argv[2]);
  int win_size = std::stoi(argv[3]);

  std::string prefix = "hehe";
  ndn::Name my_prefix = ndn::Name(prefix).append("node_" + std::to_string(my_node_id));
  ndn::Name to_prefix = ndn::Name(prefix).append("node_" + std::to_string(to_node_id));
  CP cp(my_prefix, to_prefix);

  int warming = 2;
  std::cout << "Start " << warming << " seconds warming up ..." << std::endl;
  sleep(warming);
  std::cout << "After " << warming << " seconds warming up, start commiting ..." << std::endl;

  for (int i = 0; i < win_size; i++) {
    cp.consume();
  }

  int interval = 3;

  cp.punch();

  for (int j = 0; j < interval * 4 * 10; j++) {
    printf("Time %d\n", j + 1);
    cp.before();
    sleep(1);
    cp.after();
  }

  cp.stop_punch();

  printf("Last %d s period\n", interval);
  for (int i = interval; i > 0; i--) {
    printf("Counting %d\n", i);
    sleep(1);
  }

  printf("Writing File Finished!\n");
  printf("Over!!!\n");

  return 0;
}
