#include <iostream>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <vector>
#include <unistd.h>
#include <string.h>


// 函数：安全地从socket读取指定长度的数据
size_t read_n_bytes(int fd, char *buffer, size_t n) {
    size_t total_read = 0;
    ssize_t bytes_read;
    while (total_read < n) {
        bytes_read = read(fd, buffer + total_read, n - total_read);
        if (bytes_read == 0) { // 对端关闭连接
            return total_read;
        }
        if (bytes_read == -1) {
            return -1; // 读取错误
        }
        total_read += bytes_read;
    }
    return total_read;
}

// 主函数：从socket读取4字节长度，然后读取对应长度的数据
int read_message(int socket_fd) {
    uint32_t length;
    char *data;

    // 首先读取4字节的长度信息
    if (read_n_bytes(socket_fd, (char *)&length, 4) != 4) {
        return -1;
    }

    // 将网络字节序转换为主机字节序
    length = ntohl(length);

    // 根据读取的长度分配内存
    data = (char *)malloc(length);
    if (data == nullptr) {
        return -1;
    }

    // 读取指定长度的数据
    if (read_n_bytes(socket_fd, data, length) != length) {
        free(data);
        return -1;
    }

    // 释放内存
    free(data);
    return 0;
}

void run(int fd){
    std::cout << "recv fd:" << fd << std::endl;
    while (true){
        if (read_message(fd) < 0){
            break;
        }
    }
    close(fd);
    std::cout << "recv end fd:" << fd << std::endl;
}

int main() {
    int listenFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in server;
    server.sin_port = htons(8888);;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenFd, (struct sockaddr*)&server, sizeof(server)) != 0){
        return 0;
    }

    if (int ret = listen(listenFd, 100) != 0){
        return ret;
    }
    std::vector<std::thread> threads;
    while (true){
        struct sockaddr_in cli;
        uint32_t len = sizeof(cli);
        int fd = accept(listenFd, (struct sockaddr*)&cli, &len);
        if(fd < 0){
            continue;
        }
        threads.push_back(std::thread(run, fd));
        std::cout<<"new cli: " << cli.sin_addr.s_addr<<std::endl;
    }
}