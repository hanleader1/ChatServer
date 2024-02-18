#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"
#include <functional>
#include <string>

using namespace std::placeholders;
using json=nlohmann::json;

ChatServer::ChatServer(EventLoop* loop,const InetAddress& listenAddr,const string& name_arg)
    :_server(loop,listenAddr,name_arg){
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));
    _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));
    start();
    cout<<"ChatServer启动"<<endl;
    _server.setThreadNum(4);
}

void ChatServer::start(){
    _server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr& conn){
    if(!conn->connected()){
        LOG_INFO<<"断开连接";
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}
void ChatServer::onMessage(const TcpConnectionPtr &conn,Buffer *buf,Timestamp time){
    string buffer=buf->retrieveAllAsString();
    if(json::accept(buffer)){
        //数据反序列化
        json js=json::parse(buffer);
        //通过js[msgid]获取业务handler，解耦网络和业务
        LOG_INFO<<"业务msgid :"<<js["msgid"].get<int>();
        auto msgHandler=ChatService::instance()->getHandler(js["msgid"].get<int>());
        //回调消息绑定好的处理器进行相应的业务处理
        msgHandler(conn,js,time);
    }
    else{
        LOG_INFO<<"JSON不合法";
    }
}