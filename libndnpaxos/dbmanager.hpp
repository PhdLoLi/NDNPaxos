/**
 * Created on: Dec 24, 2015
 * @Author: Lijing OoOfreedom@gmail.com 
 */

#pragma once
#include "internal_types.hpp"
#include <sqlite3.h> 

namespace ndnpaxos {
class View;
class Commo;
class Captain;
class DBManager {
 public:
  DBManager(node_id_t node_id, node_id_t node_num, int win_size = 1);
  ~DBManager(); 
  void start();
  void commit(std::string &sql);
  /**
   * set_callback from outside
   */
  void set_callback(callback_db_t& cb);

 private:
  View *view_;
  Commo *commo_;
  Captain *captain_;
  sqlite3 *db_;
  callback_db_t callback_;
  int win_size_;
  void count_exe_latency(slot_id_t slot_id, PropValue& prop_value, node_id_t node_id);
  void count_latency(slot_id_t slot_id, PropValue& prop_value, int try_time);
  static int callback_db(void *NotUsed, int argc, char **argv, char **azColName);
}; 

} // namespace ndnpaxos 
