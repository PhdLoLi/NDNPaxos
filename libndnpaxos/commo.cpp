/**
 * Created on Dec 09, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#include "commo.hpp"
#include "captain.hpp"
#include <iostream>
//#include "threadpool.hpp" 
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>

//using namespace boost::threadpool;
namespace ndnpaxos {

Commo::Commo(Captain *captain, View &view) 
  : captain_(captain), view_(&view), reg_ok_(false) {

  face_ = ndn::make_shared<ndn::Face>();
  scheduler_ = ndn::unique_ptr<ndn::Scheduler>(new ndn::Scheduler(face_->getIoService()));
  LOG_INFO_COM("%s Init START", view_->hostname().c_str());

  for (uint32_t i = 0; i < view_->nodes_size(); i++) {
    consumer_names_.push_back(ndn::Name(view_->prefix()).append(view_->hostname(i)));
//    ndn::Interest interest;
//    interest.setInterestLifetime(ndn::time::milliseconds(100));
//    interest.setMustBeFresh(true);
//    consumer_interests_.push_back(interest);
    LOG_INFO_COM("Add consumer_names[%d]: %s", i, consumer_names_[i].toUri().c_str());
  }

  LOG_INFO("setInterestFilter start %s", consumer_names_[view_->whoami()].toUri().c_str());
  face_->setInterestFilter(consumer_names_[view_->whoami()],
                        bind(&Commo::onInterest, this, _1, _2),
                        bind(&Commo::onRegisterSucceed, this, _1),
                        bind(&Commo::onRegisterFailed, this, _1, _2));
//  boost::thread listen(boost::bind(&Commo::start, this));
}

Commo::~Commo() {
}

void Commo::start() {
  LOG_INFO("processEvents attached!");
//  face_->processEvents();
  face_->getIoService().run();
  LOG_INFO("processEvents attach Finished?!");
} 

//void Commo::set_pool(ThreadPool *pool) {
//void Commo::set_pool(pool *pl) {
//  pool_ = pl;
//}

void Commo::broadcast_msg(google::protobuf::Message *msg, MsgType msg_type) {
  
  // boost::mutex::scoped_lock lock(reg_ok_mutex_);
  // while (!reg_ok_) reg_ok_cond_.wait(lock);

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

    LOG_DEBUG_COM("Broadcast to --%s (msg_type):%s", view_->hostname(i).c_str(), msg_type_str[msg_type].c_str());

    if (msg_type == PREPARE)
      scheduler_->scheduleEvent(ndn::time::milliseconds(0),
                             bind(&Commo::consume, this, new_name));
    else // ACCEPT DECIDE 
      consume(new_name);

    LOG_DEBUG_COM("Broadcast to --%s (msg_type):%s finished", view_->hostname(i).c_str(), msg_type_str[msg_type].c_str());
  }

}

void Commo::produce(std::string &content, ndn::Name& dataName) {
  // Create Data packet
  ndn::shared_ptr<ndn::Data> data = ndn::make_shared<ndn::Data>();
  data->setName(dataName);
  data->setFreshnessPeriod(ndn::time::seconds(10));
  data->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());

  // Sign Data packet with default identity
  keyChain_.sign(*data);

  // Return Data packet to the requester
//  std::cout << ">>Producer D: " << *data << std::endl;
  face_->put(*data);

}

void Commo::send_one_msg(google::protobuf::Message *msg, MsgType msg_type, node_id_t node_id) {
//  std::cout << " --- Commo Send ONE to captain " << node_id << " MsgType: " << msg_type << std::endl;
  // boost::mutex::scoped_lock lock(reg_ok_mutex_);
  // while (!reg_ok_) reg_ok_cond_.wait(lock);

  LOG_DEBUG_COM("Send ONE to --%s (msg_type):%s", view_->hostname(node_id).c_str(), msg_type_str[msg_type].c_str());

  std::string msg_str;
  msg->SerializeToString(&msg_str);
  msg_str.append(std::to_string(msg_type));

  ndn::name::Component message(reinterpret_cast<const uint8_t*>
                               (msg_str.c_str()), msg_str.size());
  ndn::Name new_name(consumer_names_[node_id]);
  new_name.append(message);
//    scheduler_->scheduleEvent(ndn::time::milliseconds(0),
//                             bind(&Commo::consume, this, new_name));
  if (msg_type == COMMIT) {
    //face_->getIoService().run();
    scheduler_->scheduleEvent(ndn::time::milliseconds(0),
                             bind(&Commo::consume, this, new_name));
  } else 
    consume(new_name);
}

void Commo::send_one_msg(google::protobuf::Message *msg, MsgType msg_type, node_id_t node_id, ndn::Name& dataName) {

  LOG_DEBUG_COM("Reply ONE to --%s (msg_type):%s", view_->hostname(node_id).c_str(), msg_type_str[msg_type].c_str());

  std::string msg_str;
  msg->SerializeToString(&msg_str);
  msg_str.append(std::to_string(msg_type));

  produce(msg_str, dataName);
}

void Commo::onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest) {
  ndn::Name dataName(interest.getName());
  LOG_DEBUG_COM("<< Producer I: %s", dataName.toUri().c_str());

  // Create new name, based on Interest's name
  ndn::name::Component request = interest.getName().get(-1);
  const uint8_t* value = request.value();
  size_t size = request.value_size();
  std::string msg_str(value, value + size);

  deal_msg(msg_str, dataName);
}

void Commo::onRegisterSucceed(const ndn::InterestFilter& filter) {
  LOG_INFO_COM("onRegisterSucceed! %s", filter.getPrefix().toUri().c_str());
  // std::cerr << "onRegisterSucceed! " << filter << std::endl;
  // boost::mutex::scoped_lock lock(reg_ok_mutex_);
  // reg_ok_ = true;
  // reg_ok_cond_.notify_one();
}

void Commo::onRegisterFailed(const ndn::Name& prefix, const std::string& reason) {
  std::cerr << "ERROR: Failed to register prefix \""
            << prefix << "\" in local hub's daemon (" << reason << ")"
            << std::endl;
  face_->shutdown();
}

void Commo::deal_msg(std::string &msg_str, ndn::Name &dataName) {
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
  captain_->handle_msg(msg, static_cast<MsgType>(type), dataName);
  LOG_TRACE_COM("deal_msg Handle finish!");
}

void Commo::deal_nack(std::string &msg_str) {
  int type = int(msg_str.back() - '0');
  if (type == COMMIT) {
    MsgCommit *msg_com = new MsgCommit();
    msg_str.pop_back();
    msg_com->ParseFromString(msg_str);
    std::string text_str;
    google::protobuf::TextFormat::PrintToString(*msg_com, &text_str);
    LOG_TRACE_COM("deal_nack Received %s", text_str.c_str());
    captain_->master_change(msg_com->mutable_prop_value());
    LOG_TRACE_COM("deal_nack Handle finish!");
  }
}

void Commo::consume(ndn::Name name) {
  ndn::Interest interest(name);
  interest.setInterestLifetime(ndn::time::milliseconds(1000));
  interest.setMustBeFresh(true);
  face_->expressInterest(interest,
                         bind(&Commo::onData, this,  _1, _2),
                         bind(&Commo::onNack, this,  _1, _2),
                         bind(&Commo::onTimeout, this, _1, 0));
  LOG_TRACE_COM("Consumer Sending %s", interest.getName().toUri().c_str());
}

void Commo::onData(const ndn::Interest& interest, const ndn::Data& data) {
  ndn::Name dataName(interest.getName());
  const uint8_t* value = data.getContent().value();
  size_t size = data.getContent().value_size();
  std::string value_str(value, value + size);
  LOG_TRACE_COM("Consumer onData get");
  deal_msg(value_str, dataName);
}

void Commo::onNack(const ndn::Interest& interest, const ndn::lp::Nack& nack) {
  LOG_DEBUG_COM("Consumer NACK %s", interest.getName().toUri().c_str());
  ndn::name::Component request = interest.getName().get(-1);
  const uint8_t* value = request.value();
  size_t size = request.value_size();
  std::string msg_str(value, value + size);
  
  deal_nack(msg_str);

}

void Commo::onTimeout(const ndn::Interest& interest, int& resendTimes) {
  LOG_DEBUG_COM("Consumer Timeout %s, count %d", interest.getName().toUri().c_str(), resendTimes);
//  std::cerr << ndn::time::steady_clock::now() << " Consumer Timeout " << interest.getName().toUri() << " count " << resendTimes << std::endl;
  
  if (resendTimes < MAX_TIMEOUT) {
//    std::cerr << "Rexpress interest " << interest << std::endl;
    ndn::Interest interest_new(interest);
    interest_new.refreshNonce();
//    std::cerr << "Rexpress interest_new " << interest_new << std::endl;
    face_->expressInterest(interest_new,
                           bind(&Commo::onData, this,  _1, _2),
                           bind(&Commo::onNack, this,  _1, _2),
                           bind(&Commo::onTimeout, this, _1, resendTimes + 1));
  } else {
  }

}

ndn::shared_ptr<ndn::Face> Commo::getFace() {
  return face_;
}

void Commo::stop() {
  face_->getIoService().stop();
}

} // namespace ndnpaxos
