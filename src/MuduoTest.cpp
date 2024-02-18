#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
using namespace std::placeholders;
using namespace muduo;
using namespace muduo::net;
using std::cout;
using std::endl;

class ChatServer{
public:
    ChatServer(EventLoop *loop,const InetAddress& listen_addr,const string& name_arg)
                :_server(loop,listen_addr,name_arg),_loop(loop){
        //给服务器注册用户连接的创建和断开回调
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));
        
        //给服务器注册用户读写事件回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));

        //设置服务端的线程数量 1个IO 3个worker线程
        _server.setThreadNum(4);

        //开启事件循环
    }//明确TcpServer构造函数
    void start(){
            _server.start();
    }
private:
    //专门处理用户连接的创建和断开 epoll listenfd
    void onConnection(const TcpConnectionPtr& conn){
        if(conn->connected())// IP + port
            cout<<"IP/Port:"<<conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()
            <<"  ,state:online"<<endl;
        else{
            cout<<"IP/Port:"<<conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()
            <<"  ,state:offline"<<endl;
            conn->shutdown();
            //_loop->quit();
        }
    }

    //处理用户读写事件(连接，缓冲区，接受到的数据时间信息)
    void onMessage(const TcpConnectionPtr& conn,Buffer* buf,Timestamp time){
        string buffer=buf->retrieveAllAsString();
        cout<<"接收到的数据是:"<<buffer<<"time:"<<time.toString()<<endl;
        conn->send(buffer); //返回收到的数据
    }
    TcpServer _server; //组合TcpServer对象
    EventLoop *_loop;  //创建eventloop指针
};

int main(){
    EventLoop loop; //epoll
    InetAddress addr("127.0.0.1",6000);
    ChatServer server(&loop,addr,"ChatServer");
    server.start();//listenfd添加到epoll上,epoll_ctl=>epoll
    loop.loop();//epoll_wait,以阻塞方式等待用户的连接
}