#include<netdb.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#define SERVER_IP NULL//"192.168.31.84"/*对于getaddrinfo函数来说,如果没有确定要监听哪一个ip地址，那name填NULL即可，就不会获取固定的ip地址了,这个函数就重要是把地址信息转化为套接字结构*/
#define PORT "8080"
#define MaxExcutable 32
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
int main(){
    //第一步，创建地址结构
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
    int socket_listen=socket(result->ai_family,result->ai_socktype,result->ai_protocol);
    if(socket_listen==-1){
        fprintf(stderr,"creat socket failed\n");
        exit(1);
    }
    //第三步，将套接字和地址绑定
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
    //第五步，进行接受
    int socket_server=accept(socket_listen,NULL,NULL);// 第二个参数是客户端的地址 目前未知，用NULL
    if(socket_server==-1){//这个最后两个参数如果填NULL，就代表来者不拒，而我这样写是限定了只能监听一个地址的
        fprintf(stderr,"accept failed\n");
        exit(1);
    }

    printf("Connection Established\n");
    char text[1024];
    int bytes_recv=recv(socket_server,text,strlen(text),0);
    printf("receive bytes=%d,%s",bytes_recv,text);
    return 0;
    
}