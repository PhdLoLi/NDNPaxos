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

class Consumer {
 public:
  Consumer(std::string node_name, ndn::Name prefix, int win_size, int total)
    : node_name_(node_name), prefix_(prefix), win_size_(win_size), total_(total), 
      commit_counter_(0), starts_(total + 1) {

    prefix_.append(node_name);

    face_ = ndn::make_shared<ndn::Face>();
    boost::thread listen(boost::bind(&Consumer::attach, this));
  }

  void attach() {
//    LOG_INFO("Consumer attached!");
    face_->getIoService().run();
//    LOG_INFO("Consumer attach Finished?!");
  }

  void start_consume() {

    start_ = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < win_size_; i++) {
      counter_mut_.lock();
      commit_counter_++;
      starts_[commit_counter_] = std::chrono::high_resolution_clock::now(); 
      counter_mut_.unlock();
      std::string value = "Commiting Value Time_" + std::to_string(commit_counter_) + " from " + node_name_;
      std::cout << " +++++++++++ ZERO Init Commit Value: %s +++++++++++ " << value << std::endl;
//      LOG_INFO(" +++++++++++ ZERO Init Commit Value: %s +++++++++++", value.c_str());
      ndn::Name new_name(prefix_);
      new_name.appendNumber(commit_counter_);
      consume(new_name);
      std::cout << " +++++++++++ ZERO Finish Commit Value: %s +++++++++++ " << value << std::endl;
//      LOG_INFO(" +++++++++++ ZERO FINISH Commit Value: %s +++++++++++", value.c_str());

    }
  }

  void consume(ndn::Name name) {
    ndn::Interest interest(name);
    interest.setInterestLifetime(ndn::time::milliseconds(1000));
    interest.setMustBeFresh(true);
    face_->expressInterest(interest,
                           bind(&Consumer::onData, this,  _1, _2),
                           bind(&Consumer::onTimeout, this, _1));
//    LOG_INFO_COM("Consumer Sending %s Finish", interest.getName().toUri().c_str());
    // processEvents will block until the requested data received or timeout occurs
  //  face_->processEvents();
  }

 private:
  
  void onData(const ndn::Interest& interest, const ndn::Data& data) {
//    const uint8_t* value = data.getContent().value();
//    size_t size = data.getContent().value_size();
//    std::string value_str(value, value + size);
    int req_num = interest.getName().get(-1).toNumber();
//    LOG_INFO_COM("Consumer onData ACK Number: %d", req_num);

    auto finish = std::chrono::high_resolution_clock::now();
    periods_.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>
                     (finish-starts_[req_num]).count());
  

    counter_mut_.lock();
    commit_counter_++;
  
    uint64_t counter_tmp = commit_counter_;
    counter_mut_.unlock();


    if (counter_tmp <= total_ * 1) {
  //    LOG_INFO("++++ I just Commit Value: %s ++++", value.c_str());
      if (counter_tmp % 10 == 0) {
        auto finish = std::chrono::high_resolution_clock::now();
        uint64_t period = std::chrono::duration_cast<std::chrono::milliseconds>(finish-start_).count();
        start_ = std::chrono::high_resolution_clock::now();
        int throughput = 10 * 1000 / period;
        printf("Last_commit -- counter:%d milliseconds:%llu throughput:%d\n", counter_tmp, period, throughput);
        printf("periods[%d] = %d\n", periods_.size() - 1, periods_[periods_.size() - 1]);
        throughputs_.push_back(throughput);
      }
//      std::cout << "master want to commit Value: " << value << std::endl;
//      boost::thread commit_first(bind(&Master::commit_thread, this, value));
//      LOG_INFO(" +++++++++++ Init Commit Value: %s +++++++++++", value.c_str());
      starts_[counter_tmp] = std::chrono::high_resolution_clock::now();
      ndn::Name new_name(prefix_);
      new_name.appendNumber(counter_tmp);
      consume(new_name);
//      LOG_INFO(" +++++++++++ FINISH Commit Value: %s +++++++++++", value.c_str());
//      std::cout << "master want to commit Value Finish: " << value << std::endl;
    }
  }
  
  void onTimeout(const ndn::Interest& interest) {
//    LOG_DEBUG_COM("Consumer Timeout %s", interest.getName().toUri().c_str());
  }

  ndn::shared_ptr<ndn::Face> face_;
  ndn::Name prefix_;
  ndn::KeyChain keyChain_;

  std::string node_name_;

  int win_size_;
  int total_;
  uint64_t commit_counter_;

  boost::mutex counter_mut_;
  
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
