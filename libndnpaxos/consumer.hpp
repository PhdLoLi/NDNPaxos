/**
 * Created on Dec 12, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */
#pragma once
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>

namespace ndnpaxos {
class Consumer {
 public:
  Consumer();
  Consumer(ndn::Name prefix);
  Consumer(ndn::shared_ptr<ndn::Face>);
  void consume(ndn::Name name);

 private:
  void onData(const ndn::Interest& interest, const ndn::Data& data); 
  void onTimeout(const ndn::Interest& interest);
  ndn::shared_ptr<ndn::Face> face_;
  ndn::Name prefix_;
};
}// namespace ndnpaxos

