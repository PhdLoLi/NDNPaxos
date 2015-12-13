/**
 * Created on Dec 09, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#pragma once

#include <ndn-cxx/face.hpp>
#include "view.hpp"
#include "threadpool.hpp" 
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>

#include <unistd.h>
#include <google/protobuf/text_format.h>


using namespace boost::threadpool;
namespace ndnpaxos {
class View;
class Producer;
class Consumer;
class Captain;
class Commo {
 public:
  Commo(Captain *captain, View &view);
  ~Commo();
  void broadcast_msg(google::protobuf::Message *, MsgType);
  void send_one_msg(google::protobuf::Message *, MsgType, node_id_t);
  void set_pool(pool *);

 private:

  View *view_;
  pool *pool_;

  // for NDN
  Consumer *con_;
  Producer *producer_;
  std::vector<ndn::Name> consumer_names_;

};
} // namespace ndnpaxos
