//
// Created by erick on 12/24/18.
//

#include <string.h>
#include <string>
#include <fstream>
#include <iostream>
#include <ctime>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <zconf.h>

#include "MyServer.h"

static unsigned long alarmSeconds = LOG_SECONDS;

static void writeLog(int n) {
    const char *buf = "an hour log...\n";

    std::ofstream stream;
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo);
    std::string str(buffer);
    stream.open(std::string("./") + str);
    stream << buf;
    stream.close();

    alarm(alarmSeconds);  // 重新设置为1个小时

}

MyServer::MyServer(int maxWaiter) :
        TcpServer(maxWaiter) {
    // 测试用的8个线程
    threadPool = std::make_unique<ThreadPool>(8);
    alarmSeconds = LOG_SECONDS;  // 一个小时
    signal(SIGALRM, writeLog);
}

void MyServer::newConnection() {
    // 建立新连接在主线程
    int connfd = accept(m_listen_sockfd, NULL, NULL);
    if (connfd < 0) {
        //throw std::runtime_error("accept new connection error\n");
        std::cout << "accept new connection error\n";
    }

    std::cout << "a new client come\n";

    epoll_event ev;
    bzero(&ev, sizeof(ev));
    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    ev.data.fd = connfd;

    // 注册新的连接事件
    if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, connfd, &ev) < 0) {
        // throw std::runtime_error("register event error\n");
        std::cout << "register event error\n";
        return;
    }
    setnonblocking(connfd);
}

void MyServer::existConnection(int fd) {
    std::cout << "deal existed connection\n";

    // 处理已经存在客户端的请求在子线程处理
    threadPool->enqueue([this, fd]() {  // lambda统一处理即可

        if (this->m_epollEvents[fd].events & EPOLLIN) { // 数据

            char buf[MAX_BUFFER];
            memset(buf, 0, MAX_BUFFER);
            int rc = recv(m_epollEvents[fd].data.fd, buf, MAX_BUFFER, 0);

            std::cout << "rc: " << rc << std::endl;

            if (rc <= 0) {
                throw std::runtime_error("recv() error \n");
            } else {

                std::cout << "receive client message\n";

                time_t rawtime;
                struct tm *timeinfo;
                char buffer[80];
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo);
                std::string str(buffer);

                std::ofstream stream;
                stream.open(std::string("./") + str);
                stream << buf;
                stream.close();

                const char *msg = "receive your msg\n";
                int ret = send(m_epollEvents[fd].data.fd, msg, strlen(msg), 0);
                if (ret < 0) {
                    //throw std::runtime_error("error in send()\n");
                    std::cout << "error on send()\n";
                    return;
                }
            }

        } else if (this->m_epollEvents[fd].events & EPOLLRDHUP) {  // 客户端主动断开连接
            if (epoll_ctl(m_epfd, EPOLL_CTL_DEL, m_epollEvents[fd].data.fd, NULL) < 0) {
                throw std::runtime_error("delete client error\n");
            }
            std::cout << "a client left\n";
        } else {  // 未知错误
            //throw std::runtime_error("unknown error");
            std::cout << "unknown error\n";
            return;
        }

    });
}

int MyServer::setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void MyServer::setTimer(unsigned long n) {
    alarmSeconds = n;
}
