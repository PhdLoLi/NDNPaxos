/**
 * Created on Mar 29, 2016
 * @author Lijing Wang OoOfreedom@gmail.com
 */

#include "client.hpp"

namespace ndnpaxos {

int main(int argc, char** argv) {

  if (argc < 2) {
    std::cerr << "Usage:Window_Size " << std::endl;
    return 0;
  }

  ndn::Name prefix("/ndn/thu/naxos");
  int win_size = std::stoi(argv[1]);
  Client client(prefix, win_size);
  client.start_commit();
  return 0;
}

} // ndnpaxos

int main(int argc, char** argv) {
  return ndnpaxos::main(argc, argv);
}
