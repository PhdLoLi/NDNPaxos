#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h> 
#include <string>
#include "internal_types.hpp"

int callback(void *NotUsed, int argc, char **argv, char **azColName) {
  int i;
  for(i = 0; i < argc; i++) {
     printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");
  return 0;
}

int main(int argc, char* argv[]) {
  sqlite3 *db;
  char *zErrMsg = 0;
  int  rc;

  /* Open database */
  rc = sqlite3_open("test.db", &db);
  if (rc) {
    LOG_INFO("Can't open database: %s", sqlite3_errmsg(db));
    return 0;
  } else {
    LOG_INFO("Open database Successfully");
  }

  /* Create SQL statement */
  std::string sql = "CREATE TABLE COMPANY("  \
        "ID INT PRIMARY KEY     NOT NULL," \
        "NAME           TEXT    NOT NULL," \
        "AGE            INT     NOT NULL," \
        "ADDRESS        CHAR(50)," \
        "SALARY         REAL );";

  /* Execute SQL statement */
  rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
  if (rc != SQLITE_OK) {
    LOG_INFO("SQL error: %s", zErrMsg);
    sqlite3_free(zErrMsg);
  } else {
    LOG_INFO("Table created successfully");
  }
  sqlite3_close(db);
  return 0;
}
