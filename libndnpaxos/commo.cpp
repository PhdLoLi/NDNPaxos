/**
 * Created on Dec 09, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#include "consumer.hpp"
#include "producer.hpp"
#include "commo.hpp"
#include <iostream>

namespace ndnpaxos {

Commo::Commo(Captain *captain, View &view) 
  : view_(&view) {
  producer_ = new Producer(ndn::Name(view_->prefix()).
                          append(view_->hostname()), captain);
  con_ = new Consumer();

  LOG_INFO_COM("%s Init START", view_->hostname().c_str());

  for (uint32_t i = 0; i < view_->nodes_size(); i++) {
    consumer_names_.push_back(ndn::Name(view_->prefix()).append(view_->hostname(i)));
//    ndn::Interest interest;
//    interest.setInterestLifetime(ndn::time::milliseconds(100));
//    interest.setMustBeFresh(true);
//    consumer_interests_.push_back(interest);
    LOG_INFO_COM("Add consumer_names[%d]: %s", i, consumer_names_[i].toUri().c_str());
  }
  producer_->attach();
}

Commo::~Commo() {
}

//void Commo::set_pool(ThreadPool *pool) {
void Commo::set_pool(pool *pl) {
  pool_ = pl;
}

void Commo::broadcast_msg(google::protobuf::Message *msg, MsgType msg_type) {

  std::string msg_str;
  msg->SerializeToString(&msg_str);
  msg_str.append(std::to_string(msg_type));
  ndn::name::Component message(reinterpret_cast<const uint8_t*>
                               (msg_str.c_str()), msg_str.size());

  for (uint32_t i = 0; i < view_->nodes_size(); i++) {
    
    if (i == view_->whoami()) {
      continue;
    }

    ndn::Name new_name(consumer_names_[i]);
    new_name.append(message);

    LOG_DEBUG_COM("Broadcast to --%s (msg_type):%d", view_->hostname(i).c_str(), msg_type);
    // need to change to multithread
    con_->consume(new_name);
    LOG_DEBUG_COM("Broadcast to --%s (msg_type):%d finished", view_->hostname(i).c_str(), msg_type);
  }

}

void Commo::send_one_msg(google::protobuf::Message *msg, MsgType msg_type, node_id_t node_id) {
//  std::cout << " --- Commo Send ONE to captain " << node_id << " MsgType: " << msg_type << std::endl;
  LOG_DEBUG_COM("Send ONE to --%s (msg_type):%d", view_->hostname(node_id).c_str(), msg_type);

  std::string msg_str;
  msg->SerializeToString(&msg_str);
  msg_str.append(std::to_string(msg_type));
  ndn::name::Component message(reinterpret_cast<const uint8_t*>
                               (msg_str.c_str()), msg_str.size());
  ndn::Name new_name(consumer_names_[node_id]);
  new_name.append(message);
  con_->consume(new_name);
}

} // namespace ndnpaxos
