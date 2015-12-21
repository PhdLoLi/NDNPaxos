/**
 * Created on Dec 09, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#pragma once

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include "internal_types.hpp"
#include <unistd.h>
#include <google/protobuf/text_format.h>

namespace ndnpaxos {
class View;
class Captain;
//class pool;
class Commo {
 public:
  Commo(Captain *captain, View &view);
  ~Commo();
  void broadcast_msg(google::protobuf::Message *, MsgType);
  void send_one_msg(google::protobuf::Message *, MsgType, node_id_t);
  void send_one_msg(google::protobuf::Message *, MsgType, node_id_t, ndn::Name& dataName);
//  void send_one_msg(google::protobuf::Message *, MsgType, node_id_t, google::protobuf::Message *, MsgType);
//  void set_pool(pool *);
  void start();
  void stop();
  ndn::shared_ptr<ndn::Face> getFace();

 private:
  // for producer part
  void onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest);
  void onRegisterFailed(const ndn::Name& prefix, const std::string& reason);
  void produce(std::string &str, ndn::Name&);

  // for consumer part
  void onData(const ndn::Interest& interest, const ndn::Data& data); 
  void onTimeout(const ndn::Interest& interest, int& resendTimes);
  void consume(ndn::Name name);

  void deal_msg(std::string &msg_str, ndn::Name &dataName); 
  void deal_timeout(std::string &msg_str); 

  Captain *captain_;
  View *view_;
//  pool *pool_;

  // for NDN
  ndn::shared_ptr<ndn::Face> face_;
  ndn::unique_ptr<ndn::Scheduler> scheduler_;// scheduler
  std::vector<ndn::Name> consumer_names_;
  ndn::KeyChain keyChain_;


};
} // namespace ndnpaxos
