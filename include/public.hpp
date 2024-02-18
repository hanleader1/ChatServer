#ifndef PUBLIC_H
#define PUBLIC_H

enum MsgType{
    LOGIN_MSG=1,//登录消息
    LOGIN_MSG_ACK=11,
    REGISTER_MSG=0,//注册消息
    REGISTER_MSG_ACK=00,
    ONE_CHAT_MSG=2,
    ADD_FRIEND_MSG=3,
    CREATE_GROUP_MSG=4,
    ADD_GROUP_MSG=5,
    GROUP_CHAT_MSG=6,
    LOGINOUT_MSG=7
};


#endif