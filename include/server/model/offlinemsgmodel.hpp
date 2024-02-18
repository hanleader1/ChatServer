#pragma once
#include <string>
#include <vector>
using namespace std;

class OfflineMsgModel{
public:
    void insert(int id,string msg);
    void remove(int id);
    vector<string> query(int id);

private:
};