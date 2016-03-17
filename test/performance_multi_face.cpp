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
#include "threadpool.hpp" 
#include <chrono>

using namespace boost::threadpool;

class Consumer {
 public:
  Consumer(std::string node_name, ndn::Name prefix, int win_size, int total)
    : node_name_(node_name), prefix_(prefix), win_size_(win_size), total_(total), 
      commit_counter_(0), thr_counter_(0),  starts_(total), periods_(total) {

    prefix_.append(node_name);

    for (int i = 0; i < win_size; i++) {
      faces_.push_back(ndn::make_shared<ndn::Face>());
//    pool_ = new pool(win_size);
      boost::thread listen(boost::bind(&Consumer::attach, this, i));
    }
  }

  void attach(int i) {
    printf("Consumer %d attached!\n", i);
//    face_->getIoService().run();
      faces_[i]->processEvents();
//      faces_[i]->getIoService().run();
    printf("Consumer %d attach Finished?!", i);
  }

  void start_consume() {

    start_ = std::chrono::high_resolution_clock::now();
    

    for (int i = 0; i < win_size_; i++) {
      
      ndn::Name new_name(prefix_);
      counter_mut_.lock();
      int counter = commit_counter_;
      std::string value =  std::to_string(commit_counter_) + " from " + node_name_;
      std::cout << " +++++++++++ ZERO Init Commit Value: %s +++++++++++ " << value << std::endl;
      new_name.appendNumber(commit_counter_);
      starts_[commit_counter_] = std::chrono::high_resolution_clock::now(); 
      commit_counter_++;
      counter_mut_.unlock();
      consume(new_name, counter % win_size_);
//      usleep(20000);
//      std::cout << " +++++++++++ ZERO Finish Commit Value: %s +++++++++++ " << value << std::endl;

    }
  }

  void consume(ndn::Name name, int counter) {
    ndn::Interest interest(name);
    interest.setInterestLifetime(ndn::time::milliseconds(1000));
    interest.setMustBeFresh(true);
    faces_[counter]->expressInterest(interest,
                           bind(&Consumer::onData, this,  _1, _2),
                           bind(&Consumer::onTimeout, this, _1));
//    LOG_INFO_COM("Consumer Sending %s Finish", interest.getName().toUri().c_str());
    // processEvents will block until the requested data received or timeout occurs
//    face_->processEvents();
  }

 private:
  
  void onData(const ndn::Interest& interest, const ndn::Data& data) {
//    const uint8_t* value = data.getContent().value();
//    size_t size = data.getContent().value_size();
//    std::string value_str(value, value + size);
    int req_num = interest.getName().get(-1).toNumber();
    printf("Consumer onData ACK Number: %d\n", req_num);

    auto finish = std::chrono::high_resolution_clock::now();
    periods_[req_num] = (std::chrono::duration_cast<std::chrono::nanoseconds>
                        (finish-starts_[req_num]).count());
  
    thr_mut_.lock();
    thr_counter_++;

    if (thr_counter_ % 10 == 0) {
      auto finish = std::chrono::high_resolution_clock::now();
      uint64_t period = std::chrono::duration_cast<std::chrono::milliseconds>(finish-start_).count();
      int throughput = 10 * 1000 / period;
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
      int counter = commit_counter_;
      std::string value = std::to_string(commit_counter_) + " from " + node_name_;
//      std::cout << "Start commit +++++++++++ " << value << std::endl;
      new_name.appendNumber(commit_counter_);
      commit_counter_++;
      counter_mut_.unlock();
      consume(new_name, counter % win_size_);
    } else {
//      printf("commit_counter %d Finish!!!!!!!!!!!!!\n", commit_counter_);
      commit_counter_++;
      counter_mut_.unlock();
    }
  }
  
  void onTimeout(const ndn::Interest& interest) {
//    LOG_DEBUG_COM("Consumer Timeout %s", interest.getName().toUri().c_str());
  }

  std::vector<ndn::shared_ptr<ndn::Face>> faces_;
  ndn::Name prefix_;
  ndn::KeyChain keyChain_;

  std::string node_name_;

  int win_size_;
  int total_;
  uint64_t commit_counter_;
  uint64_t thr_counter_;

  boost::mutex counter_mut_;
  boost::mutex thr_mut_;
  
  std::vector<uint64_t> periods_;
  std::vector<uint64_t> throughputs_;
  std::vector<std::chrono::high_resolution_clock::time_point> starts_;
  
  std::chrono::high_resolution_clock::time_point start_;
};

int main(int argc, char** argv) {

  if (argc < 5) {
    std::cout << "Usage: my_node_id to_node_id win_size total" << std::endl;
    return 0;
  }

  std::string my_node_name = "node_" + std::string(argv[1]);
  std::string to_node_name = "node_" + std::string(argv[2]);
  int win_size = std::stoi(argv[3]);
  int total = std::stoi(argv[4]);
  
  ndn::Name to_name("haha");
  to_name.append(to_node_name);

  Consumer consumer(my_node_name, to_name, win_size, total);

  std::cout << "Start 2 seconds warming up ..." << std::endl;
  sleep(2);
  std::cout << "After 2 seconds warming up, start commiting ..." << std::endl;
  consumer.start_consume();

  std::cout << "Main thread sleeping ..." << std::endl;
  sleep(10000000);

  return 0;
}
