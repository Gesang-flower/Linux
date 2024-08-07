#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <systemd/sd-bus.h>

#define PORT 8080
#define MAX_CLIENTS 10

// Declare sd_bus object globally
sd_bus *bus = NULL;

/********/
static void print_error(const char *prefix, int r) {
    fprintf(stderr, "%s: %s\n", prefix, strerror(-r));
}

static void print_reply(sd_bus_message *reply) {
    const char *response_message;
    int r = sd_bus_message_read(reply, "s", &response_message);
    if (r < 0) {
        fprintf(stderr, "Failed to parse reply: %s\n", strerror(-r));
    } else {
        printf("Response message: %s\n", response_message);
    }
}

void call_create_table(sd_bus *bus, const char *table_name) {
    sd_bus_message *reply = NULL;
    int r;

    r = sd_bus_call_method(bus,
                           "net.poettering.Calculator",       // Service name
                           "/net/poettering/Calculator", // Object path
                           "net.poettering.Calculator",       // Interface name
                           "create_table",                // Method name
                           NULL,
                           &reply,
                           "s",                          // Input signature
                           table_name);                  // Input parameter

    if (r < 0) {
        print_error("Failed to call CreateTable method", r);
    } else {
        print_reply(reply);
        sd_bus_message_unref(reply);
    }
}

void call_dbus_insert(sd_bus *bus, const char *table_name, const char *value) {
    sd_bus_message *reply = NULL;
    int r;

    r = sd_bus_call_method(bus,
                           "net.poettering.Calculator",       // Service name
                           "/net/poettering/Calculator", // Object path
                           "net.poettering.Calculator",       // Interface name
                           "dbus_insert",
                           NULL,
                           &reply,
                           "ss",
                           table_name,
                           value);

    if (r < 0) {
        print_error("Failed to call DBusInsert method", r);
    } else {
        print_reply(reply);
        sd_bus_message_unref(reply);
    }
}

void call_dbus_select(sd_bus *bus, const char *table_name) {
    sd_bus_message *reply = NULL;
    int r;

    r = sd_bus_call_method(bus,
                           "net.poettering.Calculator",       // Service name
                           "/net/poettering/Calculator", // Object path
                           "net.poettering.Calculator",       // Interface name
                           "dbus_select",
                           NULL,
                           &reply,
                           "s",
                           table_name);

    if (r < 0) {
        print_error("Failed to call DBusSelect method", r);
    } else {
        print_reply(reply);
        sd_bus_message_unref(reply);
    }
}
/********/

// 客户端处理函数
void *handle_client(void *socket_desc) {
    int sock = *(int *)socket_desc;
    free(socket_desc); // 释放内存
    char client_message[2000]; // 缓冲区用于存储客户端消息
    int read_size;

    // 接收客户端消息
    while ((read_size = recv(sock, client_message, 2000, 0)) > 0) {
        client_message[read_size] = '\0';
        printf("Received from client: %s\n", client_message);

        // 将数据插入数据库
        call_dbus_insert(bus, "Messages", client_message);
        // 从数据库中选择数据
        call_dbus_select(bus, "Messages");
        printf("Data select successfully\n");
                
      if (send(sock, client_message, strlen(client_message), 0) < 0) {
      perror("Send failed");
    }
}
    if (read_size == 0) {
        puts("Client disconnected");
    } else if (read_size == -1) {
        perror("Recv failed");
    }

   close(sock); // 关闭套接字
    return NULL;
}
int main() {
    // 创建D-Bus连接
    int r;
    r = sd_bus_open_user(&bus);
    if (r < 0) {
        fprintf(stderr, "Failed to connect to D-Bus: %s\n", strerror(-r));
        return EXIT_FAILURE;
    }

    call_create_table(bus, "Messages");

    // 请求一个已知的服务名
    r = sd_bus_request_name(bus, "com.example.CalculatorService", 0);
    if (r < 0) {
        fprintf(stderr, "Failed to acquire service name: %s\n", strerror(-r));
        sd_bus_unref(bus);
        return EXIT_FAILURE;
    }

    // 创建socket
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Socket creation failed!");
        exit(EXIT_FAILURE);
    }

    // 准备地址
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    // 绑定到本地地址127.0.0.1
    if (inet_aton("127.0.0.1", &server_addr.sin_addr) == 0) {
        perror("Invalid address");
        close(server_sock);
        exit(EXIT_FAILURE);
    }
    //server.sin_addr.s_addr = INADDR_ANY,绑定到所有接口

    // 绑定
    int res = bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (res == -1) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // 监听接口
    if (listen(server_sock, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // 等待客户端连接
    int client_sock;
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    client_sock = accept(server_sock, (struct sockaddr *)&client, &client_len);
    if (client_sock <= 0) {
        perror("Accept failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // 动态分配内存存储客户端套接字
    int *new_sock = malloc(sizeof(int));
    if (new_sock == NULL) {
        perror("Failed to allocate memory");
        close(client_sock);
        close(server_sock);
        exit(EXIT_FAILURE);
    }
    *new_sock = client_sock;

    // 创建处理客户端的线程
    pthread_t client_thread;
    if (pthread_create(&client_thread, NULL, handle_client, (void *)new_sock) < 0) {
        perror("Could not create thread");
        free(new_sock);
        close(client_sock);
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // 等待处理客户端线程结束
    pthread_join(client_thread, NULL);

    // 关闭服务器套接字
    close(server_sock);
    return 0;
}
