/**
 * Created on Dec 12, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */
#include "producer.hpp" 
#include "captain.hpp"

namespace ndnpaxos {

Producer::Producer(ndn::Name prefix) 
  : prefix_(prefix) {
  face_ = ndn::make_shared<ndn::Face>();
  face_->setInterestFilter(prefix,
                          bind(&Producer::onInterest, this, _1, _2),
                          ndn::RegisterPrefixSuccessCallback(),
                          bind(&Producer::onRegisterFailed, this, _1, _2));

}

Producer::Producer(ndn::Name prefix, Captain *captain) 
  : prefix_(prefix), captain_(captain) {
  face_ = ndn::make_shared<ndn::Face>();
  face_->setInterestFilter(prefix,
                          bind(&Producer::onInterest, this, _1, _2),
                          ndn::RegisterPrefixSuccessCallback(),
                          bind(&Producer::onRegisterFailed, this, _1, _2));

}

Producer::Producer(ndn::Name prefix, Captain *captain, ndn::shared_ptr<ndn::Face> face) 
  : face_(face), prefix_(prefix), captain_(captain) {
  face->setInterestFilter(prefix,
                          bind(&Producer::onInterest, this, _1, _2),
                          ndn::RegisterPrefixSuccessCallback(),
                          bind(&Producer::onRegisterFailed, this, _1, _2));

}

void Producer::attach() {
  boost::thread listen(boost::bind(&Producer::run, this));
}

void Producer::run() {
  LOG_INFO("Producer attached!");
//  face_->processEvents();
  face_->getIoService().run();

  LOG_INFO("Producer attach Finished?!");
} 

void Producer::onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest) {
  LOG_INFO_COM("<< Producer I: %s", interest.getName().toUri().c_str());

  // Create new name, based on Interest's name
  ndn::Name dataName(interest.getName());

  ndn::name::Component request = interest.getName().get(-1);
  const uint8_t* value = request.value();
  size_t size = request.value_size();
  std::string msg_str(value, value + size);

  static const std::string content = "HELLO KITTY";

  // Create Data packet
  ndn::shared_ptr<ndn::Data> data = ndn::make_shared<ndn::Data>();
  data->setName(dataName);
  data->setFreshnessPeriod(ndn::time::seconds(10));
  data->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());

  // Sign Data packet with default identity
  keyChain_.sign(*data);

  // Return Data packet to the requester
//  std::cout << ">> D: " << *data << std::endl;
  face_->put(*data);

  deal_msg(msg_str);
}

void Producer::onRegisterFailed(const ndn::Name& prefix, const std::string& reason) {
  std::cerr << "ERROR: Failed to register prefix \""
            << prefix << "\" in local hub's daemon (" << reason << ")"
            << std::endl;
  face_->shutdown();
}

void Producer::deal_msg(std::string msg_str) {
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
  LOG_TRACE_COM("Producer Received %s", text_str.c_str());
  captain_->handle_msg(msg, static_cast<MsgType>(type));
  LOG_TRACE_COM("Producer Handle finish!");
}

} // namespace ndnpaxos 
