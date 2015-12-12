/**
 * Created on Dec 10, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#include <ndn-cxx/face.hpp>
#include <google/protobuf/text_format.h>
#include "view.hpp"

namespace ndnpaxos {

class Consumer {
 public:
   void run() {
     int node_num = 3;
     std::string config_file = "config/localhost-" + std::to_string(node_num) + ".yaml";
     int my_id = 1;
   
     // init view for one captain
     View view(my_id, config_file);
     view.print_host_nodes();

     ndn::Name name(view.prefix());
     name.append("node1");

     // set Msg
     MsgAccept *msg_acc = new MsgAccept();
     MsgHeader *msg_header = new MsgHeader();
     msg_header->set_msg_type(MsgType::ACCEPT);
     msg_header->set_node_id(view.whoami());
     msg_header->set_slot_id(0);

     msg_acc->set_allocated_msg_header(msg_header);
     msg_acc->set_ballot_id((1<<16) + 1);
     PropValue *rs_value = new PropValue();
     rs_value->set_id(2);
     rs_value->set_data("I am Lijing Wang!!");
     msg_acc->set_allocated_prop_value(rs_value);

     std::string msg_str;
     msg_acc->SerializeToString(&msg_str);


     google::protobuf::Message *msg = new MsgAccept();
     msg->ParseFromString(msg_str);
     std::string text_str;
     google::protobuf::TextFormat::PrintToString(*msg, &text_str);
     LOG_INFO_COM("Sending Message:\n %s", text_str.c_str());

     std::cout << "msg_str size: " << msg_str.size() << std::endl;

     msg_str.append(std::to_string(MsgType::ACCEPT));

     ndn::name::Component message(reinterpret_cast<const uint8_t*>(msg_str.c_str()), msg_str.size());
     name.append(message);

     ndn::Interest interest(name);
     interest.setInterestLifetime(ndn::time::milliseconds(1000));
     interest.setMustBeFresh(true);
     m_face.expressInterest(interest,
                            bind(&Consumer::onData, this,  _1, _2),
                            bind(&Consumer::onTimeout, this, _1));
     std::cout << "Sending " << interest << std::endl;
     // processEvents will block until the requested data received or timeout occurs
     m_face.processEvents();
  }

 private:
  void onData(const ndn::Interest& interest, const ndn::Data& data) {
    const uint8_t* value = data.getContent().value();
    size_t size = data.getContent().value_size();
    std::string value_str(value, value + size);
    std::cout << "value_str: " << value_str << std::endl;
    std::cout << "value_size: " << size << std::endl;
    std::cout << "interest name: " << interest.getName() << std::endl;
    std::cout << "data name: " << data.getName() << std::endl;
  }

  void onTimeout(const ndn::Interest& interest) {
    std::cout << "Timeout " << interest << std::endl;
  }

 private:
   ndn::Face m_face;
};

} // namespace ndnpaxos

int main(int argc, char** argv) {
  ndnpaxos::Consumer consumer;
  try {
    consumer.run();
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return 0;
}
