#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sqlite3.h>
#include <systemd/sd-bus.h>

const char *data_base_name = "database.db";

static int method_create_table(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    const char *table_name;
    int r;

    r = sd_bus_message_read(m, "s", &table_name); // Read table_name parameter
    if (r < 0) {
        fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
        return r;
    }

    // create a database file
    sqlite3 *db;
    char *err_msg = 0;
    const char *sql = "CREATE TABLE IF NOT EXISTS Messages (ID INTEGER PRIMARY KEY AUTOINCREMENT, Data TEXT);";

    int rc = sqlite3_open(data_base_name, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -EINVAL; // Return appropriate error code
    }

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -EINVAL; // Return appropriate error code
    }

    sqlite3_close(db);
    // Reply with success
    return sd_bus_reply_method_return(m, "");
}

static int method_dbus_insert(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    const char *table_name, *key, *value;
    int r;

    r = sd_bus_message_read(m, "sss", &table_name, &key, &value); // Read table_name, key, and value parameters
    if (r < 0) {
        fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
        return r;
    }

    //insert data into database
    sqlite3 *db;
    char *err_msg = 0;
    char sql[256];

    int rc = sqlite3_open(data_base_name, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -EINVAL; // Return appropriate error code
    } else {
        printf("Database opened successfully\n");
    }

    snprintf(sql, sizeof(sql), "INSERT INTO Messages(Data) VALUES('%s', '%s');", table_name, value);
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -EINVAL; // Return appropriate error code
    } else {
        printf("Data inserted successfully\n");
    }

    sqlite3_close(db);

    // Reply with success
    return sd_bus_reply_method_return(m, "");
}

static int method_dbus_select(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    const char *table_name, *key;
    int r;

    r = sd_bus_message_read(m, "ss", &table_name, &key); // Read table_name and key parameters
    if (r < 0) {
        fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
        return r;
    }

    // In a real scenario, perform database select operation from table_name using key and obtain value
    sqlite3 *db;
    sqlite3_stmt *res;
    int rc;
    char sql[256];
    char response_message[1024];
    response_message[0] = '\0'; // Initialize response buffer
    int total_length = 0;

    snprintf(sql, sizeof(sql), "SELECT Data FROM %s WHERE Key = ?;", table_name);

    rc = sqlite3_open(data_base_name, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -EINVAL; // Return appropriate error code
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -EINVAL; // Return appropriate error code
    }

    sqlite3_bind_text(res, 1, key, -1, SQLITE_STATIC);

    while (sqlite3_step(res) == SQLITE_ROW) {
        const char *data = (const char *)sqlite3_column_text(res, 0);
        int len = strlen(data);
        if (total_length + len + 1 > 1000) {
            fprintf(stderr, "Response message buffer too small\n");
            sqlite3_finalize(res);
            sqlite3_close(db);
            return -EINVAL; // Return appropriate error code
        }
        strcat(response_message, data);
        strcat(response_message, "\n");
        total_length += len + 1;
    }

    sqlite3_finalize(res);
    sqlite3_close(db);

    return sd_bus_reply_method_return(m, "s", response_message);
}

// Define the D-Bus vtable with the methods
static const sd_bus_vtable database_vtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("create_table", "s", "", method_create_table, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("dbus_insert", "sss", "", method_dbus_insert, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("dbus_select", "ss", "s", method_dbus_select, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_VTABLE_END
};

int main(int argc, char *argv[]) {
    sd_bus_slot *slot = NULL;
    sd_bus *bus = NULL;
    int r;

    // Connect to the system bus
    r = sd_bus_open_system(&bus);
    if (r < 0) {
        fprintf(stderr, "Failed to connect to system bus: %s\n", strerror(-r));
        goto finish;
    }

    // Add the object to the bus
    r = sd_bus_add_object_vtable(bus,
                                 &slot,
                                 "/com/example/Database",  /* object path */
                                 "com.example.Database",
                                 database_vtable,
                                 NULL);
    if (r < 0) {
        fprintf(stderr, "Failed to add object to bus: %s\n", strerror(-r));
        goto finish;
    }

    // Request a well-known service name
    r = sd_bus_request_name(bus, "com.example.Database", 0);
    if (r < 0) {
        fprintf(stderr, "Failed to acquire service name: %s\n", strerror(-r));
        goto finish;
    }

    // Main loop: process requests
    for (;;) {
        r = sd_bus_process(bus, NULL);
        if (r < 0) {
            fprintf(stderr, "Failed to process bus: %s\n", strerror(-r));
            goto finish;
        }
        if (r > 0) // Processed a request, continue to process another one right away
            continue;

        // Wait for the next request to process
        r = sd_bus_wait(bus, (uint64_t) -1);
        if (r < 0) {
            fprintf(stderr, "Failed to wait on bus: %s\n", strerror(-r));
            goto finish;
        }
    }

finish:
    sd_bus_slot_unref(slot);
    sd_bus_unref(bus);

    return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
