/**
 * Created on Dec 06, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#include <ndn-cxx/face.hpp>
#include "view.hpp"

namespace ndn {

namespace examples {

class Consumer {
 public:
   void run() {
     int node_num = 3;
     std::string config_file = "config/localhost-" + to_string(node_num) + ".yaml";
     int my_id = 1;
   
     // init view for one captain
     ndnpaxos::View view(my_id, config_file);
     view.print_host_nodes();

     Name name(view.prefix());
     name.append(std::to_string(0)).append(std::to_string(ndnpaxos::PREPARE));
     name.appendNumber(0).appendNumber((1 << 16) + 1); // slot_id

     Interest interest(name);
     interest.setInterestLifetime(time::milliseconds(1000));
     interest.setMustBeFresh(true);
     m_face.expressInterest(interest,
                            bind(&Consumer::onData, this,  _1, _2),
                            bind(&Consumer::onTimeout, this, _1));
     std::cout << "Sending " << interest << std::endl;
     // processEvents will block until the requested data received or timeout occurs
     m_face.processEvents();
  }

 private:
   void onData(const Interest& interest, const Data& data) {
     const uint8_t* value = data.getContent().value();
     size_t size = data.getContent().value_size();
     std::string value_str(value, value + size);
     std::cout << "value_str: " << value_str << std::endl;
     std::cout << "value_size: " << size << std::endl;
     std::cout << "interest name: " << interest.getName() << std::endl;
     std::cout << "data name: " << data.getName() << std::endl;
     ndnpaxos::value_id_t value_id = data.getName().get(-3).toNumber();
     if (size > 0) {
       LOG_INFO("Has data, value_id is %llu", value_id);
     } else {
       LOG_INFO("Has no data!");
     }
   }
 
   void onTimeout(const Interest& interest) {
     std::cout << "Timeout " << interest << std::endl;
   }

 private:
   Face m_face;
};

} // namespace examples
} // namespace ndn

int main(int argc, char** argv) {
  ndn::examples::Consumer consumer;
  try {
    consumer.run();
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return 0;
}
