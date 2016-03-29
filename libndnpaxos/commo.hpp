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
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include "threadpool.hpp" 

using namespace boost::threadpool;
namespace ndnpaxos {
class View;
class Captain;
class Commo {
 public:
  Commo(Captain *captain, View &view, int role = 0);
  ~Commo();
  void broadcast_msg(google::protobuf::Message *, MsgType);
  void send_one_msg(google::protobuf::Message *, MsgType);
  void send_one_msg(google::protobuf::Message *, MsgType, node_id_t);
  void send_one_msg(google::protobuf::Message *, MsgType, node_id_t, ndn::Name& dataName);
  void produce_log(slot_id_t, PropValue *);
  void consume_log(int win_size);
//  void send_one_msg(google::protobuf::Message *, MsgType, node_id_t, google::protobuf::Message *, MsgType);
//  void set_pool(pool *);
  void start();
  void stop();
  ndn::shared_ptr<ndn::Face> getFace();
  void inform_client(slot_id_t slot_id, int try_time, ndn::Name &dataName);


 private:
  // for producer part
  void onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest);
  void onInterestLog(const ndn::InterestFilter& filter, const ndn::Interest& interest);
  void onInterestCommit(const ndn::InterestFilter& filter, const ndn::Interest& interest);
  void onRegisterSucceed(const ndn::InterestFilter& filter);
  void onRegisterFailed(const ndn::Name& prefix, const std::string& reason);
  void produce(std::string &str, ndn::Name&, int seconds = 0);

  // for consumer part
  void onData(const ndn::Interest& interest, const ndn::Data& data); 
  void onDataLog(const ndn::Interest& interest, const ndn::Data& data); 
  void onNack(const ndn::Interest& interest, const ndn::lp::Nack& nack); 
  void onTimeout(const ndn::Interest& interest, int& resendTimes);
  void consume(ndn::Name& name);
  void consume_log_next();

  void deal_msg(std::string &msg_str, ndn::Name &dataName); 
  void deal_nack(std::string &msg_str); 
  
  void handle_myself(google::protobuf::Message *msg, MsgType msg_type);

  Captain *captain_;
  View *view_;
  pool *pool_;
  pool *win_pool_;

  // for NDN
  ndn::shared_ptr<ndn::Face> face_;
//  ndn::unique_ptr<ndn::Scheduler> scheduler_;// scheduler
  std::vector<ndn::Name> consumer_names_;
  ndn::KeyChain keyChain_;
  ndn::Name log_name_;
  ndn::Name commit_name_;

  bool reg_ok_;

  slot_id_t log_counter_;
  boost::mutex log_mut_;
//  boost::mutex reg_ok_mutex_;
//  boost::condition_variable reg_ok_cond_;
};
} // namespace ndnpaxos
