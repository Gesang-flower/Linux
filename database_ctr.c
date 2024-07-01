#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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


int database_select(const char *data_base_name, char *response_message, int max_size) {
    sqlite3 *db;
    sqlite3_stmt *res;
    int rc;
    const char *sql = "SELECT * FROM Messages;";

    rc = sqlite3_open(data_base_name, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    // 清空响应消息缓冲区
    response_message[0] = '\0';
    int total_length = 0;
    while (sqlite3_step(res) == SQLITE_ROW) {
        const char *data = (const char *)sqlite3_column_text(res, 1);
        int len = strlen(data);
        if (total_length + len + 1 > max_size) {
            fprintf(stderr, "Response message buffer too small\n");
            sqlite3_finalize(res);
            sqlite3_close(db);
            return 1;
        }
        strcat(response_message, data);
        strcat(response_message, "\n");
        total_length += len + 1;
    }

    sqlite3_finalize(res);
    sqlite3_close(db);
    return 0;
}

