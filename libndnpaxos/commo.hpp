/**
 * Created on Dec 09, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#pragma once

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/security/key-chain.hpp>

#include "view.hpp"
#include "threadpool.hpp" 
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>

#include <unistd.h>
#include <google/protobuf/text_format.h>


using namespace boost::threadpool;
namespace ndnpaxos {
class Captain;
class Commo {
 public:
  Commo(std::vector<Captain *> &);
  Commo(Captain *captain, View &view, pool *pool);
  Commo(Captain *captain, View &view);
  ~Commo();
  void waiting_msg();
  void broadcast_msg(google::protobuf::Message *, MsgType);
  void send_one_msg(google::protobuf::Message *, MsgType, node_id_t);
  void set_pool(pool *);

  void deal_msg(std::string);

 private:
  void onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest);
  void onRegisterFailed(const ndn::Name& prefix, const std::string& reason);
  void onData(const ndn::Interest& interest, const ndn::Data& data);
  void onTimeout(const ndn::Interest& interest);

  std::vector<Captain *> captains_;
  Captain * captain_;
  View *view_;
  pool *pool_;

  // for NDN
  boost::asio::io_service io_service_;
  ndn::Face face_;
  ndn::Scheduler scheduler_;
  ndn::KeyChain key_chain_;

  std::vector<ndn::Name> consumer_names_;
  std::vector<ndn::Interest> consumer_interests_;

};
} // namespace ndnpaxos
