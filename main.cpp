#include <iostream>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <vector>
#include <unistd.h>
#include <string.h>

int send_n_data(int sock, size_t len,char *data_ptr) {
    // 发送长度信息
    uint32_t net_len = htonl(len);  // 将长度转换为网络字节序
    if (send(sock, &net_len, sizeof(net_len), 0) != sizeof(net_len)) {
        return -1;
    }

    // 发送实际数据
    size_t total_sent = 0;
    while (total_sent < len) {
        ssize_t sent = send(sock, data_ptr + total_sent, len - total_sent, 0);
        if (sent == -1) {
            return -1;
        }
        total_sent += sent;
    }
    return int(total_sent);
}

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
int read_message(int socket_fd,char *data) {
    uint32_t length;

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
    char* readBuff = (char*)malloc(1024000);
    char* writeBuff = (char*)malloc(1024000);
    while (true){
        if (read_message(fd,readBuff) < 0){
            break;
        }
        if (send_n_data(fd, 1024, writeBuff) < 0){
            break;
        }
    }
    close(fd);
    free(readBuff);
    free(writeBuff);
    std::cout << "recv end fd:" << fd << std::endl;
}

int bindAddr(uint16_t port,size_t size){
    int listenFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in server;
    server.sin_port = htons(port);;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    int ret = bind(listenFd, (struct sockaddr*)&server, sizeof(server));
    if (ret < 0){
        return ret;
    }
    if (int ret = listen(listenFd, size) != 0){
        return ret;
    }
    return listenFd;
}

int main() {
    int listenFd = bindAddr(8888, 100);
    if (listenFd < 0){
        return listenFd;
    }
    std::vector<std::thread> threads;
    while (true){
        struct sockaddr_in cli;
        uint32_t len = sizeof(cli);
        int fd = accept(listenFd, (struct sockaddr*)&cli, &len);
        if(fd < 0){
            continue;
        }
        std::thread(run, fd).detach();
        std::cout<<"new cli: " << cli.sin_addr.s_addr<<std::endl;
    }
}