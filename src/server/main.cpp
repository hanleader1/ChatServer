#include "chatserver.hpp"
#include <signal.h>
#include "chatservice.hpp"

void resetHandler(int){
    ChatService::instance()->reset();
    exit(0);
}

int main(int arc,char** argv){
    signal(SIGINT,resetHandler);
    EventLoop loop;
    
    InetAddress addr("127.0.0.1",atoi(argv[1]));
    ChatServer server(&loop,addr,"charServer");
    server.start();
    loop.loop();
    return 0;
}
