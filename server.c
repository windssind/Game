#include<stdio.h>
#include<netdb.h>
#include<math.h>
#include<stdbool.h>
#define MaxExcutable 100
#define Block 50
#define Board_Vx 10
#define Board_Vy 10
#define Board_h 15
#define Board_w 170
#define Board_start_x Block*4
#define Board_start_y Block*14
#define Ball_radius 13
#define Col 15
#define Row 8
#define UP 1
#define LEFT 2
#define DOWN 3
#define RIGHT 4
#define BoardColorChange 5
#define DisConnected 6
#define LaunchBall 7
#define MaxHp 4
#define FPS 30
#undef main
#include"SDL2/SDL.h"
#include"SDL2/SDL_image.h"
#include"SDL2/SDL_ttf.h"
#include<time.h>
#include<math.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include<unistd.h>
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
typedef struct Brick{
    enum Element element;
    int HP;
    int status;// 0代表这块砖不存在，1代表存在
}Brick;
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
    int NO;//
    bool HaveNewBallCreated;
}Board;
typedef struct Location{
    int x;
    int y;
}Location;

typedef struct Bullet{
    int x;
    int y;
    int dy;
    int BulletLength;
    int BulletDistance;
    int BulletNum;
    int status;
}Bullet;// Bullet[0]作为计数的Bullet
typedef struct BallMessage{
    int BallNum;
    int x;
    int y;
    int dx;
    int dy;
}BallMessage;
typedef struct Message{
    Ball map[Row+2][Col+2];
    Board board[2];
    BallMessage ballMessage[2][4];
}Message;
Board board[2];// two board
int socket_listen;// 监听的套接字
int Player_socket[PlayerNum];// 服务于 player 的套接字
void WaitForConnection();
void ExchangeMessage();
void AnalyseMSG(int NO,int instruction);
void RecvMSG(int NO,int *MSG);
void CreatThread();
void CopyMap(Message *MSG);
void CopyBoard(Message *MSG);
void CopyBall(Message *MSG);
void AnalyseMSG(int NO,int instruction);
int Thread_send(void *data);
int Thread_listen(void *data);
void SendMSG(int NO,Message *MSG);
void Initgame();
void InitBoard();
void InitBall();
void InitMap();
Ball map[Row+2][Col+2];
Board board[PlayerNum];
int main(){
    WaitForConnection();
    CreatThread();
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

void CreatThread(){
    Thread_listen(NULL);
    Thread_send(NULL);
}

int Thread_listen(void *data){
    struct timeval interval;
    interval.tv_sec=0;
    interval.tv_usec=30000;
    fd_set set;
    FD_ZERO(&set);
    for(int i=0;i<PlayerNum;i++){
        FD_SET(Player_socket[i],&set);
    }
    int MSG;
    while(1){
        fd_set readset=set;
        if(select(Player_socket[0]>Player_socket[1]?Player_socket[0]+1:Player_socket[1]+1,&readset,NULL,NULL,&interval)>0){
            for(int i=0;i<PlayerNum;i++){
                if(FD_ISSET(Player_socket[i],&readset)){
                    RecvMSG(i,&MSG);
                    // recv then analyse        
                    AnalyseMSG(i,MSG);
                }else{
                    board[i].dx=0;
                    board[i].dy=0;
                }
            }
        }else{
            for(int i=0;i<PlayerNum;i++){
                board[i].dx=0;
                board[i].dy=0;
            }// no message,speed equals zero
        }
    }
}

 void AnalyseMSG(int NO,int instruction){
    switch(instruction){
        case UP:
            board[NO].dy=-Board_Vy;
            board[NO].y+=board[NO].dy;
            break;
        case DOWN:
            board[NO].dy=Board_Vy;
            board[NO].y+=board[NO].dy;
            break;
        case LEFT:
            board[NO].dx=-Board_Vx;
            board[NO].x+=board[NO].dx;
            break;
        case RIGHT:
            board[NO].dx=Board_Vx;
            board[NO].x+=board[NO].dx;
            break;
        default:
            break;
    }
 }

 void RecvMSG(int NO,int *MSG){
    int recvbytes=recv(Player_socket[NO],MSG,sizeof(int),MSG_DONTWAIT);
    if(recvbytes==0){
        fprintf(stderr,"Player%d disconnected\n ",NO+1);
        for(int i=0;i<PlayerNum;i++){
            close(Player_socket[i]);
        }
    }else if(recvbytes==-1){
        fprintf(stderr,"recv Player%d fail\n",NO==0?1:2);
    }else{
        printf("recvbytes=%d\n",recvbytes);
    }
 }

 int Thread_send(void *data){
    Message MSG;
    CopyMap(&MSG);
    CopyBoard(&MSG);
    CopyBall(&MSG);
    for(int i=0;i<PlayerNum;i++){
        SendMSG(i,&MSG);
    }
    
 }

 void SendMSG(int NO,Message *MSG){
    int sendbytes=send(Player_socket[NO],MSG,sizeof(Message),MSG_DONTWAIT);
    if(sendbytes==0){
        fprintf(stderr,"Player%d\n disconnected",NO+1);
        for(int i=0;i<PlayerNum;i++){
            close(Player_socket[i]);
        }
    }else if(sendbytes==-1){
        fprintf(stderr,"send to Player%d fail\n",NO+1);
    }else{
        printf("snedbytes=%d\n",sendbytes);
    }
 }

 void CopyMap(Message *MSG){
    for(int i=0;i<Row+2;i++){
        for(int j=0;j<Col+2;j++){
            MSG->map[i][j]=map[i][j];
        }
    }
 }

 void CopyBoard(Message *MSG){
    for(int i=0;i<PlayerNum;i++){
        MSG->board[i]=board[i];
    }
 }

 void CopyBall(Message *MSG){
    for(int i=0;i<PlayerNum;i++){
        Ball *tmp=board[i].HeadNode->next;
        for(int j=0;j<board[i].BallNum;j++){
            MSG->ballMessage[i][j].BallNum=board[i].BallNum;
            MSG->ballMessage[i][j].dx=tmp->dx;
            MSG->ballMessage[i][j].dy=tmp->dy;
            MSG->ballMessage[i][j].x=tmp->x;
            MSG->ballMessage[i][j].y=tmp->y;
        }
    }
 }

 void Initgame(){
    InitMap(level);
    InitBoard();
    InitBall();
 }

 void InitBoard(){
    for(int i=0;i<PlayerNum;i++){
        board[i].dx=0;
        board[i].dy=0;
        board[i].element=4;
        board[i].h=Board_h;
        board[i].w=Board_w;
        board[i].HeadNode=(Ball *)malloc(sizeof (Ball));
        board[i].HeadNode->next=NULL;
        board[i].BallNum=0;
        board[i].CountPower_NewBall=0;
        board[i].CountPower_Bullet=0;
        board[i].CountPower_Wall=0;
        board[i].y=Board_start_y;
        board[i].HaveNewBallCreated=false;
    }
}

void InitBall(){
    for(int i=0;i<PlayerNum;i++){
        Ball *NewBall=CreatBall(board[i].HeadNode,board[i].x,board[i].y-Ball_radius*2,&board[i]);
    }
}

void InitMap(int level){
    switch (level){
    case 1:
        InitMap_1();
        break;
    case 2:
        InitMap_2();
        break;
    default:
        break;
    }
    return ;
}