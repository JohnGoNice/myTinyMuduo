#include"sudoku.h"
#include"base/Atomic.h"
#include"base/Logging.h"
#include"base/Thread.h"
#include"net/EventLoop.h"
#include"net/InetAddress.h"
#include"net/TcpServer.h"

#include<utility>
#include<stdio.h>
#include<unistd.h>

using namespace muduo;
using namespace muduo::net;


class SudokuServer{
    public:
    SudokuServer(EventLoop* loop,const InetAddress& listenAddr)
        : server_(loop,listenAddr,"SudokuServer"),
          startTime_(Timestamp::now())
    {
        server_.setConnectionCallback(std::bind(&SudokuServer::onConnection,this,_1));
        server_.setMessageCallback(std::bind(&SudokuServer::onMessage,this,_1,_2,_3));
    }
    void start(){
        server_.start();
    }

    private:
        void onConnection(const TcpConnectionPtr& conn){
            LOG_TRACE<<conn->peerAddress().toIpPort()<<" -> "
                     <<conn->localAddress().toIpPort()<<" is "
                     <<(conn->connected()?"UP":"DOWN");
        }
        void onMessage(const TcpConnectionPtr& conn, Buffer* buf,Timestamp){
            LOG_DEBUG<<conn->name();
            size_t len = buf->readableBytes();
            //当缓冲区长度大于kCells+2，提取读缓冲区的数据
            //（可能一次受到多个请求，循环处理）
            //如果收到非法请求服务端会断开连接
            while(len>=kCells+1){
                const char* eof = buf->findEOL();
                if(eof){
                    string request(buf->peek(),eof);
                    buf->retrieveUntil(eof+1);
                    len = buf->readableBytes();
                    if(!processRequest(conn,request)){
                        conn->send("Bad Request!\r\n");
                        conn->shutdown();
                        break;
                    }
                }else if(len>100){
                    conn->send("Id too long!\r\n");
                    conn->shutdown();
                    break;
                }else{
                    break;
                }
            }
        }
        bool processRequest(const TcpConnectionPtr& conn,const string &request){
            string id;
            string puzzle;
            bool goodRequest = true;
            //提取ID：（矩阵）
            string::const_iterator colon = find(request.begin(),request.end(),':');
            if(colon!=request.end()){
                id.assign(request.begin(),colon);
                puzzle.assign(colon+1,request.end());
            }else{
                puzzle = request;
            }
            if(puzzle.size() == implicit_cast<size_t>(kCells)){
                LOG_DEBUG<<conn->name();
                string result = solveSudoku(puzzle);
                //将得到的结果发回客户端
                if(id.empty()){
                    conn->send(result+"\r\n");
                }else{
                    conn->send(id+":"+result+"\r\n");
                }
            }else{
                goodRequest = false;
            }
            return goodRequest;
        }

        TcpServer server_;
        Timestamp startTime_;

};

int main(int argc,char* argv[]){
    LOG_INFO<<"pid = "<<getpid()<<", tid = "<<CurrentThread::tid();
    EventLoop loop;
    InetAddress listenAddr(9981);
    SudokuServer server(&loop,listenAddr);
    server.start();
    loop.loop();
}