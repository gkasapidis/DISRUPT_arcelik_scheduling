//
// Created by gregkwaste on 8/30/19.
//

#ifndef SCHEDULING_DB_UTILS_H
#define SCHEDULING_DB_UTILS_H

#include <string>
#include <sstream>
#include <mysql/mysql.h>

//DB Settings Struct
struct database_settings {
    std::string hostname;
    std::string user;
    std::string pass;
    unsigned int port;
    std::string db_name;
};

//DB Connection Interface
struct database_connection {
    MYSQL m;
    MYSQL_RES *result;
};

int connect_to_db(const database_settings* db_settings, database_connection *conn);
void execute_query(MYSQL *m, const std::string& query, const std::string& success_string);
void exec_query(database_connection& db_conn, const char *query);
void free_result(database_connection *conn);
void disconnect_from_db(database_connection *conn);

#endif //SCHEDULING_DB_UTILS_H
