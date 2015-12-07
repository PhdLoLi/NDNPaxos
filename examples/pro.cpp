/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#include <ndn-cxx/face.hpp>

namespace ndn {

namespace examples {

class Consumer {
 public:
   void run() {
     Name name("/ndn/thu/paxos/node1/prepare");
     name.appendNumber(0).appendNumber(1); // slot_id
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
     std::cout << data << std::endl;
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
