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
#include <boost/thread/condition.hpp>

#include <unistd.h>
#include <string>
#include <stdlib.h>
#include <chrono>

class Producer {
 public:
  Producer(ndn::Name prefix) : prefix_(prefix) {
    face_ = ndn::make_shared<ndn::Face>();
    prefix_id_ = face_->setInterestFilter(prefix,
                            bind(&Producer::onInterest, this, _1, _2),
                            ndn::RegisterPrefixSuccessCallback(),
                            bind(&Producer::onRegisterFailed, this, _1, _2));
//    boost::thread timer(boost::bind(&Producer::recording, this));
    boost::thread listen(boost::bind(&Producer::attach, this));
//    pool_ = new pool(1);
  }

  ~Producer() {
    face_->shutdown();
  }

  void recording() {
    int warming = 2;
    int interval = 3;
    for (int i = 0; i < warming + interval; i++) {
      printf("Counting %d\n", i + 1);
      sleep(1);
    }
    printf("%d s passed start punching\n", warming + interval);
    int punch = 1;
    for (int i = 0; i < interval * 4; i++) {
      sleep(punch);
      printf("PUNCH! %d\n", i + 1);
    }
    printf("Last %d s period\n", interval);
    for (int i = interval ; i > 0; i--) {
      printf("Counting %d\n", i);
      sleep(1);
    }
//    face_->unsetInterestFilter(prefix_id_);
//    face_->getIoService().stop();
    face_->shutdown();
    printf("Last Last %d s period\n", warming);
    for (int i = warming ; i > 0; i--) {
      printf("Counting %d\n", i);
      sleep(1);
    }
//    face_->shutdown();
    printf("Over!!!\n");
  }
  

  void attach() {
    std::cout << "Producer attach starting ..." << std::endl;
    face_->processEvents();
    std::cout << "Producer attach finishing ..." << std::endl;
//    face_->getIoService().run();
  }

  void produce() {
    std::cout << "start producing" << std::endl;
    start_ = std::chrono::high_resolution_clock::now();
    int total = 100000;
    for (int i = 0; i < total; i ++) {
      // Create new name, based on Interest's name
      ndn::Name dataName(prefix_);
      dataName.append(std::to_string(i));
//      dataName.appendNumber(i);
  
      static const std::string content = "HELLO WORLD";
  
      // Create Data packet
      ndn::shared_ptr<ndn::Data> data = ndn::make_shared<ndn::Data>();
      data->setName(dataName);
      data->setFreshnessPeriod(ndn::time::seconds(100));
      data->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());
//      pool_->schedule(boost::bind(&Producer::sign_thread, this, data));
      keyChain_.signWithSha256(*data);
      face_->put(*data);

    }
    auto finish = std::chrono::high_resolution_clock::now();
    uint64_t period = std::chrono::duration_cast<std::chrono::milliseconds>(finish-start_).count();
    int throughput = total * 1000 / period;
    printf("produce finished -- counter:%d milliseconds:%llu throughput:%d\n", total, period, throughput);
  }

 private:


  void onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest) {
//    std::cout << "Producer I: " << interest.getName() << std::endl;

    // Create new name, based on Interest's name
    ndn::Name dataName(interest.getName());
  
    static const std::string content = "HELLO WORLD";
  
    // Create Data packet
    ndn::shared_ptr<ndn::Data> data = ndn::make_shared<ndn::Data>();
    data->setName(dataName);
    data->setFreshnessPeriod(ndn::time::seconds(0));
    data->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());
//    pool_->schedule(boost::bind(&Producer::sign_thread, this, data));
    keyChain_.signWithSha256(*data);
    face_->put(*data);
//    sign_thread(data);
  }
  
  void sign_thread(ndn::shared_ptr<ndn::Data> data) {
    // Sign Data packet with default identity
//    keyChain_.sign(*data);
    keyChain_.signWithSha256(*data);
    // Return Data packet to the requester
    face_->put(*data);
    std::cout << "Producer Signing Done!!! " << std::endl;
  }

  void onRegisterFailed(const ndn::Name& prefix, const std::string& reason) {
    std::cerr << "ERROR: Failed to register prefix \""
              << prefix << "\" in local hub's daemon (" << reason << ")"
              << std::endl;
    face_->shutdown();
  }

  ndn::shared_ptr<ndn::Face> face_;
  ndn::Name prefix_;
  ndn::KeyChain keyChain_;
//  pool *pool_;
  std::chrono::high_resolution_clock::time_point start_;
  const ndn::RegisteredPrefixId *prefix_id_;

};

int main(int argc, char** argv) {

  if (argc < 2) {
    std::cout << "Usage: my_node_id" << std::endl;
    return 0;
  }

  std::string my_node_name = "node_" + std::string(argv[1]);
  ndn::Name prefix("hehe");
  prefix.append(my_node_name);

  Producer producer(prefix);
  producer.recording();

//  producer.produce();

  return 0;
}
