/**
 * Created on Dec 06, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include "view.hpp"

namespace ndnpaxos {

class Producer {
 public:
   void run(std::string node_name) {

     int node_num = 3;
     std::string config_file = "config/localhost-" + std::to_string(node_num) + ".yaml";
     int my_id = 0;
   
     // init view for one captain
     View view(my_id, config_file);
     view.print_host_nodes();

     ndn::Name prefix(view.prefix());
     prefix.append(std::to_string(view.whoami()));
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

     ballot_id_t ballot_id = dataName.get(-1).toNumber();
     slot_id_t slot_id = dataName.get(-2).toNumber();
     MsgType msg_type = MsgType(std::stoi(dataName.get(-3).toUri()));
     node_id_t node_id = std::stoi(dataName.get(-4).toUri());
//     std::string node_id = dataName.get(-4).toUri();
     
     std::cout << "node_id: " << node_id << std::endl;
     std::cout << "msg_type: " << msg_type_str[msg_type] << std::endl;
     std::cout << "slot_id: " << slot_id << std::endl;
     std::cout << "ballot_id: " << ballot_id << std::endl;

     value_id_t value_id = (1 << 16) + 2;
     dataName
//       .append(m_nodeName) // add node_name component to Interest name
       .appendNumber(value_id)
       .appendNumber(2) // add "max_ballot_id" component to Interest name
       .append("T"); // add "reply" component to Interest name
//       .appendVersion();  // add "version" component (current UNIX timestamp in milliseconds)
 
     static const std::string content = "HELLO KITTY";
 
     // Create Data packet
     ndn::shared_ptr<ndn::Data> data = ndn::make_shared<ndn::Data>();
     data->setName(dataName);
     data->setFreshnessPeriod(ndn::time::seconds(10));
     data->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());
 
     // Sign Data packet with default identity
     m_keyChain.sign(*data);
     // m_keyChain.sign(data, <identityName>);
     // m_keyChain.sign(data, <certificate>);
 
     // Return Data packet to the requester
     std::cout << ">> D: " << *data << std::endl;
     m_face.put(*data);
   }
 

  void
  onRegisterFailed(const ndn::Name& prefix, const std::string& reason)
  {
    std::cerr << "ERROR: Failed to register prefix \""
              << prefix << "\" in local hub's daemon (" << reason << ")"
              << std::endl;
    m_face.shutdown();
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
  std::string node_name = "node1";
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
