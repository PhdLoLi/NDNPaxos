/**
 * Created on Dec 06, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#include <ndn-cxx/face.hpp>
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
     name.append(std::to_string(0)).append(std::to_string(PREPARE));
     name.appendNumber(0).appendNumber((1 << 16) + 1); // slot_id

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
     value_id_t value_id = data.getName().get(-3).toNumber();
     if (size > 0) {
       LOG_INFO("Has data, value_id is %llu", value_id);
     } else {
       LOG_INFO("Has no data!");
     }
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
