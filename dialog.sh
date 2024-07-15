#!/bin/bash

SOCKET_CLIENT="./Socket_client"
FILENAME="./server_responses.txt"

while true; do
    # 使用 dialog 显示界面，让用户输入数据
    MESSAGE=$(dialog --inputbox "Enter message:" 8 40 2>&1 >/dev/tty)

    # 如果用户取消输入，则退出脚本
    if [ $? -ne 0 ]; then
        exit 1
    fi

    # 发送数据给 Socket_client
    $SOCKET_CLIENT <<< "$MESSAGE"
    
    # 检查是否有服务器响应
    if [ -s "$FILENAME" ]; then
        RESPONSE=$(cat "$FILENAME")
        dialog --msgbox "Server reply: $RESPONSE" 8 40
        # 清空文件内容
        > "$FILENAME"
    fi
done

exit 0

