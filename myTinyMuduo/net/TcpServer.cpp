//
// Created by john on 4/25/19.
//

#include "net/TcpServer.hpp"

#include "base/Logging.hpp"
#include "net/Acceptor.hpp"
#include "net/EventLoop.hpp"
#include "net/EventLoopThread.hpp"
#include "net/EventLoopThreadPool.hpp"
#include "net/SocketsOps.hpp"

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, Option option)
    : loop_(loop),
      ipPort_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop,listenAddr,option == kReusePort)),
      threadPool_(new EventLoopThreadPool(loop,name_)),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback),
      nextConnId_(1)
{
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection,this,_1,_2));
}

TcpServer::~TcpServer() {
    loop_->assertInLoopThread();
    LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";

    for(auto& item:connections_){
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        //让所有TcpConnection所属的EventLoop清理剩下的连接
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed,conn));
    }
}

void TcpServer::setThreadNum(int numThreads) {
    assert(0<= numThreads);
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start() {
    if(started_.getAndSet(1)==0){
        threadPool_->start(threadInitCallback_);
        assert(!acceptor_->listenning());
        //主线程开始监听新连接
        loop_->runInLoop(std::bind(&Acceptor::listen,get_pointer(acceptor_)));
    }
}

//有连接建立的时候Acceptor::handleRead()调用newConnection
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {
    //必须在主线程由主事件循环调用
    loop_->assertInLoopThread();
    //在线程池中取出一个线程的EventLoop对象处理新连接
    EventLoop* ioLoop = threadPool_->getNextLoop();

    char buf[64];
    snprintf(buf, sizeof(buf),"-%s#%d",ipPort_.c_str(),nextConnId_);
    ++nextConnId_;  //对每个新建立的连接编号
    std::string connName = name_+buf;
    LOG_INFO << "TcpServer::newConnection [" << name_<< "] - new connection ["
             << connName<< "] from " << peerAddr.toIpPort();
    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    //构造TcpConnection对象，并用shared_ptr管理
    TcpConnectionPtr conn(new TcpConnection(ioLoop,connName,sockfd,localAddr,peerAddr));
    //将新建的TcpConnection对象添加到connections_
    connections_[connName] = conn;

    //设置重要的4个回调函数（三个半事件）
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection,this,_1));

    //runInLoop将connectEstablished加入ioLoop的任务队列
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished,conn));

}

//切换为主线程（排队执行避免竞争）
void TcpServer::removeConnection(const TcpConnectionPtr &conn) {
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop,this,conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn) {
    //调用完成之前TcpConnectionPtr不会析构
    //见“ TcpConnection::handleClose：TcpConnectionPtr guardThis(shared_from_this());”
    loop_->assertInLoopThread();
    LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
             << "] - connection " << conn->name();
    //在主线程中的事件循环中，删除conn连接
    size_t n=connections_.erase(conn->name());
    (void)n;
    assert(n==1);
    //获取conn所属EventLoop对象
    EventLoop* ioLoop = conn->getLoop();
    //切换到IO线程执行connectDestroyed
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed,conn));
}
























