#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER "127.0.0.1"
#define PORT 8080
#define FILENAME "./server_responses.txt"
int main() {
    /* 创建 socket */
    int sock;
    char message[1000], server_reply[2000];  
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    //puts("Socket created");

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
    //puts("Connected");
    
    FILE *fp = fopen(FILENAME, "a+");
    if (fp == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }
    
    while (1) {
        printf("Enter message: ");
        scanf("%s", message);
        
       /* 发送消息给服务器 */
        if (send(sock, message, strlen(message), 0) < 0) {
            perror("send failed");
            return EXIT_FAILURE;
        }
        
        // 清空 message 缓冲区，避免下一次循环重复发送相同的消息
    	memset(message, 0, sizeof(message));
    	
        // 接收服务器的响应
        if (recv(sock, server_reply, sizeof(server_reply), 0) < 0) {
            perror("recv failed");
            return EXIT_FAILURE;
        }
        
        printf("Server reply: %s\n", server_reply);
        
        // 将响应写入文件
        if (fprintf(fp, "%s\n", server_reply) < 0) {
            perror("Error writing to file");
            break;  // 如果写入失败，跳出循环
        }
	 fflush(fp);
	 
        /* 清空 server_reply 缓冲区 */
        memset(server_reply, 0, sizeof(server_reply));
    }

    fclose(fp);
    close(sock);
    return 0;
}

