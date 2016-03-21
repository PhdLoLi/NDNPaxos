/**
 * Created on Dec 12, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */
#pragma once
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <google/protobuf/text_format.h>
#include "threadpool.hpp" 

using namespace boost::threadpool;

namespace ndnpaxos {
class Captain;
class Producer {
 public:
  Producer(ndn::Name prefix);
  Producer(ndn::Name prefix, Captain *captain);
  Producer(ndn::Name prefix, Captain *captain, ndn::shared_ptr<ndn::Face> face);
  void attach();
  void run();
  void init_curr_value();

 private:
  void onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest);
  void onRegisterFailed(const ndn::Name& prefix, const std::string& reason);
  void deal_msg(std::string msg_str); 

  ndn::shared_ptr<ndn::Face> face_;
  ndn::Name prefix_;
  Captain *captain_;
  ndn::KeyChain keyChain_;
};
}// namespace ndnpaxos
