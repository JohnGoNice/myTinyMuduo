//
// Created by john on 4/21/19.
//

#include "net/Acceptor.h"

#include "base/Logging.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include "net/SocketsOps.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop),
      //构造一个非阻塞Socket对象
      acceptSocket_(sockets::creatNonblockingOrDie(listenAddr.family())),
      //创建1个属于当前事件循环（当前线程）的Channel对象，并将该Channel对象与监听套接字建立对应关系
      acceptChannel_(loop,acceptSocket_.fd()),
      listenning_(false),
      //打开一个空fd用于占位
      idleFd_(::open("/dev/null",O_RDONLY|O_CLOEXEC))
{
    assert(idleFd_ >= 0);
    //设置监听套接字，复用地址和端口
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reuseport);
    //绑定地址和端口
    acceptSocket_.bindAddress(listenAddr);

    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));

}


Acceptor::~Acceptor() {
    //取消Channel对监听套接字所有事件的监听
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    ::close(idleFd_);
}

//开始监听TCP连接请求
void Acceptor::listen(){
    //不允许跨线程
    loop_->assertInLoopThread();
    listenning_ = true;
    //acceptSocket_成为监听套接字
    acceptSocket_.listen();
    //acceptChannel_开启对读事件的监听
    acceptChannel_.enableReading();
}

//acceptChannel_可读事件发生了（有Tcp连接已建立）
void Acceptor::handleRead(){
    //不允许跨线程
    loop_->assertInLoopThread();
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    //若已建立TCP连接，connfd为已连接套接字
    if(connfd >= 0){
        if(newConnectionCallback_){
            //调用建立连接回调函数处理已连接套接字（需要使用对端地址）
            //将TCP连接分给其他线程的EventLoop处理
            newConnectionCallback_(connfd,peerAddr);
        }else{
            sockets::close(connfd);
        }
    }
    else{
        //这里使用了1个技巧来处理，已连接套接字达到上限的技巧
        //通过使用一个占位描述符，在描述符达到上限后关闭占位描述符
        //然后接受新的TCP连接，然后马上关闭连接，这样可以及时通知客户端，服务端文件描述符已满
        //随后继续打开一个占位描述符
        LOG_SYSERR << "in Acceptor::handleRead";
        // Read the section named "The special problem of
        // accept()ing when you can't" in libev's doc.
        // By Marc Lehmann, author of libev.
        if(errno == EMFILE){
            ::close(idleFd_);
            idleFd_ = ::accept(acceptSocket_.fd(),NULL,NULL);
            ::close(idleFd_);
            idleFd_ = ::open("dev/null", O_RDONLY|O_CLOEXEC);
        }
    }
}






















