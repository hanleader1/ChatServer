#ifndef CHATSERVICE_H
#define CHARSERVICE_H

#include <muduo/net/TcpServer.h>
#include <unordered_map>
#include <functional>
#include <mutex>

#include "usermodel.hpp"
#include "json.hpp"
#include "offlinemsgmodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"
using namespace std;
using namespace muduo;
using namespace muduo::net;
using json=nlohmann::json;

//ChatServer服务业务类
//单例模式，类似工具类
using MsgHandler=function<void(const TcpConnectionPtr&,json&,Timestamp)>;
class ChatService{
public:
    //获取单例对象的接口函数
    static ChatService* instance();
    void login(const TcpConnectionPtr&,json&,Timestamp);
    void regist(const TcpConnectionPtr&,json&,Timestamp);
    MsgHandler getHandler(int);
    void clientCloseException(const TcpConnectionPtr&);
    void oneChat(const TcpConnectionPtr&,json&,Timestamp);
    void addFriend(const TcpConnectionPtr&,json&,Timestamp);
    void createGroup(const TcpConnectionPtr&,json&,Timestamp);
    void addGroup(const TcpConnectionPtr&,json&,Timestamp);
    void groupChat(const TcpConnectionPtr&,json&,Timestamp);
    void loginOut(const TcpConnectionPtr&,json&,Timestamp);
    void handleRedisSubscribeMessage(int id,string msg);
    void reset();

    
private:
    ChatService();
    unordered_map<int,MsgHandler> msgHandlerMap;
    
    unordered_map<int,TcpConnectionPtr> usrConnectionMap;

    mutex connectMutex;
    //数据操作类对象
    UserModel usrModel;
    OfflineMsgModel offlineMsgModel;
    FriendModel friendModel;
    GroupModel groupModel;
    Redis redis;
};

#endif