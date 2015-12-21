/**
 * Created on Dec 21, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <stdlib.h>

namespace ndnpaxos {

class TS
{
public:
  TS(std::string node_name)
  : node_name_(node_name) {
    face_ = ndn::make_shared<ndn::Face>();
    face_->setInterestFilter(ndn::Name("ndn/thu/paxos").append(node_name_),
                             bind(&TS::onInterest, this, _1, _2),
                             ndn::RegisterPrefixSuccessCallback(),
                             bind(&TS::onRegisterFailed, this, _1, _2));
  }

  void start() {
    std::cout << "processEvents attached!" << std::endl;
  //  face_->processEvents();
    face_->getIoService().run();
    std::cout << "processEvents Finished?!" << std::endl;
  }

  void consume(ndn::Name name) {
    ndn::Interest interest(name);
    interest.setInterestLifetime(ndn::time::milliseconds(1000));
    interest.setMustBeFresh(true);
    face_->expressInterest(interest,
                           bind(&TS::onData, this,  _1, _2),
                           bind(&TS::onTimeout, this, _1, 0));
    std::cout << "Consumer Sending I " << interest.getName() << std::endl;
  }

private:
  void onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest) {
    std::string cmd = interest.getName().get(-1).toUri();

    std::cout << "Producer I: " << interest << std::endl;

    static const std::string content = "Reply from" + node_name_;

    ndn::Name dataName(interest.getName());
    ndn::shared_ptr<ndn::Data> data = ndn::make_shared<ndn::Data>();
    data->setName(dataName);
    data->setFreshnessPeriod(ndn::time::seconds(10));
    data->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());

    keyChain_.sign(*data);

    std::cout << "Producer D: " << *data << std::endl;
    face_->put(*data);

    if (cmd.compare("COMMIT") == 0) {
      consume(ndn::Name("ndn/thu/paxos/node1/PREPARE"));
    }
  }


  void onRegisterFailed(const ndn::Name& prefix, const std::string& reason) {
    std::cerr << "ERROR: Failed to register prefix \""
              << prefix << "\" in local hub's daemon (" << reason << ")"
              << std::endl;
    face_->shutdown();
  }

  
  void onData(const ndn::Interest& interest, const ndn::Data& data) {

    std::string cmd = interest.getName().get(-1).toUri();

    std::cout << "Consumer onData " << interest.getName().toUri() << std::endl;
    if (cmd.compare("PREPARE") == 0) {
      consume(ndn::Name("ndn/thu/paxos/node1/ACCEPT"));
    } else if (cmd.compare("ACCEPT") == 0) {
      consume(ndn::Name("ndn/thu/paxos/node1/DECIDE"));
    } 
  }
  
  void onTimeout(const ndn::Interest& interest, int& resendTimes) {
    std::cout << "Consumer Timeout " << interest.getName().toUri() << " count " << resendTimes;
    
//    if (resendTimes < 3) {
//      ndn::Interest interest_new(interest);
//      face_->expressInterest(interest_new,
//                             bind(&TS::onData, this,  _1, _2),
//                             bind(&TS::onTimeout, this, _1, resendTimes + 1));
//    }
  }

private:
  ndn::shared_ptr<ndn::Face> face_;
  ndn::KeyChain keyChain_;
  std::string node_name_;
};

int main(int argc, char** argv) {

  if (argc < 2) {
    std::cout << "Usage: node0 or node1" << std::endl;
    return 0;
  }

  std::string node_name = argv[1];
 
  ndnpaxos::TS ts(node_name);
  
  if (node_name.compare("node1") == 0) {
    ts.consume(ndn::Name("ndn/thu/paxos/node0/COMMIT"));
  }

  ts.start(); 

  return 0;
}

} // namespace ndnpaxos

int main(int argc, char** argv) {
  return ndnpaxos::main(argc, argv);
}