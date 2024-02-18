#ifndef CHATSERVER_H
#define CHATSERVER_H
#include <iostream>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Logging.h>
using std::cout;
using std::endl;

using namespace muduo::net;
using namespace muduo;

class ChatServer{
public:
    ChatServer(EventLoop* ,const InetAddress& ,const string& );
    void start();

private:
    TcpServer _server;
    EventLoop *_loop;
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr &conn,Buffer *buf,Timestamp time);

};

#endif