#include<iostream>
#include<stdlib.h>
#include<unistd.h>
#include<string>
#include<arpa/inet.h>
#include<strings.h>
#include"threadpool.h"
//基于多线程实现
#include<pthread.h>
void working(void* arg);

using namespace std;

struct PoolInfo
{
    int fd;
};


//信息结构体 
struct SockInfo
{
    struct sockaddr_in addr;    // 存放ip和端口
    int fd;                     //存放用于通信文件描述符
};
//定义结构体数组，最大允许创建线程512个
struct SockInfo infos[512];
void acceptConnect(void* arg);


//子线程函数
void working(void* arg)
{  
    //类型转换 arg
    struct SockInfo* pinfo = (struct SockInfo*)arg;
    // 连接建立成功，打印客户端ip和端口信息，需要文件描述符；因此需要参数传递到函数体内部，
    // 创建一个结构体，把结构体的地址传递给woking函数
    char ip[32];

    printf("客户端IP地址：%s, 端口：%d\n", 
        //输出是大端，需要转换为小端再输出
        inet_ntop(AF_INET, &pinfo->addr.sin_addr.s_addr, ip, sizeof(ip)),
        ntohs(pinfo->addr.sin_port)
        );
    
    //5. 与客户端通信
    while(1)
    {
        //接收数据额
        char buffer[1024];
        //read recv 通信文件描述符
        int len = recv(pinfo->fd, buffer, sizeof(buffer), 0);
        if(len > 0)
        {
            printf("Clinet say: %s\n", buffer);
            send(pinfo->fd, buffer, sizeof(buffer), 0);
        }
        else if(len == 0)//==0说明断开连接
        {
            printf("0000000客户端断开连接0000000\n");
            break;
        }
        else
        {
            perror("recv");
            break;
        }
    }
    //关闭通信文件描述符
    close(pinfo->fd);
}


//使用线程池进行通信,创建连接函数
void acceptConnect(void* arg)
{

    ThreadPool pool;
    //类型转换
    PoolInfo* poolinfo = (PoolInfo*)arg;
    //4. 阻塞并等待客户端连接 主进程 建立连接、 创建子线程
    int addrlen =sizeof(struct sockaddr_in);
    //父进程监听，子进程通信
    while(1)
    {
        //定义结构体指针
        struct SockInfo* p_info;
        p_info = (struct SockInfo*)malloc(sizeof(struct SockInfo));
        // accept(); 参数1. 用于监听的文件描述符
        // 文件描述符从poolinfo指针中取出
        p_info->fd = accept(poolinfo->fd, NULL, NULL);

        if(p_info->fd == -1)
        {
            perror("accept");
            //跳出循环，断开连接
            continue;
        }
        //添加通信的任务,addTask
        pool.addTask(working, p_info);
    }
    //关闭监听的描述符，通信的描述符在子线程中使用
    close(poolinfo->fd);
}

//主函数
int main()
{
    //1. 创建监听的套接字
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd==-1)
    {
        perror("socket");
        return -1;
    }
    //2. 绑定本地ip 端口 初始化结构体
    struct sockaddr_in saddr;
    //初始化saddr
    saddr.sin_family = AF_INET;
    //初始化端口 主机字节 -> 网络字节 端口号 转化为大端 
    saddr.sin_port = htons(8888);
    // 初始化ip地址
    saddr.sin_addr.s_addr = htonl(INADDR_ANY); //绑定任意的IP地址
    int ret = bind(listenfd, (struct sockaddr*)&saddr, sizeof(saddr));
    if(ret== -1)
    {
        perror("bind");
        return -1;
    }

    //3. 设置监听
   int rec = listen(listenfd, 128);
      if(rec== -1)
    {
        perror("listen");
        return -1;
    }

    // 初始化线程池，创建线程池
    ThreadPool pool(2,4);
    // 实例化结构体，info存储两部分数据 1. 用于监听的套接字listenfd 2. 线程池对象
    PoolInfo* info = (PoolInfo*)malloc(sizeof(PoolInfo));
    info->fd = listenfd;

    pool.addTask(acceptConnect, info);
    
    pthread_exit(NULL);


    return 0;   
}
