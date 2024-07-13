#!/bin/bash

# 设置服务器地址和端口号
SERVER="127.0.0.1"
PORT="8080"
SERVER_DATA_FILE="./server_data.txt"

# 准备对话框的布局和选项
dialog_input() {
    dialog --clear --title "TCP Client" \
    --inputbox "Enter message to send:" 10 50 2>&1 >/dev/tty
}

dialog_message() {
    local message="$1"
    dialog --clear --title "Server Reply" \
    --msgbox "$message" 10 50
}

# 主循环，直到用户选择退出
while true; do
    # 显示输入对话框
    input=$(dialog_input)

    # 检查用户是否取消输入
    if [ $? -ne 0 ]; then
        break
    fi

    # 发送输入的消息到服务器（不打印换行）
    echo -n "$input" | nc $SERVER $PORT > /dev/null

    # 调用 read_response.sh 脚本读取服务器的响应消息
    server_reply=$(./read_response.sh)

    # 检查是否成功读取到服务器的响应消息
    if [ -z "$server_reply" ]; then
        dialog_message "Failed to read server response. Please try again."
        continue
    fi

    # 显示服务器的响应消息
    dialog_message "$server_reply"

done

# 显示退出消息
dialog --clear --title "TCP Client" --msgbox "Exiting TCP Client." 10 50

exit 0

