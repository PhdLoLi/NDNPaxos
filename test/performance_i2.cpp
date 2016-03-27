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
    : my_prefix_(my_prefix), to_prefix_(to_prefix) {

//    prefix_.append("node_" + std::to_string(to_node_id));
    face_ = ndn::make_shared<ndn::Face>();
    face_->setInterestFilter(my_prefix,
               bind(&CP::onInterest, this, _1, _2),
               bind(&CP::onRegisterSucceed, this, _1),
               bind(&CP::onRegisterFailed, this, _1, _2));
//    pool_ = new pool(win_size);
//    boost::thread listen(boost::bind(&CP::attach, this));
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

  void consume(ndn::Name name) {
    ndn::Interest interest(name);
    interest.setInterestLifetime(ndn::time::milliseconds(1000));
    interest.setMustBeFresh(true);
    face_->expressInterest(interest,
                           bind(&CP::onData, this,  _1, _2),
                           bind(&CP::onTimeout, this, _1));
//    std::cout << "Sending " << interest << std::endl;
  }
 
  void onData(const ndn::Interest& interest, const ndn::Data& data) {
  }
  
  void onTimeout(const ndn::Interest& interest) {
//    LOG_DEBUG_COM("CP Timeout %s", interest.getName().toUri().c_str());
  }

  std::vector<uint64_t> periods_;
  std::vector<uint64_t> throughputs_;

 private:

  void onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest) {
//    std::cout << "Producer I: " << interest.getName() << std::endl;
    int req_num = std::stoi(interest.getName().get(-1).toUri());
    ndn::Name new_name(to_prefix_);
    consume(new_name.append(std::to_string(req_num)));
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
};

int main(int argc, char** argv) {

  if (argc < 3) {
    std::cout << "Usage: my_node_id" << std::endl;
    return 0;
  }

  int my_node_id = std::stoi(argv[1]);
  int to_node_id = std::stoi(argv[2]);

  std::string prefix = "hehe";
  ndn::Name my_prefix = ndn::Name(prefix).append("node_" + std::to_string(my_node_id));
  ndn::Name to_prefix = ndn::Name(prefix).append("node_" + std::to_string(to_node_id));
  CP cp(my_prefix, to_prefix);

  cp.attach();


  return 0;
}
