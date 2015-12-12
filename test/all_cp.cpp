/**
 * Created on Dec 11, 2015
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#include "producer.hpp"
#include "consumer.hpp"
#include "view.hpp"
#include "threadpool.hpp" 
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>

#include <unistd.h>
#include <google/protobuf/text_format.h>
#include <string>
#include <stdlib.h>


using namespace boost::threadpool;

int main(int argc, char** argv) {

  if (argc < 3) {
    std::cout << "Usage: my_node_id to_node_id" << std::endl;
    return 0;
  }

  int node_id = std::stoi(argv[1]);
  int node_num = 3;
  std::string config_file = "config/localhost-" + ndn::to_string(node_num) + ".yaml";
  
  ndnpaxos::View view(node_id, config_file);
  
  ndn::Name prefix("haha");
  prefix.append(view.hostname());

  ndnpaxos::Producer producer(prefix);
  producer.attach();

  ndnpaxos::Consumer consumer;

  int to_id = std::stoi(argv[2]);
  ndn::Name to_name("haha");
  to_name.append(view.hostname(to_id));

  while (1) {
    sleep(5);
    consumer.consume(to_name.appendNumber(rand()));
  }

//  cp.waiting_msg();
  return 0;
}
