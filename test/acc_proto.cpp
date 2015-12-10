/**
 * Created on Dec 10, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <google/protobuf/text_format.h>
#include "view.hpp"

namespace ndnpaxos {

class Producer {
 public:
   void run(std::string node_name) {

     int node_num = 3;
     std::string config_file = "config/localhost-" + ndn::to_string(node_num) + ".yaml";
     int my_id = 0;
   
     // init view for one captain
     View view(my_id, config_file);
     view.print_host_nodes();

     ndn::Name prefix(view.prefix());
     prefix.append(view.hostname());
     m_face.setInterestFilter(prefix,
                              bind(&Producer::onInterest, this, _1, _2),
                              ndn::RegisterPrefixSuccessCallback(),
                              bind(&Producer::onRegisterFailed, this, _1, _2));
     m_nodeName = node_name;
     m_face.processEvents();
   }

 private:
   void onInterest(const ndn::InterestFilter& filter, const ndn::Interest& interest) {
     std::cout << "<< I: " << interest << std::endl;
 
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
     m_keyChain.sign(*data);
 
     // Return Data packet to the requester
     std::cout << ">> D: " << *data << std::endl;
     m_face.put(*data);
   }
 

  void
  onRegisterFailed(const ndn::Name& prefix, const std::string& reason) {
    std::cerr << "ERROR: Failed to register prefix \""
              << prefix << "\" in local hub's daemon (" << reason << ")"
              << std::endl;
    m_face.shutdown();
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
}

private:
  ndn::Face m_face;
  ndn::KeyChain m_keyChain;
  std::string m_nodeName;
};

} // namespace ndnpaxos

int
main(int argc, char** argv)
{
  ndnpaxos::Producer producer;
  std::string node_name = "node2";
  try {
    if (argc > 1)
      node_name = argv[1];

    producer.run(node_name);
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return 0;
}
