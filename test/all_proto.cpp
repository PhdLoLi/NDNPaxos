/**
 * Created on Dec 11, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/security/key-chain.hpp>

#include "view.hpp"
#include "threadpool.hpp" 
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>

#include <unistd.h>
#include <google/protobuf/text_format.h>
#include <string>


using namespace boost::threadpool;

namespace ndnpaxos {

class CP {

 public:
  CP(View &view) 
    : face_producer_(m_ioService), face_consumer_(m_ioService), view_(&view) {

    for (uint32_t i = 0; i < view_->nodes_size(); i++) {
      consumer_names_.push_back(ndn::Name(view_->prefix()).append(view_->hostname(i)));
//      ndn::Interest interest;
//      interest.setInterestLifetime(ndn::time::milliseconds(1000));
//      interest.setMustBeFresh(true);
//      consumer_interests_.push_back(interest);
      LOG_INFO_COM("Add consumer_names[%d]: %s", i, consumer_names_[i].toUri().c_str());
    }

//    waiting_msg();
    boost::thread listen(boost::bind(&CP::waiting_msg, this)); 

  }

  void waiting_msg() {
    std::cout << "Init waiting_msg prefix: " << consumer_names_[view_->whoami()].toUri() 
              << " start listening!" << std::endl;
    face_producer_.setInterestFilter(consumer_names_[view_->whoami()].toUri(),
                            bind(&CP::onInterest, this, _1, _2),
                            ndn::RegisterPrefixSuccessCallback(),
                            bind(&CP::onRegisterFailed, this, _1, _2));

    std::cout << "Before call processEvents" << std::endl;
    face_producer_.processEvents();
    std::cout << "After call processEvents" << std::endl;
  }
  
  void io_run() {
    m_ioService.run();
  }

  void broadcast_msg(google::protobuf::Message *msg, MsgType msg_type) {
  
    std::string msg_str;
    msg->SerializeToString(&msg_str);
    msg_str.append(std::to_string(msg_type));
    
    ndn::name::Component message(reinterpret_cast<const uint8_t*>(msg_str.c_str()), msg_str.size());

    for (uint32_t i = 0; i < view_->nodes_size(); i++) {
      
      if (i == view_->whoami()) {
        continue;
      }
  
//      ndn::Name new_name = ndn::Name(consumer_names_[i]).append(message);
//      consumer_interests_[i].setName(new_name);
//  
//      LOG_INFO_COM("Broadcast to --%s (msg_type):%d", view_->hostname(i).c_str(), msg_type);
//      face_consumer_.expressInterest(consumer_interests_[i],
//                            bind(&CP::onData, this, _1, _2),
//                            bind(&CP::onTimeout, this, _1));
//      face_consumer_.processEvents();
      LOG_INFO_COM("Broadcast to --%s (msg_type):%d finished", view_->hostname(i).c_str(), msg_type);
    }
  
  }

  void send_one_msg(google::protobuf::Message *msg, MsgType msg_type, node_id_t node_id) {

    LOG_INFO_COM("Send ONE to --%s (msg_type):%d", view_->hostname(node_id).c_str(), msg_type);
  
    std::string msg_str;
    msg->SerializeToString(&msg_str);
    msg_str.append(std::to_string(msg_type));

    ndn::name::Component message(reinterpret_cast<const uint8_t*>
                                 (msg_str.c_str()), msg_str.size());
    ndn::Name new_name(consumer_names_[node_id]);
    new_name.append(message);

    ndn::Interest interest(new_name);
    interest.setInterestLifetime(ndn::time::milliseconds(1000));
    interest.setMustBeFresh(true);

//    consumer_interests_[node_id].setName(new_name);
  
    face_consumer_.expressInterest(interest,
                          bind(&CP::onData, this, _1, _2),
                          bind(&CP::onTimeout, this, _1));
    face_consumer_.processEvents();
//    std::cout << "Sending Interest: " << consumer_interests_[node_id] << std::endl;
    
  }

  google::protobuf::Message * generate_msg(std::string value) {
    MsgAccept *msg_acc = new MsgAccept();
    MsgHeader *msg_header = new MsgHeader();
    msg_header->set_msg_type(MsgType::ACCEPT);
    msg_header->set_node_id(view_->whoami());
    msg_header->set_slot_id(0);

    msg_acc->set_allocated_msg_header(msg_header);
    msg_acc->set_ballot_id((1<<16) + 1);
    PropValue *rs_value = new PropValue();
    rs_value->set_id(2);
    rs_value->set_data(value);
    msg_acc->set_allocated_prop_value(rs_value);
    return msg_acc;
  }

 private:
  void onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest) {
    std::cout << "<< Receive Interest: " << interest << std::endl;
 
    // Create new name, based on Interest's name
    ndn::Name dataName(interest.getName());
    ndn::name::Component request = interest.getName().get(-1);
    const uint8_t* value = request.value();
    size_t size = request.value_size();
    std::string msg_str(value, value + size);
    deal_msg(msg_str);
   
    static const std::string content = "HELLO KITTY";
 
    // Create Data packet
    ndn::shared_ptr<ndn::Data> data = ndn::make_shared<ndn::Data>();
    data->setName(dataName);
    data->setFreshnessPeriod(ndn::time::seconds(10));
    data->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());
 
    // Sign Data packet with default identity
    keyChain_.sign(*data);
 
    // Return Data packet to the requester
//    std::cout << ">> Sending D: " << *data << std::endl;
    face_producer_.put(*data);
  }
 

  void onRegisterFailed(const ndn::Name& prefix, const std::string& reason) {
    std::cerr << "ERROR: Failed to register prefix \""
              << prefix << "\" in local hub's daemon (" << reason << ")"
              << std::endl;
    face_producer_.shutdown();
  }

  void deal_msg(std::string msg_str) {

    int type = int(msg_str.back() - '0');
    google::protobuf::Message *msg = nullptr;
    LOG_INFO_COM("type %d", type);
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
    std::cout << "msg_str size: " << msg_str.size() << std::endl;
    msg->ParseFromString(msg_str);
    std::string text_str;
    google::protobuf::TextFormat::PrintToString(*msg, &text_str);
    LOG_INFO_COM("Received\n %s", text_str.c_str());

    handle_msg(msg, static_cast<MsgType>(type));

    LOG_INFO_COM("Return this message");
  }

  void handle_msg(google::protobuf::Message *msg, MsgType msg_type) {
    switch (msg_type) {
      case PREPARE: {
        break;
      }
      case PROMISE: {
        break;
      }
      case ACCEPT: {
        MsgAccept *msg_acc = (MsgAccept *)msg;
        node_id_t from_node = msg_acc->msg_header().node_id();
        std::string value = "Lijing from " + view_->hostname();
        LOG_INFO("case ACCEPT! Sending one message to %s", 
                 (view_->hostname(from_node)).c_str());
        send_one_msg(generate_msg(value), ACCEPT, from_node);
        break;
      }
      case ACCEPTED: {
        break;
      }
      case DECIDE: {
        break;
      }
      case LEARN: {
        break;
      }
      case TEACH: {
        break;
      }
      case COMMIT: {
        break;
      }
      case COMMAND: {
        break;
      }
    }

  }

  void onData(const ndn::Interest& interest, const ndn::Data& data) {
    std::cout << "onData received!" << data << std::endl;
  }
  
  void onTimeout(const ndn::Interest& interest) {
    // Do nothing or need to retransmit??
    std::cout << "Timeout " << interest << std::endl;
  }

 private:
  boost::asio::io_service m_ioService;
  ndn::Face face_producer_;
  ndn::Face face_consumer_;
//  ndn::Scheduler m_scheduler;
  ndn::KeyChain keyChain_;
  std::vector<ndn::Name> consumer_names_;
  std::vector<ndn::Interest> consumer_interests_;
  View *view_;
};

} // namespace ndnpaxos

int
main(int argc, char** argv)
{
  if (argc == 1) {
    std::cout << "Usage: node_id to_nodes(-1/broadcast or node_id)" << std::endl;
    return 0;
  }

  int node_id = std::stoi(argv[1]);
  int node_num = 3;
  std::string config_file = "config/localhost-" + ndn::to_string(node_num) + ".yaml";
  
  ndnpaxos::View view(node_id, config_file);
  view.print_host_nodes();
  ndnpaxos::CP cp(view);

// if it is consumer/proposer others no need 
  if (argc > 2) {
    int to_id = std::stoi(argv[2]);
    google::protobuf::Message *msg = cp.generate_msg("Lijing Init!!"); 
    switch(to_id) {
      case -1: {
        cp.broadcast_msg(msg, ndnpaxos::MsgType::ACCEPT);          
        break;
      }
      default: {
        cp.send_one_msg(msg, ndnpaxos::MsgType::ACCEPT, to_id);
      }
    }
  }
  cp.io_run();
//  cp.waiting_msg();
  return 0;
}
