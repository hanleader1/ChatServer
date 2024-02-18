#ifndef USERMODEL_H
#define USERMODEL_H
#include "db.h"
#include "user.hpp"

class UserModel{
public:
    bool insert(User& user);
    User queryById(int id);
    bool updateStateById(const int,const char*);
    bool reset();
private:
};

#endif