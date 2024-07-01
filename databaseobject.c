#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>
#include "database_ctr.h" // 包含数据库操作头文件

// DBus方法处理函数
static DBusHandlerResult handle_method_call(DBusConnection *conn, DBusMessage *msg, void *user_data) {
    const char *method_name = dbus_message_get_member(msg);

    if (strcmp(method_name, "database_insert") == 0) {
        // 解析消息参数
        DBusError err;
        dbus_error_init(&err);
        const char *database_name;
        const char *data;

        // 检索参数
        if (!dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &database_name,
                                   DBUS_TYPE_STRING, &data, DBUS_TYPE_INVALID)) {
            fprintf(stderr, "Failed to parse D-Bus message arguments: %s\n", err.message);
            dbus_error_free(&err);
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

        // 调用数据库插入函数
        int result = database_insert(database_name, data);
        if (result != 0) {
            fprintf(stderr, "Failed to insert data into database\n");
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

        // 构造成功的回复消息
        DBusMessage *reply = dbus_message_new_method_return(msg);
        if (!reply) {
            fprintf(stderr, "Failed to create D-Bus reply message\n");
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

        // 发送回复消息
        if (!dbus_connection_send(conn, reply, NULL)) {
            fprintf(stderr, "Failed to send D-Bus reply message\n");
            dbus_message_unref(reply);
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

        // 释放消息资源
        dbus_message_unref(reply);

        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

// 注册DBus对象和方法
void register_dbus_service() {
    DBusConnection *conn;
    DBusError err;
    dbus_error_init(&err);

    // 连接到系统DBus
    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "DBus connection error: %s\n", err.message);
        dbus_error_free(&err);
        return;
    }
    if (conn == NULL) {
        fprintf(stderr, "DBus connection is NULL\n");
        return;
    }

    // 注册对象路径和方法处理函数
    DBusObjectPathVTable vtable = {
        .message_function = handle_method_call,
    };
    dbus_connection_register_object_path(conn, "/com/example/DatabaseObject", &vtable, NULL);

    // 进入DBus事件循环
    while (dbus_connection_read_write_dispatch(conn, -1)) {
        // 等待DBus消息
    }

    // 关闭DBus连接
    dbus_connection_close(conn);
}

// 发送插入数据请求
void send_insert_request(const char *database_name, const char *data) {
    DBusConnection *conn;
    DBusError err;
    DBusMessage *msg;
    DBusMessage *reply;
    const char *service_name = "com.example.DatabaseService";
    const char *object_path = "/com/example/DatabaseObject";
    const char *interface_name = "com.example.DatabaseInterface";
    const char *insert_method_name = "database_insert";

    // 初始化DBus错误结构
    dbus_error_init(&err);

    // 连接到系统DBus
    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "DBus connection error: %s\n", err.message);
        dbus_error_free(&err);
        return;
    }
    if (conn == NULL) {
        fprintf(stderr, "DBus connection is NULL\n");
        return;
    }

    // 创建一个方法调用消息（database_insert）
    msg = dbus_message_new_method_call(
        service_name,      // 目标服务名称
        object_path,       // 对象路径
        interface_name,    // 接口名称
        insert_method_name // 方法名称（插入数据）
    );
    if (!msg) {
        fprintf(stderr, "Failed to create D-Bus message\n");
        return;
    }

    // 添加方法参数到消息中
    dbus_message_append_args(msg, DBUS_TYPE_STRING, &database_name,
                             DBUS_TYPE_STRING, &data, DBUS_TYPE_INVALID);

    // 发送消息并等待回复
    reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "DBus call failed: %s\n", err.message);
        dbus_error_free(&err);
        return;
    }

    // 释放消息资源
    dbus_message_unref(msg);
    dbus_message_unref(reply);

    // 关闭DBus连接
    dbus_connection_close(conn);
}