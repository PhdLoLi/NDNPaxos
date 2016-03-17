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

#include <unistd.h>
#include <string>
#include <stdlib.h>
#include <chrono>
#include "threadpool.hpp" 

using namespace boost::threadpool;

class Consumer {
 public:
  Consumer(ndn::Name prefix, int total)
    : prefix_(prefix), total_(total), 
      commit_counter_(0), thr_counter_(0), starts_(total), periods_(total) {

//    prefix_.append("node_" + std::to_string(to_node_id));
    face_ = ndn::make_shared<ndn::Face>();
//    pool_ = new pool(win_size);
    boost::thread listen(boost::bind(&Consumer::attach, this));
  }

  void attach() {
//    LOG_INFO("Consumer attached!");
    face_->processEvents();
//    face_->getIoService().run();
//    LOG_INFO("Consumer attach Finished?!");
  }

  void clock_start() {
    start_ = std::chrono::high_resolution_clock::now();
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

 private:
  
  void onData(const ndn::Interest& interest, const ndn::Data& data) {
//    const uint8_t* value = data.getContent().value();
//    size_t size = data.getContent().value_size();
//    std::string value_str(value, value + size);
    int req_num = std::stoi(interest.getName().get(-1).toUri());
//    printf("Consumer onData ACK Number: %d\n", req_num);

    auto finish = std::chrono::high_resolution_clock::now();
    periods_[req_num] = (std::chrono::duration_cast<std::chrono::nanoseconds>
                        (finish-starts_[req_num]).count());
  
    thr_mut_.lock();
    thr_counter_++;

    int stage = 10000;
    if (thr_counter_ % stage == 0) {
      auto finish = std::chrono::high_resolution_clock::now();
      uint64_t period = std::chrono::duration_cast<std::chrono::milliseconds>(finish-start_).count();
      int throughput = stage * 1000 / period;
      printf("onData -- counter:%d milliseconds:%llu throughput:%d", thr_counter_, period, throughput);
      printf("   periods[%d] = %d thr_counter = %d\n", req_num, periods_[req_num], thr_counter_);
      throughputs_.push_back(throughput);
      start_ = std::chrono::high_resolution_clock::now();
    }

//    printf("periods[%d] = %d thr_counter = %d\t", req_num, periods_[req_num], thr_counter_);
    thr_mut_.unlock();

    ndn::Name new_name(prefix_);
    counter_mut_.lock();
    if (commit_counter_ < total_) {
      starts_[commit_counter_] = std::chrono::high_resolution_clock::now();
      std::string value = std::to_string(commit_counter_);
//      std::cout << "Start commit +++++++++++ " << value << std::endl;
//      new_name.appendNumber(commit_counter_);
      new_name.append(std::to_string(commit_counter_));
      commit_counter_++;
      counter_mut_.unlock();
      consume(new_name);
    } else {
//      printf("commit_counter %d Finish!!!!!!!!!!!!!\n", commit_counter_);
      commit_counter_++;
      counter_mut_.unlock();
    }
  }
  
  void onTimeout(const ndn::Interest& interest) {
//    LOG_DEBUG_COM("Consumer Timeout %s", interest.getName().toUri().c_str());
  }

  ndn::shared_ptr<ndn::Face> face_;
  ndn::Name prefix_;

  int win_size_;
  int total_;

  uint64_t commit_counter_;
  uint64_t thr_counter_;

  boost::mutex counter_mut_;
  boost::mutex thr_mut_;
  
  std::vector<std::chrono::high_resolution_clock::time_point> starts_;
  std::vector<uint64_t> periods_;
  std::vector<uint64_t> throughputs_;
  
  std::chrono::high_resolution_clock::time_point start_;
};

int main(int argc, char** argv) {

  if (argc < 4) {
    std::cout << "Usage: to_node_range win_size total" << std::endl;
    return 0;
  }

  int to_node_range = std::stoi(argv[1]);
//  std::string to_node_name = "node_" + std::string(argv[1]);
  int win_size = std::stoi(argv[2]);
  int total = std::stoi(argv[3]);
  std::vector<ndn::Name> prefixs;
  

  std::vector<Consumer *> consumers;

  for (int i =  0; i < to_node_range; i++) {
    ndn::Name to_name("hehe");
    to_name.append("node_" + std::to_string(i));
    prefixs.push_back(to_name);
    consumers.push_back(new Consumer(to_name, total));
  }

  std::cout << "Start 2 seconds warming up ..." << std::endl;
  sleep(2);
  std::cout << "After 2 seconds warming up, start commiting ..." << std::endl;
//  consumer.start_consume();

  for (int i = 0; i < win_size; i++) {
    for (int j = 0; j < to_node_range; j++) {
      if (i == 0) {
        consumers[j]->clock_start();
      }
      consumers[j]->consume();
    }
  }
  std::cout << "Main thread sleeping ..." << std::endl;
  sleep(10000000);

  return 0;
}
