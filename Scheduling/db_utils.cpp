#include "db_utils.h"
#include <string>
#include <iostream>
#include <assert.h>

using namespace std;

//-------------------------------------
//-------------------------------------
//DB CONNECTION/DISCONNECTION/QUERIES PARSERS
int connect_to_db(const database_settings* db_settings, database_connection *conn){
    //Return Statuses
    //1: All Good
    //0: Connection Failed
    //-1: Error - Not handled for now

    //Initiate connection to database
    cout << "SQL Report: client version: "<<mysql_get_client_info()<<endl;
    mysql_init(&conn->m);
    if (!mysql_real_connect(&conn->m, db_settings->hostname.c_str(),
                            db_settings->user.c_str(), db_settings->pass.c_str(),
                            db_settings->db_name.c_str(), db_settings->port, nullptr, 0)){
        cerr << mysql_error(&conn->m) << endl;
        printf("Connection failed\n");
        return 0;
    }

    printf("Connection Successful\n");
    return 1;
}


void disconnect_from_db(database_connection *conn){
    mysql_close(&conn->m);
}

void free_result(database_connection *conn){
    mysql_free_result(conn->result);
}

void disconnect_from_db(MYSQL m){
    mysql_close(&m);
    //Cleanup mysql elements
}


void execute_query(MYSQL *m, const string& query, const string& success_string)
{
    if(mysql_query(m, query.c_str()) == 0) cout << success_string << endl;
    else {
        cerr << "Query failed: "<<  mysql_error(m) << endl;
        cout << query;
        assert(false);
    }
}

void exec_query(database_connection& db_conn, const char *query){
    if(!(mysql_query(&db_conn.m, query) == 0)) cout << "Query Unsuccessfull" << endl;

    db_conn.result = mysql_store_result(&db_conn.m);
    if (db_conn.result == NULL){
        printf("%d %s\n", mysql_errno(&db_conn.m), mysql_error(&db_conn.m));
        assert(db_conn.result != NULL);
    }
}

void execute_query(database_connection& db_conn, const string& query, const string& success_string)
{
    if(mysql_query(&db_conn.m, query.c_str()) == 0){
        cout << success_string << endl;
        db_conn.result = mysql_store_result(&db_conn.m);
    }
    else {
        printf("%d %s\n", mysql_errno(&db_conn.m), mysql_error(&db_conn.m));
        cerr << "Query failed: "<<  mysql_error(&db_conn.m) << endl;
        cout << query;
        assert(false);
    }
}
