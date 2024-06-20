#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>


int create_table(const char *data_base_name) {
    sqlite3 *db;
    char *err_msg = 0;
    const char *sql = "CREATE TABLE IF NOT EXISTS Messages (ID INTEGER PRIMARY KEY AUTOINCREMENT, Data TEXT);";

    int rc = sqlite3_open(data_base_name, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    sqlite3_close(db);
    return 0;
}


int database_insert(const char *data_base_name, const char *data) {
    sqlite3 *db;
    char *err_msg = 0;
    char sql[256];

    int rc = sqlite3_open(data_base_name, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    } else {
        printf("Database opened successfully\n");
    }

    snprintf(sql, sizeof(sql), "INSERT INTO Messages(Data) VALUES('%s');", data);
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    } else {
        printf("Data inserted successfully\n");
    }

    sqlite3_close(db);
    return 0;
}


int database_select(const char *data_base_name)
{
    sqlite3 *db;
    sqlite3_stmt *res;
    int rc,i=0;
    const char *sql = "SELECT * FROM Messages;";

    rc = sqlite3_open(data_base_name,&db);
    if(rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
    if(rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    while(sqlite3_step(res) ==  SQLITE_ROW)
    {
        printf("%s\n", sqlite3_column_text(res, 1));
        printf("%s\n",i);
        i++;//第几次插入
    }
    sqlite3_finalize(res);
    sqlite3_close(db);
    return 0;
}

int main() //测试
{
    const char *data_base_name = "test.db";//后续需要改文件名
    const char *data = "Hello, World!";//测试


    // 创建表
    if (create_table(data_base_name) != 0) {
        fprintf(stderr, "Failed to create table\n");
        return 1;
    }
    // 插入数据到数据库
    if (database_insert(data_base_name, data) != 0) {
        fprintf(stderr, "Failed to insert data\n");
        return 1;
    }

    // 从数据库中选择数据
    if (database_select(data_base_name) != 0) {
        fprintf(stderr, "Failed to select data\n");
        return 1;
    }

    return 0;
}
