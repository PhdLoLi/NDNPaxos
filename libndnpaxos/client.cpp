/**
 * Created on Mar 28, 2016
 * @author Lijing Wang OoOfreedom@gmail.com
 */


#include "client.hpp"

namespace ndnpaxos {

Client::Client(ndn::Name prefix, int commit_win, int ratio) 
 : prefix_(prefix), com_win_(commit_win), ratio_(ratio),
   commit_counter_(0), thr_counter_(0), starts_(20000000),
   recording_(false), done_(false) {

  prefix_.append("commit");
  face_ = ndn::make_shared<ndn::Face>();
  boost::thread listen(boost::bind(&Client::attach, this));
}

Client::~Client() {
  face_->shutdown();
}

void Client::attach() {
  LOG_INFO("Client Attached!");
  face_->processEvents();
  LOG_INFO("Client Attach Finished!");
}

void Client::stop() {
  face_->shutdown();
}

void Client::start_commit() {

  int warming = 2;
  int interval = 3;
  for (int i = 0; i < warming; i++) {
    LOG_INFO("Warming Counting %d", i + 1);
    sleep(1);
  }

  for (int i = 0; i < com_win_; i++) {
    counter_mut_.lock();
    starts_[commit_counter_] = std::chrono::high_resolution_clock::now(); 
    std::string value = "Commiting Value Time_" + std::to_string(commit_counter_) + " from " + "client_" + std::to_string(i);
    LOG_INFO(" +++++++++++ ZERO Init Commit Value: %s +++++++++++", value.c_str());
    // interest format /prefix/commit/write_or_read(0 or 1,Number)/client_id(Number)/commit_counter(Number)/value_string
    ndn::Name new_name(prefix_);
    int write_or_read = 0;
    if (ratio_ == 100) {
      new_name.appendNumber(write_or_read).appendNumber(i).appendNumber(commit_counter_).append(value);
    }

    commit_counter_++;
    counter_mut_.unlock();

    consume(new_name);
    
    LOG_INFO(" +++++++++++ ZERO FINISH Commit Value: %s +++++++++++", value.c_str());

  }

  for (int i = 0; i < interval; i++) {
    LOG_INFO("Not Recording Counting %d", i + 1);
    sleep(1);
  }
  LOG_INFO("%d s passed start punching", interval);

  thr_mut_.lock();
  recording_ = true;
  thr_mut_.unlock();

  uint64_t before = 0;
  uint64_t throughput = 0;

  for (int j = 0; j < interval * 4; j++) {
    LOG_INFO("Time %d", j + 1);

    thr_mut_.lock();
    before = thr_counter_;
    thr_mut_.unlock();

    sleep(1);

    thr_mut_.lock();
    throughput = thr_counter_ - before; 

    if (periods_.size() > 0) {
      LOG_INFO("PUNCH!  -- counter:%lu second:1 throughput:%lu latency:%f ms", thr_counter_, throughput, periods_[periods_.size() - 1] / 1000.0);
    }
    else {
      LOG_INFO("PUNCH! -- counter:%lu second:1 throughput:%lu periods_.size() == 0", thr_counter_, throughput);
    }

    thr_mut_.unlock();
    throughputs_.push_back(throughput);
  }
  
  thr_mut_.lock();
  recording_ = false;
  done_ = true;
  thr_mut_.unlock();

  LOG_INFO("Last %d s period", interval);
  for (int i = interval; i > 0; i--) {
    LOG_INFO("Stop Committing Counting %d", i);
    sleep(1);
  }

//  std::ofstream file_throughput_;
//  std::ofstream file_latency_;
//  std::ofstream file_trytime_;
//  std::string thr_name;
//  std::string lat_name;
//  std::string try_name;
//  
//  LOG_INFO("Writing File Now!");
//  #if MODE_TYPE == 3
//  thr_name = "results/ndnpaxos/Q_t_" + std::to_string(node_num_) + "_" + std::to_string(win_size_) + ".txt";
//  lat_name = "results/ndnpaxos/Q_l_" + std::to_string(node_num_) + "_" + std::to_string(win_size_) + ".txt";
//  try_name = "results/ndnpaxos/Q_r_" + std::to_string(node_num_) + "_" + std::to_string(win_size_) + ".txt";
//  #elif MODE_TYPE == 2
//  thr_name = "results/ndnpaxos/M_t_" + std::to_string(node_num_) + "_" + std::to_string(win_size_) + ".txt";
//  lat_name = "results/ndnpaxos/M_l_" + std::to_string(node_num_) + "_" + std::to_string(win_size_) + ".txt";
//  try_name = "results/ndnpaxos/M_r_" + std::to_string(node_num_) + "_" + std::to_string(win_size_) + ".txt";
//  #else
//  thr_name = "results/ndnpaxos/t_" + std::to_string(node_num_) + "_" + std::to_string(win_size_) + ".txt";
//  lat_name = "results/ndnpaxos/l_" + std::to_string(node_num_) + "_" + std::to_string(win_size_) + ".txt";
//  try_name = "results/ndnpaxos/r_" + std::to_string(node_num_) + "_" + std::to_string(win_size_) + ".txt";
//  #endif
//
//  file_throughput_.open(thr_name);
//
//  file_latency_.open(lat_name);
//
//  file_trytime_.open(try_name);
//
//  for (int i = 0; i < throughputs_.size(); i++) {
//    file_throughput_ << throughputs_[i] << "\n";
//  }
//
//  file_throughput_.close();
//
//  for (int j = 0; j < periods_.size(); j++) {
//    file_latency_ << periods_[j] << "\n";
//  }
//  file_latency_.close();
//
//  for (int j = 0; j < trytimes_.size(); j++) {
//    file_trytime_ << trytimes_[j] << "\n";
//  }
//  file_trytime_.close();

//  LOG_INFO("Writing File Finished!");
  stop();
  
}

void Client::consume(ndn::Name &name) {
  ndn::Interest interest(name);
  interest.setInterestLifetime(ndn::time::milliseconds(1000));
  interest.setMustBeFresh(true);
  face_->expressInterest(interest,
                         bind(&Client::onData, this,  _1, _2),
                         bind(&Client::onTimeout, this, _1));
}
void Client::onTimeout(const ndn::Interest& interest) {
  // do nothing
}

void Client::onData(const ndn::Interest& interest, const ndn::Data& data) {
  // counting
  ndn::Name inName = interest.getName();
  int commit_counter = inName.get(-2).toNumber();
  int client_id = inName.get(-3).toNumber();
  int type = inName.get(-4).toNumber(); 

//  std::cout << "commit_counter: " << commit_counter << std::endl; 
//  std::cout << "client_id: " << client_id << std::endl; 
//  std::cout << "type: " << type << std::endl; 
  thr_mut_.lock();
  if (recording_) {

    const uint8_t* value = data.getContent().value();
    size_t size = data.getContent().value_size();
    std::string value_str(value, value + size);
    int try_time = std::stoi(value_str);
//    std::cout << "try_time: " << try_time << std::endl; 

    auto finish = std::chrono::high_resolution_clock::now();
    periods_.push_back(std::chrono::duration_cast<std::chrono::microseconds>
                      (finish-starts_[commit_counter]).count());
    trytimes_.push_back(try_time);
//     LOG_INFO("periods_.size() : %d thr_counter_ : %d periods_[thr_counter_] :%llu \n", periods_.size(), thr_counter_, periods_[thr_counter_]);
    thr_counter_++;
  }
  thr_mut_.unlock();

  if (done_ == false) {
    counter_mut_.lock();
    std::string value = "Commiting Value Time_" + std::to_string(commit_counter_) + " from " + "client_" + std::to_string(client_id);
//    std::cout << "start commit : " << value << std::endl;
    starts_[commit_counter_] = std::chrono::high_resolution_clock::now();
    ndn::Name new_name(interest.getName().getPrefix(-2));
    new_name.appendNumber(commit_counter_).append(value);
    commit_counter_++;
    counter_mut_.unlock();
    consume(new_name);
  }

}

}  //  namespace ndnpaxos
