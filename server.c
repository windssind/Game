#include<stdio.h>
#include<netdb.h>
#include<stdlib.h>
#include<string.h>
#include"SDL2/SDL.h"
#include <unistd.h>
#include<stdbool.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#define PORT "8080"
#define SERVER_IP NULL
#define MaxExcutable 100
#define PlayerNum 2
enum Element{
    fire,
    water,
    thunder,
    ice,
    normal
};
typedef struct Ball{
    int dx;
    int dy;
    int x;
    int y;
    int radius;
    enum Element element;
    struct Ball *next;
}Ball;
typedef struct Board{
    enum Element element;
    Ball *HeadNode;
    int BallNum;
    int CountPower_Bullet;
    int CountPower_NewBall;
    int CountPower_Wall;
    int dx;
    int dy;
    int x;
    int y;
    int w;
    int h;
    int NO;
}Board;
typedef struct Message{
    int board_x;
    int board_y;
    int board_dx;
    int board_dy;
    int CountPower_NewBall;
    enum Element element; 
    bool isLaunch;
    bool isChangeColor;
    bool isCreatNewBall;
}Message;
int socket_listen;// 监听的套接字
int Player_socket[PlayerNum];// 服务于 player 的套接字
void WaitForConnection();
void ExchangeMessage();
int main(){
    WaitForConnection();
    ExchangeMessage();
    close(socket_listen);
}
void WaitForConnection(){
    struct addrinfo *result,hints;
    int recvbytes,sendbytes;
    memset(&hints,0,sizeof hints);// 如果没有这一句话，就会ai_socktype not supported
    hints.ai_family=AF_INET;
    hints.ai_flags=AI_PASSIVE;//只用于监听
    hints.ai_socktype=SOCK_STREAM;
    if(getaddrinfo(SERVER_IP,PORT,&hints,&result)!=0){
        fprintf(stderr,"getaddrinfo failed()\n");
        printf("%s",gai_strerror(getaddrinfo(SERVER_IP,PORT,&hints,&result)));
        exit(1);
    }
    //第二步，创建套接字结构
    socket_listen=socket(result->ai_family,result->ai_socktype,result->ai_protocol);
    if(socket_listen==-1){
        fprintf(stderr,"creat listen socket failed\n");
        exit(1);
    }

    if(bind(socket_listen,result->ai_addr,result->ai_addrlen)!=0){
        fprintf(stderr,"bind failed\n");
        exit(1);
    }
    // 绑定之后，就可以free了
    freeaddrinfo(result);
    //第四步，进行监听
    if(listen(socket_listen,MaxExcutable)!=0){
        fprintf(stderr,"listen failed\n");
        exit(1);
    }
    for(int i=0;i<PlayerNum;i++){
        Player_socket[i]=accept(socket_listen,NULL,NULL);// 第二个参数是客户端的地址 目前未知，用NULL
        if(Player_socket[i]==-1){//这个最后两个参数如果填NULL，就代表来者不拒，而我这样写是限定了只能监听一个地址的
        fprintf(stderr,"accept Player[%d] failed\n",i+1);
        exit(1);
        }
        printf("Connection with Player%d successfully\n",i+1);
    }
    int NO=1;
    for(int i=1;i<PlayerNum+1;i++,NO++){//发送连接成功的消息
        sendbytes=send(Player_socket[i-1],&NO,sizeof(int),0);// strlen+1，可以把/0也穿过去
        printf("sendbytes=%d\n",sendbytes);
        if(sendbytes==-1){
            fprintf(stderr,"Though Connection Established,the good news failed");
            exit(1);
        }
    }
    
}

void ExchangeMessage(){
    int recvbytes,sendbytes;
    fd_set set;
    FD_ZERO(&set);
    for(int i=0;i<PlayerNum;i++){
        FD_SET(Player_socket[i],&set);
    }
    Message buf;
    while(1){
        fd_set readset=set;
        if(select(Player_socket[0]>Player_socket[1]?Player_socket[0]+1:Player_socket[1]+1,&readset,NULL,NULL,NULL)>0){
            printf("enter\n");
            for(int i=0;i<PlayerNum;i++){
                if(FD_ISSET(Player_socket[i],&readset)){
                    recvbytes=recv(Player_socket[i],&buf,sizeof(Message),0);
                    if(recvbytes==0){
                        fprintf(stderr,"Player%d disconnected\n ",i+1);
                        for(int i=0;i<PlayerNum;i++){
                            close(Player_socket[i]);
                        }
                        break;
                    }else if(recvbytes==-1){
                        fprintf(stderr,"recv Player%d fail\n",i==0?1:2);
                    }else{
                        printf("recvbytes=%d\n",recvbytes);
                    }
                    sendbytes=send(Player_socket[i==0?1:0],&buf,sizeof(Message),0);
                    if(sendbytes==0){
                        fprintf(stderr,"Player%d\n disconnected",i+1);
                        for(int i=0;i<PlayerNum;i++){
                            close(Player_socket[i]);
                        }
                        break;
                    }else if(sendbytes==-1){
                        fprintf(stderr,"send to Player%d fail\n",~i+1);
                    }else{
                        printf("snedbytes=%d\n",sendbytes);
                    }
                    
                    memset(&buf,0,sizeof(Message));
                }
            }
        }
    }
}