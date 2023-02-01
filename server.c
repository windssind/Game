#include<stdio.h>
#include<netdb.h>
#include<stdlib.h>
#include<string.h>
#include"SDL2/SDL.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#define PORT "8080"
#define SERVER_IP NULL
#define MaxExcutable 100
#define PlayerNum 2
int socket_listen;// 监听的套接字
int Player_socket[PlayerNum];// 服务于 player 的套接字
int recvbytes,sendbytes;
fd_set set;
void WaitForConnection();
void ExchangeMessage();
int main(){
    WaitForConnection();
    ExchangeMessage();
    close(socket_listen);
}
void WaitForConnection(){
    struct addrinfo *result,hints;
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
    char Established[MaxExcutable];
    for(int i=0;i<PlayerNum;i++){//发送连接成功的消息
        sprintf(Established,"Connection Established,you are the Player%c",i+1+'0');
        sendbytes=send(Player_socket[i],Established,strlen(Established)+1,0);// strlen+1，可以把/0也穿过去
        printf("%d",sendbytes);
        if(sendbytes==-1){
            fprintf(stderr,"Though Connection Established,the good news failed");
            exit(1);
        }
    }
    
}

void ExchangeMessage(){
    FD_ZERO(&set);
    for(int i=0;i<PlayerNum;i++){
        FD_SET(Player_socket[i],&set);
    }
    fd_set readset=set;
    int buf;
    while(1){
        if(select(Player_socket[0]>Player_socket[1]?Player_socket[0]+1:Player_socket[1]+1,&readset,NULL,NULL,NULL)>0){
            for(int i=0;i<PlayerNum;i++){
                if(FD_ISSET(Player_socket[i],&set)){
                    recvbytes=recv(Player_socket[i],&buf,sizeof(int),0);
                    if(recvbytes==0){
                        fprintf(stderr,"Player%d disconnected\n ",i+1);
                        for(int i=0;i<PlayerNum;i++){
                            close(Player_socket[i]);
                        }
                        break;
                    }
                    if(recvbytes==-1){
                        fprintf(stderr,"recv Player%d fail\n",~i+1);
                    }
                    sendbytes=send(Player_socket[i==0?1:0],&buf,sizeof(int),0);
                    if(sendbytes==0){
                        fprintf(stderr,"Player%d\n disconnected",i+1);
                        for(int i=0;i<PlayerNum;i++){
                            close(Player_socket[i]);
                        }
                        break;
                    }
                    if(sendbytes==-1){
                        fprintf(stderr,"send to Player%d fail\n",~i+1);
                    }
                    
                    memset(&buf,0,sizeof(int));
                }
            }
        }
    }
}