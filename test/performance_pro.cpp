/**
 * Created on Mar,14 2016
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>
//#include <boost/thread.hpp>
//#include <boost/bind.hpp>

#include <unistd.h>
#include <string>
#include <stdlib.h>
#include <chrono>

class Producer {
 public:
  Producer(ndn::Name prefix) : prefix_(prefix) {
    face_ = ndn::make_shared<ndn::Face>();
    face_->setInterestFilter(prefix,
                            bind(&Producer::onInterest, this, _1, _2),
                            ndn::RegisterPrefixSuccessCallback(),
                            bind(&Producer::onRegisterFailed, this, _1, _2));
//    boost::thread listen(boost::bind(&Producer::attach, this));
//    pool_ = new pool(1);
  }

  void attach() {
    std::cout << "attach starting ..." << std::endl;
    face_->processEvents();
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
//
//  producer.produce();

  producer.attach();
  std::cout << ("Main thread sleeping ...") << std::endl;
  sleep(10000000);

  return 0;
}
