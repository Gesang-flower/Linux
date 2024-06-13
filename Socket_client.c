#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER "127.0.0.1"
#define PORT 8080

int main() {
    /* 创建 socket */
    int sock;
    char message[1000], server_reply[2000];  
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    puts("Socket created");

    /* 准备地址 */
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    /* 修改为服务器所在主机 IP 地址 */
    if (inet_aton(SERVER, &server.sin_addr) == 0) {
        perror("inet_aton failed");
        exit(EXIT_FAILURE);
    }

    /* 连接服务器 */
    int res = connect(sock, (struct sockaddr *)&server, sizeof(server));
    if (res < 0) {
        perror("connect failed");
        return EXIT_FAILURE;
    }
    puts("Connected");

    while (1) {
        printf("Enter message: ");
        scanf("%s", message);

        /* 发送消息给服务器 */
        if (send(sock, message, strlen(message), 0) < 0) {
            perror("send failed");
            return EXIT_FAILURE;
        }

        /* 清空 server_reply 缓冲区 */
        memset(server_reply, 0, sizeof(server_reply));
        
        /* 接收服务器的响应 */
        if (recv(sock, server_reply, 2000, 0) < 0) {
            perror("recv failed");
            break;
        }

        printf("Server reply: %s\n", server_reply);
    }

    close(sock);
    return 0;
}

