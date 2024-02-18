#include "friendmodel.hpp"
#include "db.h"

void FriendModel::insert(int userid,int friendid){
    char sql[128]={0};
    sprintf(sql,"insert into friend values(%d,%d)",userid,friendid);
    MySql mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}

vector<User> FriendModel::query(int userid){
    char sql[512]={0};
    sprintf(sql,"select u.id,u.name,u.state from user u where u.id in(select friendid from friend where userid=%d) or u.id in(select userid from friend where friendid=%d)",
    userid,userid);
    MySql mysql;
    vector<User> v;
    if(mysql.connect()){
        MYSQL_RES *res=mysql.query(sql);
        if(res!=nullptr){
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr){
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                
                v.push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return v;
}