# read_response.sh
# 读取服务器响应消息
SERVER_DATA_FILE="./server_data.txt"
retry=3
while [ $retry -gt 0 ]; do
    if [ -f "$SERVER_DATA_FILE" ]; then
        cat "$SERVER_DATA_FILE"
        rm -f "$SERVER_DATA_FILE"  # 读取后删除文件，确保下一次不读到旧数据
        break
    else
        # 等待一段时间再次尝试读取文件
        sleep 1
        ((retry--))
    fi
done

