#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <map>
using namespace muduo;
//服务接口
ChatService* ChatService::instance(){
    static ChatService service;
    return &service;
}

//构建服务模块
ChatService::ChatService(){
    msgHandlerMap.insert({LOGIN_MSG,std::bind(&ChatService::login,this,_1,_2,_3)});
    msgHandlerMap.insert({REGISTER_MSG,std::bind(&ChatService::regist,this,_1,_2,_3)});
    msgHandlerMap.insert({ONE_CHAT_MSG,std::bind(&ChatService::oneChat,this,_1,_2,_3)});
    msgHandlerMap.insert({ADD_FRIEND_MSG,std::bind(&ChatService::addFriend,this,_1,_2,_3)});
    msgHandlerMap.insert({CREATE_GROUP_MSG,std::bind(&ChatService::createGroup,this,_1,_2,_3)});
    msgHandlerMap.insert({ADD_GROUP_MSG,std::bind(&ChatService::addGroup,this,_1,_2,_3)});
    msgHandlerMap.insert({GROUP_CHAT_MSG,std::bind(&ChatService::groupChat,this,_1,_2,_3)});
    msgHandlerMap.insert({LOGINOUT_MSG,std::bind(&ChatService::loginOut,this,_1,_2,_3)});

    if(redis.connect())
        redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage,this,_1,_2));
}

void ChatService::reset(){
    UserModel usr;
    usr.reset();
}

MsgHandler ChatService::getHandler(int msgid){
    //记录错误的日志
    auto it=msgHandlerMap.find(msgid);
    if(it==msgHandlerMap.end()){
        return [=](const TcpConnectionPtr& a,json& b,Timestamp c){
            LOG_ERROR<<"cannot find handler---"<<"msgid:"<<msgid;
        };
    }
    else
        return msgHandlerMap[msgid];
}

//登录测试用例 {"msgid":1,"id":1,"password":"123456"}
//{"msgid":1,"id":2,"password":"root"}
void ChatService::login(const TcpConnectionPtr& conn,json& js,Timestamp time){
    LOG_INFO<<"login()";
    int id=js["id"];
    string pwd=js["password"];
    User user=usrModel.queryById(id);
    if (user.getId()==id&&user.getPwd()==pwd) {
        json response;
        if(user.getState()=="online"){
            //已登录，不允许重复登录
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] =1;
            response["errmsg"]="该账号已登录";
            conn->send(response.dump());
        }
        else {
            {
            lock_guard<mutex> lock(connectMutex);
            //登录成功,记录用户连接信息
            usrConnectionMap.insert({id,conn});
            LOG_INFO<<"id为"<<id<<"登录成功";
            }
            //向redis订阅channel(id)
            redis.subscribe(id);
            user.setState("online");
            usrModel.updateStateById(id,"online");
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            //查询用户是否有离线消息
            vector<string> v=offlineMsgModel.query(id);
            if(!v.empty()){
                response["offlinemsg"]=v;
                offlineMsgModel.remove(id);
            }
            //查询好友的信息
            vector<User> usrV=friendModel.query(id);
            if(!usrV.empty()){
                vector<string> v2;
                for(User &usr:usrV){
                    LOG_INFO<<"查询到的好友id:"<<usr.getId();
                    json js;
                    js["id"]=usr.getId();
                    js["name"]=usr.getName();
                    js["state"]=usr.getState();
                    v2.push_back(js.dump());
                }
                response["friends"]=v2;
            }
            conn->send(response.dump());
        }
    }
    else if(user.getId()!=id){
        json response;
            //不存在该用户
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] =2;
            response["errmsg"]="不存在该用户";
            conn->send(response.dump());
        }
    else if(user.getPwd()!=pwd){
        json response;
        //密码错误
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 3;
        response["errmsg"]="密码错误";
        conn->send(response.dump());
    }
}

//注册用例{"msgid":0,"id":2,"name":"lisi","password":"123456"}
void ChatService::regist(const TcpConnectionPtr& conn,json& js,Timestamp time){
    LOG_INFO<<"register()";
    string name=js["name"];
    string pwd=js["password"];
    User user;
    user.setName(name);
    user.setPwd(pwd);
    if(usrModel.insert(user)){
        json response;
        response["msgid"]=REGISTER_MSG_ACK;
        response["errno"]=0;
        response["id"]=user.getId();
        conn->send(response.dump());
    }
    else{
        json response;
        response["msgid"]=REGISTER_MSG_ACK;
        response["errno"]=1;
        conn->send(response.dump());
    }
}

void ChatService::clientCloseException(const TcpConnectionPtr& conn){
    int id;
    {
        lock_guard<mutex> lock(connectMutex);
        for (auto it = usrConnectionMap.begin(); it != usrConnectionMap.end();
             ++it) {
            if (it->second == conn) {
                id = it->first;
                usrConnectionMap.erase(it);
                break;
            }
        }
    }
    if(id!=-1){
        LOG_INFO<<"该用户下线";
        usrModel.updateStateById(id,"offline");
        redis.unsubscribe(id);
    }
}

// 处理注销业务
void ChatService::loginOut(const TcpConnectionPtr& conn, json& js, Timestamp time){
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(connectMutex);
        auto it = usrConnectionMap.find(userid);
        if (it != usrConnectionMap.end())
            usrConnectionMap.erase(it);
    }
    redis.unsubscribe(userid);
    // 更新用户的状态信息
    usrModel.updateStateById(userid,"offline");
}

void ChatService::handleRedisSubscribeMessage(int id, string msg) {
    lock_guard<mutex> lock(connectMutex);
    auto it=usrConnectionMap.find(id);
    if(it!=usrConnectionMap.end()){
        it->second->send(msg);
        return ;
    }
    offlineMsgModel.insert(id,msg);
}

//聊天测试用例  {"msgid":2,"id":1,"from":"zhangsan","to":2,"msg":"hello"}
void ChatService::oneChat(const TcpConnectionPtr& conn,json& json,Timestamp time){
    int toid=json["toid"].get<int>();
    {//是否在线,线程安全,否则a发给b,c的消息可能都发不出去
        lock_guard<mutex> lock(connectMutex);
        auto it=usrConnectionMap.find(toid);
        if(it!=usrConnectionMap.end()){
            it->second->send(json.dump());
            return ;
        }
    }
    
    User user=usrModel.queryById(toid);
    if(user.getState()=="online")
        redis.publish(toid,json.dump());
    offlineMsgModel.insert(toid,json["msg"]);
}

void ChatService::addFriend(const TcpConnectionPtr& conn,json& js,Timestamp time){
    int id=js["id"].get<int>();
    int friendid=js["friendid"].get<int>();
    friendModel.insert(id,friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if (groupModel.createGroup(group)){
        // 存储群组创建人信息
        groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    groupModel.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = groupModel.queryGroupUsers(userid, groupid);
    lock_guard<mutex> lock(connectMutex);
    for (int id : useridVec){
        auto it = usrConnectionMap.find(id);
        if (it != usrConnectionMap.end()){
            // 转发群消息
            it->second->send(js.dump());
        }
    }
}

