/**
 * Created on Dec 09, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#include "commo.hpp"
#include "captain.hpp"
#include <iostream>

namespace ndnpaxos {

Commo::Commo(Captain *captain, View &view) 
  : captain_(captain), view_(&view), 
    face_(io_service_), scheduler_(io_service_) {

  LOG_INFO_COM("%s Init START", view_->hostname().c_str());

  for (uint32_t i = 0; i < view_->nodes_size(); i++) {
    consumer_names_.push_back(ndn::Name(view_->prefix()).append(view_->hostname(i)));
    ndn::Interest interest;
    interest.setInterestLifetime(ndn::time::milliseconds(100));
    interest.setMustBeFresh(true);
    consumer_interests_.push_back(interest);
    LOG_INFO_COM("Add consumer_names[%d]: %s", i, consumer_names_[i].toUri().c_str());
  }

  boost::thread listen(boost::bind(&Commo::waiting_msg, this)); 
}

Commo::~Commo() {
}

void Commo::waiting_msg() {
  face_.setInterestFilter(consumer_names_[view_->whoami()],
                          bind(&Commo::onInterest, this, _1, _2),
                          ndn::RegisterPrefixSuccessCallback(),
                          bind(&Commo::onRegisterFailed, this, _1, _2));
  io_service_.run();
}

// acceptors receive interest need to deal with message 
void Commo::onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest) {
  std::string msg_str = interest.getName().get(-1).toUri();
  deal_msg(msg_str);
}

void Commo::onRegisterFailed(const ndn::Name& prefix, const std::string& reason) {
  std::cerr << "ERROR: Failed to register prefix \""
            << prefix << "\" in local hub's daemon (" << reason << ")"
            << std::endl;
  face_.shutdown();
}

void Commo::deal_msg(std::string msg_str) {
//  std::string msg_str(static_cast<char*>(request.data()), request.size());
  int type = int(msg_str.back() - '0');
  google::protobuf::Message *msg = nullptr;
//    LOG_DEBUG_COM("type %d", type);
  switch(type) {
    case PREPARE: {
      msg = new MsgPrepare();
      break;
    }
    case PROMISE: {
      msg = new MsgAckPrepare();
      break;
    }
    case ACCEPT: {
      msg = new MsgAccept();
      break;
    }
    case ACCEPTED: {
      msg = new MsgAckAccept();
      break;
    }
    case DECIDE: {
      msg = new MsgDecide();
      break;
    }
    case LEARN: {
      msg = new MsgLearn();
      break;
    }                        
    case TEACH: {
      msg = new MsgTeach();
      break;
    }
    case COMMIT: {
      msg = new MsgCommit();
      break;
    }
    case COMMAND: {
      msg = new MsgCommand();
      break;
    }
  }
  msg_str.pop_back();
  msg->ParseFromString(msg_str);
  std::string text_str;
  google::protobuf::TextFormat::PrintToString(*msg, &text_str);
  LOG_DEBUG_COM("Received %s", text_str.c_str());
//    pool_->schedule(boost::bind(&Captain::handle_msg, captain_, msg, static_cast<MsgType>(type)));
  captain_->handle_msg(msg, static_cast<MsgType>(type));
  LOG_DEBUG("Handle finish!");
}

//void Commo::set_pool(ThreadPool *pool) {
void Commo::set_pool(pool *pl) {
  pool_ = pl;
}

void Commo::broadcast_msg(google::protobuf::Message *msg, MsgType msg_type) {

  for (uint32_t i = 0; i < view_->nodes_size(); i++) {
    
    if (i == view_->whoami()) {
      continue;
    }

    std::string msg_str;
    msg->SerializeToString(&msg_str);
    msg_str.append(std::to_string(msg_type));
    
    consumer_interests_[i].setName(ndn::Name(consumer_names_[i].append(msg_str)));

    LOG_DEBUG_COM("Broadcast to --%s (msg_type):%d", view_->hostname(i).c_str(), msg_type);
    face_.expressInterest(consumer_interests_[i],
                          bind(&Commo::onData, this, _1, _2),
                          bind(&Commo::onTimeout, this, _1));
    LOG_DEBUG_COM("Broadcast to --%s (msg_type):%d finished", view_->hostname(i).c_str(), msg_type);
  }

}

void Commo::send_one_msg(google::protobuf::Message *msg, MsgType msg_type, node_id_t node_id) {
//  std::cout << " --- Commo Send ONE to captain " << node_id << " MsgType: " << msg_type << std::endl;
  LOG_DEBUG_COM("Send ONE to --%s (msg_type):%d", view_->hostname(node_id).c_str(), msg_type);

  std::string msg_str;
  msg->SerializeToString(&msg_str);
  msg_str.append(std::to_string(msg_type));

  consumer_interests_[node_id].setName(ndn::Name(consumer_names_[node_id].append(msg_str)));

  face_.expressInterest(consumer_interests_[node_id],
                          bind(&Commo::onData, this, _1, _2),
                          bind(&Commo::onTimeout, this, _1));
}

// now only use Intersts to exchange info, so there is no need to deal with data
void Commo::onData(const ndn::Interest& interest, const ndn::Data& data) {
//  std::cout << data << std::endl;
}

void Commo::onTimeout(const ndn::Interest& interest) {
  // Do nothing or need to retransmit??
  //std::cout << "Timeout " << interest << std::endl;
}

} // namespace ndnpaxos
