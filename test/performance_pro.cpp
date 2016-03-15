/**
 * Created on Mar,14 2016
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <unistd.h>
#include <string>
#include <stdlib.h>
#include <chrono>
#include "threadpool.hpp" 

using namespace boost::threadpool;
class Producer {
 public:
  Producer(ndn::Name prefix) {
    face_ = ndn::make_shared<ndn::Face>();
    face_->setInterestFilter(prefix,
                            bind(&Producer::onInterest, this, _1, _2),
                            ndn::RegisterPrefixSuccessCallback(),
                            bind(&Producer::onRegisterFailed, this, _1, _2));
    boost::thread listen(boost::bind(&Producer::attach, this));
//    pool_ = new pool(1);
  }

  void attach() {
    face_->getIoService().run();
  }

 private:

  void onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest) {
    //std::cout << "Producer I: " << interest.getName() << std::endl;

    // Create new name, based on Interest's name
    ndn::Name dataName(interest.getName());
  
    ndn::name::Component request = interest.getName().get(-1);
    const uint8_t* value = request.value();
    size_t size = request.value_size();
    std::string msg_str(value, value + size);
    
  
    static const std::string content = "HELLO KITTY";
  
    // Create Data packet
    ndn::shared_ptr<ndn::Data> data = ndn::make_shared<ndn::Data>();
    data->setName(dataName);
    data->setFreshnessPeriod(ndn::time::seconds(10));
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
  pool *pool_;
};

int main(int argc, char** argv) {

  if (argc < 2) {
    std::cout << "Usage: my_node_id" << std::endl;
    return 0;
  }

  std::string my_node_name = "node_" + std::string(argv[1]);
  ndn::Name prefix("haha");
  prefix.append(my_node_name);

  Producer producer(prefix);

  std::cout << ("Main thread sleeping ...") << std::endl;
  sleep(10000000);
//  producer.attach();

  return 0;
}
