/**
 * Created on Dec 09, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#include "commo.hpp"
#include "captain.hpp"
#include <iostream>
#include "threadpool.hpp" 
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>

using namespace boost::threadpool;
namespace ndnpaxos {

Commo::Commo(Captain *captain, View &view) 
  : captain_(captain), view_(&view) {

  LOG_INFO_COM("%s Init START", view_->hostname().c_str());

  for (uint32_t i = 0; i < view_->nodes_size(); i++) {
    consumer_names_.push_back(ndn::Name(view_->prefix()).append(view_->hostname(i)));
//    ndn::Interest interest;
//    interest.setInterestLifetime(ndn::time::milliseconds(100));
//    interest.setMustBeFresh(true);
//    consumer_interests_.push_back(interest);
    LOG_INFO_COM("Add consumer_names[%d]: %s", i, consumer_names_[i].toUri().c_str());
  }

  face_.setInterestFilter(consumer_names_[view_->whoami()],
                        bind(&Commo::onInterest, this, _1, _2),
                        ndn::RegisterPrefixSuccessCallback(),
                        bind(&Commo::onRegisterFailed, this, _1, _2));

  boost::thread listen(boost::bind(&Commo::start, this));
}

Commo::~Commo() {
}

void Commo::start() {
  LOG_INFO("processEvents attached!");
  face_.processEvents();
//  face_.getIoService().run();
  LOG_INFO("processEvents attach Finished?!");
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
    consume(new_name);
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
  consume(new_name);
}

void Commo::onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest) {
  LOG_INFO_COM("<< Producer I: %s", interest.getName().toUri().c_str());

  // Create new name, based on Interest's name
  ndn::Name dataName(interest.getName());

  ndn::name::Component request = interest.getName().get(-1);
  const uint8_t* value = request.value();
  size_t size = request.value_size();
  std::string msg_str(value, value + size);

//  static const std::string content = "HELLO KITTY";
//
//  // Create Data packet
//  ndn::shared_ptr<ndn::Data> data = ndn::make_shared<ndn::Data>();
//  data->setName(dataName);
//  data->setFreshnessPeriod(ndn::time::seconds(10));
//  data->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());
//
//  // Sign Data packet with default identity
//  keyChain_.sign(*data);
//
//  // Return Data packet to the requester
////  std::cout << ">> D: " << *data << std::endl;
//  face_.put(*data);

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
  LOG_TRACE_COM("deal_msg Received %s", text_str.c_str());
//    pool_->schedule(boost::bind(&Captain::handle_msg, captain_, msg, static_cast<MsgType>(type)));
  captain_->handle_msg(msg, static_cast<MsgType>(type));
  LOG_TRACE_COM("deal_msg Handle finish!");
}

void Commo::consume(ndn::Name name) {
  ndn::Interest interest(name);
  interest.setInterestLifetime(ndn::time::milliseconds(1000));
  interest.setMustBeFresh(true);
  face_.expressInterest(interest,
                         bind(&Commo::onData, this,  _1, _2),
                         bind(&Commo::onTimeout, this, _1));
  LOG_TRACE_COM("Consumer Sending %s", interest.getName().toUri().c_str());
  // processEvents will block until the requested data received or timeout occurs
//  face_.processEvents();
}

void Commo::onData(const ndn::Interest& interest, const ndn::Data& data) {
  const uint8_t* value = data.getContent().value();
  size_t size = data.getContent().value_size();
  std::string value_str(value, value + size);
  LOG_TRACE_COM("Consumer onData ACK: %s", value_str.c_str());
//  std::cout << "value_str: " << value_str << std::endl;
//  std::cout << "value_size: " << size << std::endl;
//  std::cout << "interest name: " << interest.getName() << std::endl;
//  std::cout << "data name: " << data.getName() << std::endl;
}

void Commo::onTimeout(const ndn::Interest& interest) {
  LOG_DEBUG_COM("Consumer Timeout %s", interest.getName().toUri().c_str());
}
} // namespace ndnpaxos
