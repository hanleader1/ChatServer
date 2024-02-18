#include "usermodel.hpp"

bool UserModel::insert(User& user) {
    char sql[256] = { 0 };
    sprintf(sql, "insert into user(name,password,state) values('%s','%s','%s')",
        user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());
    MySql mysql;
    if (mysql.connect()) {
        if (mysql.update(sql)) {
            //获取插入成功的用户id
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

User UserModel::queryById(int id) {
    char sql[256] = { 0 };
    sprintf(sql, "select * from user where id=%d",id);
    MySql mysql;
    if (mysql.connect()) {
        MYSQL_RES* res=mysql.query(sql);
        if(res!=nullptr){
            MYSQL_ROW row=mysql_fetch_row(res);
            if(row!=nullptr){
                User user(atoi(row[0]),row[1],row[2],row[3]);
                mysql_free_result(res);
                return user;
            }
        }
    }
    return false;
}

bool UserModel::updateStateById(const int id,const char* state){
    char sql[256] = { 0 };
    sprintf(sql, "update user set state='%s' where id='%d'",
        state,id);
    MySql mysql;
    if (mysql.connect()) {
        if (mysql.update(sql)) {
            return true;
        }
    }
    return false;
}

bool UserModel::reset(){
    const char* sql="update user set state='offline' where state='online'";
    MySql mysql;
    if (mysql.connect()) {
        if (mysql.update(sql)) {
            LOG_INFO<<"程序退出,全体下线!";
            return true;
        }
    }
    return false;
}