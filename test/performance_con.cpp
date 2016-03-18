/**
 * Created on Mar,14 2016
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

class Consumer {
 public:
  Consumer(ndn::Name prefix)
    : prefix_(prefix), commit_counter_(0), thr_counter_(0), 
      starts_(500000), before_(0), recording_(false), done_(false) {

//    prefix_.append("node_" + std::to_string(to_node_id));
    face_ = ndn::make_shared<ndn::Face>();
//    pool_ = new pool(win_size);
    boost::thread listen(boost::bind(&Consumer::attach, this));
  }

  ~Consumer() {
    face_->shutdown();
  }

  void attach() {
    printf("Consumer attached!\n");
    face_->processEvents();
//    face_->getIoService().run();
    printf("Consumer attach Finished!\n");
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
      printf("PUNCH! to prefix:%s -- counter:%lu second:1 throughput:%lu latency:%lu ns\n", prefix_.toUri().c_str(), thr_counter_, throughput, periods_[periods_.size() - 1]);
    else 
      printf("PUNCH! -- counter:%lu second:1 throughput:%lu periods_.size() == 0\n", thr_counter_, throughput);

    thr_mut_.unlock();
    throughputs_.push_back(throughput);
  }

  void consume() {
    ndn::Name new_name(prefix_);
    counter_mut_.lock();
    std::cout << " +++++++++++ ZERO Init Commit Value:  +++++++++++ " << commit_counter_ << std::endl;
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
                           bind(&Consumer::onData, this,  _1, _2),
                           bind(&Consumer::onTimeout, this, _1));
//    std::cout << "Sending " << interest << std::endl;
//    LOG_INFO_COM("Consumer Sending %s Finish", interest.getName().toUri().c_str());
    // processEvents will block until the requested data received or timeout occurs
//    face_->processEvents();
  }

  std::vector<uint64_t> periods_;
  std::vector<uint64_t> throughputs_;

 private:
  
  void onData(const ndn::Interest& interest, const ndn::Data& data) {
//    const uint8_t* value = data.getContent().value();
//    size_t size = data.getContent().value_size();
//    std::string value_str(value, value + size);
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
      ndn::Name new_name(prefix_);
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
  }
  
  void onTimeout(const ndn::Interest& interest) {
//    LOG_DEBUG_COM("Consumer Timeout %s", interest.getName().toUri().c_str());
  }

  ndn::shared_ptr<ndn::Face> face_;
  ndn::Name prefix_;

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

  if (argc < 3) {
    std::cout << "Usage: to_node_range win_size" << std::endl;
    return 0;
  }

  int to_node_range = std::stoi(argv[1]);
//  std::string to_node_name = "node_" + std::string(argv[1]);
  int win_size = std::stoi(argv[2]);
  std::vector<ndn::Name> prefixs;
  

  std::vector<Consumer *> consumers;

  for (int i =  0; i < to_node_range; i++) {
    ndn::Name to_name("hehe");
    to_name.append("node_" + std::to_string(i));
    prefixs.push_back(to_name);
    consumers.push_back(new Consumer(to_name));
  }

//  int warming = 2;
//  std::cout << "Start " << warming << " seconds warming up ..." << std::endl;
//  sleep(warming);
//  std::cout << "After " << warming << " seconds warming up, start commiting ..." << std::endl;

  for (int i = 0; i < win_size; i++) {
    for (int j = 0; j < to_node_range; j++) {
      consumers[j]->consume();
    }
  }

  int interval = 3;

  for (int i = 0; i < interval; i++) {
    printf("Counting %d\n", i + 1);
    sleep(1);
  }
  printf("%d s passed start punching\n", interval);

  for (int i = 0; i < to_node_range; i++) {
    consumers[i]->punch();
  }

  for (int j = 0; j < interval * 4; j++) {
    printf("Time %d\n", j + 1);
    for (int i = 0; i < to_node_range; i++) {
      consumers[i]->before();
    }
    sleep(1);
    for (int i = 0; i < to_node_range; i++) {
      consumers[i]->after();
    }
  }

  for (int i = 0; i < to_node_range; i++) {
    consumers[i]->stop_punch();
  }

  printf("Last %d s period\n", interval);
  for (int i = interval; i > 0; i--) {
    printf("Counting %d\n", i);
    sleep(1);
  }

  std::ofstream file_throughput_;
  std::ofstream file_latency_;
  
  printf("Writing File Now!\n");

  std::string thr_name = "results/ndn/t_" + std::to_string(to_node_range) + "_" + std::to_string(win_size) + ".txt";
  file_throughput_.open(thr_name);

  std::string lat_name = "results/ndn/l_" + std::to_string(to_node_range) + "_" + std::to_string(win_size) + ".txt";
  file_latency_.open(lat_name);

  for (int j = 0; j < interval * 4; j++) {
    uint64_t thr = 0;
    for (int i = 0; i < to_node_range; i++) {
      thr += consumers[i]->throughputs_[j];
    }
    file_throughput_ << thr << "\n";
  }

  file_throughput_.close();

  for (int i = 0; i < to_node_range; i++) {
    for (int j = 0; j < consumers[i]->periods_.size(); j++) {
      file_latency_ << consumers[i]->periods_[j] << "\n";
    }
  }
  file_latency_.close();

  printf("Writing File Finished!\n");
  printf("Over!!!\n");

  return 0;
}
