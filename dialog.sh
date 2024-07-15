#!/bin/bash

SOCKET_CLIENT="./Socket_client"

while true; do
    # 使用 dialog 显示界面，让用户输入数据
    MESSAGE=$(dialog --inputbox "Enter message:" 8 40 2>&1 >/dev/tty)

    # 如果用户取消输入，则退出脚本
    if [ $? -ne 0 ]; then
        exit 1
    fi

    # 发送数据给本地 SOCKET 客户端，仅当 MESSAGE 非空时发送
    if [ -n "$MESSAGE" ]; then     
        # 发送消息给 Socket_client
        SERVER_REPLY=$(timeout 3 $SOCKET_CLIENT <<< "$MESSAGE")
        dialog --msgbox "Server reply: $MESSAGE" 8 40
        dialog --msgbox "Server reply: $SERVER_REPLY" 8 40
    fi
done

exit 0

