/**
 * Created on Dec 12, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */
#include "consumer.hpp" 
#include "internal_types.hpp"

namespace ndnpaxos {

Consumer::Consumer() {
  face_ = ndn::make_shared<ndn::Face>();
}

Consumer::Consumer(ndn::Name prefix)
  : prefix_(prefix) {
  face_ = ndn::make_shared<ndn::Face>();
}

Consumer::Consumer(ndn::shared_ptr<ndn::Face> face)
  : face_(face){
}

void Consumer::consume(ndn::Name name) {
  ndn::Interest interest(name);
  interest.setInterestLifetime(ndn::time::milliseconds(1000));
  interest.setMustBeFresh(true);
  face_->expressInterest(interest,
                         bind(&Consumer::onData, this,  _1, _2),
                         bind(&Consumer::onTimeout, this, _1));
  LOG_TRACE_COM("Consumer Sending %s", interest.getName().toUri().c_str());
  // processEvents will block until the requested data received or timeout occurs
//  face_->processEvents();
}

void Consumer::onData(const ndn::Interest& interest, const ndn::Data& data) {
  const uint8_t* value = data.getContent().value();
  size_t size = data.getContent().value_size();
  std::string value_str(value, value + size);
  LOG_TRACE_COM("Consumer onData ACK: %s", value_str.c_str());
//  std::cout << "value_str: " << value_str << std::endl;
//  std::cout << "value_size: " << size << std::endl;
//  std::cout << "interest name: " << interest.getName() << std::endl;
//  std::cout << "data name: " << data.getName() << std::endl;
}

void Consumer::onTimeout(const ndn::Interest& interest) {
  LOG_DEBUG_COM("Consumer Timeout %s", interest.getName().toUri().c_str());
}

} // namespace ndnpaxos 

