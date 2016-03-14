/**
 * Created on: Dec 24, 2015
 * @Author: Lijing OoOfreedom@gmail.com
 */

#include "dbmanager.hpp"
#include "view.hpp"
#include "commo.hpp"
#include "captain.hpp"
#include <boost/bind.hpp>

namespace ndnpaxos {

DBManager::DBManager(node_id_t node_id, node_id_t node_num, int win_size)  
  : win_size_(win_size) {
  std::string config_file = "config/localhost-" + std::to_string(node_num) + ".yaml";

  callback_latency_t call_latency = boost::bind(&DBManager::count_latency, this, _1, _2, _3);
  callback_full_t callback_exe_latency = boost::bind(&DBManager::count_exe_latency, this, _1, _2, _3);

  view_ = new View(node_id, config_file);
  captain_ = new Captain(*view_, win_size_);
  captain_->set_callback(call_latency);
  captain_->set_callback(callback_exe_latency);
  commo_ = new Commo(captain_, *view_, 0);
  captain_->set_commo(commo_);
  callback_ = callback_db; 

  /* Open database */
  LOG_INFO("database name: %s", view_->db_name().c_str());
  int rc = sqlite3_open(view_->db_name().c_str(), &db_);
  if (rc) {
    LOG_INFO("Can't open database: %s", sqlite3_errmsg(db_));
  } else {
    LOG_INFO("Open database Successfully");
  }
}

DBManager::~DBManager() {
  sqlite3_close(db_);
}

void DBManager::commit(std::string &sql) {
  captain_->commit(sql);
}

void DBManager::set_callback(callback_db_t& cb) {
  callback_ = cb;
}

void DBManager::count_exe_latency(slot_id_t slot_id, PropValue& prop_value, node_id_t node_id) {
  LOG_INFO("exe_latency slot_id:%llu value_id:%llu value:%s", 
          slot_id, prop_value.id(), prop_value.data().c_str());
  char *zErrMsg = 0;
  /* Execute SQL statement */
  int rc = sqlite3_exec(db_, prop_value.data().c_str(), callback_, 0, &zErrMsg);
  if (rc != SQLITE_OK) {
    LOG_INFO("SQL error: %s", zErrMsg);
    sqlite3_free(zErrMsg);
  } else {
    LOG_INFO("Table created successfully");
  }
}

void DBManager::count_latency(slot_id_t slot_id, PropValue& prop_value, int try_time) {
}

int DBManager::callback_db(void *NotUsed, int argc, char **argv, char **azColName) {
  int i;
  for(i = 0; i < argc; i++) {
     printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");
  return 0;
}

void DBManager::start() {
  commo_->start();  
}

} // namespace ndnpaxos 
