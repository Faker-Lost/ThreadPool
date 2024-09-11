#include<iostream>
#include<stdlib.h>
#include<unistd.h>
#include<string>
#include<cstring>
#include<arpa/inet.h>
using namespace std;
int main()
{
    //1. 创建用于通信的套接字
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd==-1)
    {
        perror("socket");
        return -1;
    }
    //2. 初始化结构体，连接服务器IP PORT
    struct sockaddr_in saddr;
    //初始化saddr
    saddr.sin_family = AF_INET;
    //主机字节 -> 网络字节 端口号
    saddr.sin_port = htons(8888);
    // ip地址 主机字节->大端字节序，存到saddr中
    inet_pton(AF_INET, "192.168.191.101", &saddr.sin_addr.s_addr);
    // connetct 连接操作
    int ret = connect(fd, (struct sockaddr*)&saddr, sizeof(saddr));
    if(ret== -1)
    {
        perror("connect");
        return -1;
    }

    //3. 与服务端通信
    int number = 0;
    while(1)
    {
        //1. 发送数据
        char buffer[1024];
        sprintf(buffer, "你好,hello world, %d...\n", number++);
        send(fd, buffer, strlen(buffer)+1, 0);
        //1. 清空buffer 2. recv 接收数据 
        memset(buffer, 0, sizeof(buffer));
        int len = recv(fd, buffer, sizeof(buffer), 0);
        if(len>0)
        {
            printf("Server say: %s\n", buffer);
        }
        else if(len==0)//==0说明断开连接
        {
            printf("0000000服务器断开连接0000000\n");
            break;
        }
        else
        {
            perror("recv");
            break;
        }
        sleep(1);
    }
    //4 关闭文件描述符
    close(fd);
    
    return 0;
}
