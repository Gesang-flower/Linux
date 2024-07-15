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
    return sd_bus_reply_method_return(m, "s", "Create table successfully");
}

static int method_dbus_insert(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    const char *table_name, *value;
    int r;

    r = sd_bus_message_read(m, "ss", &table_name, &value); // Read table_name, key, and value parameters
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

    snprintf(sql, sizeof(sql), "INSERT INTO Messages(Data) VALUES('%s');", value);

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
    return sd_bus_reply_method_return(m, "s", "Data inserted successfully");
}

static int method_dbus_select(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    const char *table_name;
    int r;

    r = sd_bus_message_read(m, "s", &table_name); // Read table_name parameter
    if (r < 0) {
        fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
        return r;
    }

    // Create variables for SQLite
    sqlite3 *db;
    sqlite3_stmt *res;
    int rc;
    const char *sql_template = "SELECT * FROM %s;";
    char sql[256];

    // Open the SQLite database
    rc = sqlite3_open(data_base_name, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -EINVAL; // Return appropriate error code
    }

    // Prepare the SQL statement
    snprintf(sql, sizeof(sql), sql_template, table_name);
    rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare SQL statement: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -EINVAL; // Return appropriate error code
    }

    // Clear response message buffer
    char response_message[1000];
    response_message[0] = '\0';
    int total_length = 0;

    // Fetch rows from the result set
    while (sqlite3_step(res) == SQLITE_ROW) {
        const char *data = (const char *)sqlite3_column_text(res, 1); // Fetch data from the first column
        int len = strlen(data);
        if (total_length + len + 1 > sizeof(response_message)) {
            fprintf(stderr, "Response message buffer too small\n");
            sqlite3_finalize(res);
            sqlite3_close(db);
            return -EINVAL; // Return appropriate error code
        }
        strcat(response_message, data);
        strcat(response_message, "\n");
        total_length += len + 1;
    }

    // Finalize the SQLite statement and close the database
    sqlite3_finalize(res);
    sqlite3_close(db);

    printf("Response message: %s", response_message); // Debug: Print response message


    // Reply with success and return the response message
    return sd_bus_reply_method_return(m, "s", response_message);
}


/* The vtable of our little object, implements the net.poettering.Calculator interface */
static const sd_bus_vtable calculator_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("create_table","s","s", method_create_table,SD_BUS_VTABLE_UNPRIVILEGED),
        SD_BUS_METHOD("dbus_insert","ss","s", method_dbus_insert,SD_BUS_VTABLE_UNPRIVILEGED),
        SD_BUS_METHOD("dbus_select","s","s", method_dbus_select,SD_BUS_VTABLE_UNPRIVILEGED),
        SD_BUS_VTABLE_END
};

int main(int argc, char *argv[]) {
        sd_bus_slot *slot = NULL;
        sd_bus *bus = NULL;
        int r;

        /* Connect to the user bus this time */
        r = sd_bus_open_user(&bus);
        if (r < 0) {
                fprintf(stderr, "Failed to connect to system bus: %s\n", strerror(-r));
                goto finish;
        }

        /* Install the object */
        r = sd_bus_add_object_vtable(bus,
                                     &slot,
                                     "/net/poettering/Calculator",  /* object path */
                                     "net.poettering.Calculator",   /* interface name */
                                     calculator_vtable,
                                     NULL);
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", strerror(-r));
                goto finish;
        }

        /* Take a well-known service name so that clients can find us */
        r = sd_bus_request_name(bus, "net.poettering.Calculator", 0);
        if (r < 0) {
                fprintf(stderr, "Failed to acquire service name: %s\n", strerror(-r));
                goto finish;
        }

        for (;;) {
                /* Process requests */
                r = sd_bus_process(bus, NULL);
                if (r < 0) {
                        fprintf(stderr, "Failed to process bus: %s\n", strerror(-r));
                        goto finish;
                }
                if (r > 0) /* we processed a request, try to process another one, right-away */
                        continue;

                /* Wait for the next request to process */
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
