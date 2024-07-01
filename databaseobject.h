#ifndef DATABASEOBJECT_H
#define DATABASEOBJECT_H

// 注册DBus服务
void register_dbus_service();

// 发送插入数据请求
void send_insert_request(const char *database_name, const char *data);

#endif // DATABASE_DBUS_H
