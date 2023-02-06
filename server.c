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
#define PlayNum 2
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
    Brick map[Row+2][Col+2];
    Board board[2];
    BallMessage ballMessage[2][4];
}Message;
Board board[2];// two board
int socket_listen;// 监听的套接字
int Player_socket[PlayNum];// 服务于 player 的套接字
void WaitForConnection();
void ExchangeMessage();
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
void InitMap(int level);
void HitBoard(Board *board,Ball *ball,int NO);
void HitBrick(Board *board,Ball *ball,int NO);
void HitWall(Board *board,Ball *ball,int NO);
Brick map[Row+2][Col+2];
Board board[PlayNum];
int level=1;
int Power_ID[3];
int BrickLeft=55;
int CountPower_NewBall=0;
int CountPower_Wall=0;
int CountPower_Bullet=0;
bool GetPower_Wall=false;
bool GetPower_Bullet=false;
//void PaintFont(const char *text,int x,int y,int w,int h);
Ball *CreatBall(Ball *HeadNode,int x,int y,Board *board);
void InitMap_1();
void InitMap_2();
void InitMap_3();
void InitMap_4();
void InitMap_5();
void InitGameObject();
void LimitBoard(Board *board);
void Launch(Board *board);
int  dist(int x1,int y1,int x2,int y2);
void ElementalAttack(Ball *ball,Location location);
void DestroyBrick();
void InitBall();
void InitBoard();
void GetPower();
void AdjustBoardLocation(int operation,Board *board,int times);
void PrintFont(const char*text);
Uint32 VanishPower_Wall(Uint32 interval,void *param);
Uint32 VanishPower_Bullet(Uint32 interval,void *param);
Uint32 CreatAndMoveAndHitCheckBullet(Uint32 interval,void *param);
void Draw(SDL_Surface *surface,int x,int y,int w,int h);
void HitBrick(Board *board,Ball *ball,int NO);
void ChangeColor(Ball *ball,Location location);
void ChooseMod();
bool IsGameOver();
void DeleteBall(Ball *HeadNode,Ball *DesertedBall,Board *board);
void HitBoard(Ball *ball,Board *board);
/*Uint32 AnalyseMSG(Uint32 interval,void *param);*/
int MoveBoard(void *data);
int MoveBall(void *data);
const Uint8 *KeyValue;
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
    for(int i=0;i<PlayNum;i++){
        Player_socket[i]=accept(socket_listen,NULL,NULL);// 第二个参数是客户端的地址 目前未知，用NULL
        if(Player_socket[i]==-1){//这个最后两个参数如果填NULL，就代表来者不拒，而我这样写是限定了只能监听一个地址的
        fprintf(stderr,"accept Player[%d] failed\n",i+1);
        exit(1);
        }
        printf("Connection with Player%d successfully\n",i+1);
    }
    int NO=1;
    for(int i=1;i<PlayNum+1;i++,NO++){//发送连接成功的消息
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
    for(int i=0;i<PlayNum;i++){
        FD_SET(Player_socket[i],&set);
    }
    Message buf;
    while(1){
        fd_set readset=set;
        if(select(Player_socket[0]>Player_socket[1]?Player_socket[0]+1:Player_socket[1]+1,&readset,NULL,NULL,NULL)>0){
            printf("enter\n");
            for(int i=0;i<PlayNum;i++){
                if(FD_ISSET(Player_socket[i],&readset)){
                    recvbytes=recv(Player_socket[i],&buf,sizeof(Message),0);
                    if(recvbytes==0){
                        fprintf(stderr,"Player%d disconnected\n ",i+1);
                        for(int i=0;i<PlayNum;i++){
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
                        for(int i=0;i<PlayNum;i++){
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
    for(int i=0;i<PlayNum;i++){
        FD_SET(Player_socket[i],&set);
    }
    int MSG;
    while(1){
        fd_set readset=set;
        if(select(Player_socket[0]>Player_socket[1]?Player_socket[0]+1:Player_socket[1]+1,&readset,NULL,NULL,&interval)>0){
            for(int i=0;i<PlayNum;i++){
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
            for(int i=0;i<PlayNum;i++){
                board[i].dx=0;
                board[i].dy=0;
            }// no message,speed equals zero
        }
    }
}

 void AnalyseMSG(int NO,int instruction){
    switch(instruction){
        case UP:
            AdjustBoardLocation(UP,&board[NO],1);
            break;
        case DOWN:
            AdjustBoardLocation(DOWN,&board[0],1);
            break;
        case LEFT:
            AdjustBoardLocation(LEFT,&board[0],1);
            break;
        case RIGHT:
            AdjustBoardLocation(RIGHT,&board[0],1);
            break;
        default:
            break;
    }
 }

 void RecvMSG(int NO,int *MSG){
    while(1){
        int recvbytes=recv(Player_socket[NO],MSG,sizeof(int),MSG_DONTWAIT);
        if(recvbytes==0){
        fprintf(stderr,"Player%d disconnected\n ",NO+1);
        for(int i=0;i<PlayNum;i++){
            close(Player_socket[i]);
        }
        }else if(recvbytes==-1){
            fprintf(stderr,"recv Player%d fail\n",NO==0?1:2);
        }else{
        printf("recvbytes=%d\n",recvbytes);
        }
    }
 }

 int Thread_send(void *data){
    while(1){
        int first=clock();
        Message MSG;
        CopyMap(&MSG);
        CopyBoard(&MSG);
        CopyBall(&MSG);
        for(int i=0;i<PlayNum;i++){
            SendMSG(i,&MSG);
        }
        SDL_Delay(FPS-(clock()-first)/CLOCKS_PER_SEC*1000);
    }
}

 void SendMSG(int NO,Message *MSG){
    int sendbytes=send(Player_socket[NO],MSG,sizeof(Message),MSG_DONTWAIT);
    if(sendbytes==0){
        fprintf(stderr,"Player%d\n disconnected",NO+1);
        for(int i=0;i<PlayNum;i++){
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
    for(int i=0;i<PlayNum;i++){
        MSG->board[i]=board[i];
    }
 }

 void CopyBall(Message *MSG){
    for(int i=0;i<PlayNum;i++){
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
    for(int i=0;i<PlayNum;i++){
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
    for(int i=0;i<PlayNum;i++){
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

//一直重复a的原因：getkeyboardstate函数是以事件为基础的，不删除事件，这个按下了a就会一直在消息队列里


int  MoveBall(void *data){//有三种碰撞检测，撞边界，撞板，撞砖块
while(1){
    int first=clock();
    for(int i=0;i<PlayNum;i++){
        Ball *tmp=board[i].HeadNode->next;
        while(tmp!=NULL){
            HitWall(&board[i],tmp,i);
            HitBoard(&board[i],tmp,i);
            HitBrick(&board[i],tmp,i);
            tmp->x+=tmp->dx;
            tmp->y+=tmp->dy;
            if(tmp->dx==0&&tmp->dy==0){
                tmp->x=board[i].x+board[i].w/2;
                tmp->y=board[i].y-tmp->radius*2;
            }
            DestroyBrick();
            tmp=tmp->next;// 有bug，如果delete了，就会有问题
        }
        GetPower();
    }
    SDL_Delay(FPS-(clock()-first)/CLOCKS_PER_SEC*1000);
    }
}
void GetPower(){
    for(int i=0;i<PlayNum;i++){
        if(board[i].CountPower_Wall>=10){// 长时间的
        GetPower_Wall=true;
        Power_ID[1]=SDL_AddTimer(5000,VanishPower_Wall,NULL);
        board[i].CountPower_Wall=0;
        break;
        }
    }
    
    /*if(BrickLeft<=5&&GetPower_Bullet==false){//场上方块数量很少
        GetPower_Bullet=true;
        Power_ID[2]=SDL_AddTimer(60,CreatAndMoveAndHitCheckBullet,NULL);
        Power_ID[3]=SDL_AddTimer(5000,VanishPower_Bullet,NULL);
    }*/
 }
Ball *CreatBall(Ball *HeadNode,int x,int y,Board *board){//用链表结构
    Ball *newBall=(Ball*)malloc(sizeof(Ball));
    newBall->x=x;
    newBall->y=y;
    newBall->dx=0;
    newBall->dy=0;// 还没决定好
    newBall->radius=Ball_radius;
    newBall->element=normal;
    //次序不重要，直接用头插法就行了
    Ball *tmp=board->HeadNode->next;
    board->HeadNode->next=newBall;
    newBall->next=tmp;
    board->BallNum++;
    return newBall;
}
void DeleteBall(Ball *HeadNode,Ball *DesertedBall,Board *board){
    if(board->BallNum==1){
        return ;
    }else{
        Ball *tmp=HeadNode;
        while(tmp->next!=DesertedBall){
            tmp=tmp->next;
        }
        // 到这一步
        tmp->next=DesertedBall->next;
        free(DesertedBall);
        board->BallNum--;
    }
}

void InitMap_1(){
    for(int i=0;i<=Row+1;i++){
        for(int j=0;j<=Col+1;j++){
            if(i==0||i==Row+1||j==0||j==Col+1){
                map[i][j].status=0;
            }else{
                map[i][j].HP=MaxHp;
                map[i][j].element=rand()%5;
                map[i][j].status=1;
            }
        }
    }
}


// 可以使用Timer函数来代替SDL——delay(是必须，用SDL_Addtimer),update,move
// 判断元素反应，用音效来区分


void ElementalAttack(Ball *ball,Location location){
    // 火和水，火和雷，火和冰双倍
    // 水和雷 九宫格伤害
    // 雷和冰，直接去世
    Brick *ToBeAttacked=&map[location.y][location.x];
    if(ToBeAttacked->element==fire){// 砖块是火元素
        if(ball->element==water||ball->element==thunder||ball->element==ice){//元素反应
            ToBeAttacked->HP-=2;
        }else{// 火火相碰
            ToBeAttacked->HP-=1;
        }
    }else if(ToBeAttacked->element==water){// 砖块是水元素
        if(ball->element==fire){
            ToBeAttacked->HP-=2;
        }else if(ball->element==thunder){
            for(int i=location.y-1;i<=location.y+1;i++){
                for(int j=location.x-1;j<=location.x+1;j++){
                    if(map[i][j].status==1){
                        map[i][j].HP-=1;
                    }
                }
            }
        }else{
            ToBeAttacked->HP-=1;
        }
    }else if(ToBeAttacked->element==thunder){// 砖块是雷元素
        if(ball->element==fire){
            ToBeAttacked->HP-=2;
        }else if(ball->element==water){
            for(int i=location.y-1;i<=location.y+1;i++){
                for(int j=location.x-1;j<=location.x+1;j++){
                    if(map[i][j].status==1){
                        map[i][j].HP-=1;
                    }
                }
            }
        }else if(ball->element==ice){
            ToBeAttacked->HP-=4;
        }else{
            ToBeAttacked->HP-=1;
        }
    }else if(ToBeAttacked->element==ice){
        if(ball->element==fire){
            ToBeAttacked->HP-=2;
        }else if(ball->element==thunder){
            ToBeAttacked->HP-=4;
        }else{
            ToBeAttacked->HP-=1;
        }
    }else if(ToBeAttacked->element==normal){
        ToBeAttacked->element=ball->element;
        ToBeAttacked->HP-=1;
    }
}

void DestroyBrick(){
    for(int i=1;i<=Row;i++){
        for(int j=1;j<=Col;j++){
            if(map[i][j].HP<=0){
                map[i][j].status=0;
            }
        }
    }
}

void HitBrick(Board *board,Ball *ball,int NO){
    Location location;
    int x=ball->x;
    int y=ball->y;
    int col=x/Block+1;
    int row=y/Block+1;
    if(col>=0&&col<=Col+1&&row>=0&&row<=Row+1){// 有碰撞的可能
        if(ball->x>=Block*(col-1)&&ball->x<=Block*col&&ball->y-ball->radius<=Block*(row-1)&&map[row-1][col].status==1){//上边碰撞
            location.x=col;
            location.y=row-1;
            ball->dy=-ball->dy;
        }else if(ball->x>=Block*(col-1)&&ball->x<=Block*col&&ball->y+ball->radius>=Block*(row)&&map[row+1][col].status==1){// 下边碰撞
            location.x=col;
            location.y=row+1;
            ball->dy=-ball->dy;
        }else if(ball->y>=Block*(row-1)&&ball->y<=Block*row&&ball->x-ball->radius<=Block*(col-1)&&map[row][col-1].status==1){
            location.x=col-1;
            location.y=row;
            ball->dx=-ball->dx;
        }else if(ball->y>=Block*(row-1)&&ball->y<=Block*row&&ball->x+ball->radius>=Block*(col+1)&&map[row][col+1].status==1){
            location.x=col+1;
            location.y=row;
            ball->dx=-ball->dx;
        }else if(dist((row-1)*Block,(col-1)*Block,ball->y,ball->x)<=(ball->radius+3)&&map[row-1][col-1].status==1){// zuo shang jiao
            location.x=col-1;
            location.y=row-1;
            ball->dy=-ball->dy;
        }else if(dist((row-1)*Block,col*Block,ball->y,ball->x)<=(ball->radius+3)&&map[row-1][col+1].status==1){// zuo shang jiao
            location.x=col+1;
            location.y=row-1;
            ball->dy=-ball->dy;
        }else if(dist(row*Block,(col-1)*Block,ball->y,ball->x)<=(ball->radius+3)&&map[row+1][col-1].status==1){// zuo shang jiao
            location.x=col-1;
            location.y=row+1;
            ball->dy=-ball->dy;
        }else if(dist(row*Block,col*Block,ball->y,ball->x)<=(ball->radius+3)&&map[row+1][col+1].status==1){// zuo shang jiao
            location.x=col+1;
            location.y=row+1;
            ball->dy=-ball->dy;
        }
        else{
            return ;
        }
        ElementalAttack(ball,location);
        /*GetPower();*/
        /*这个地方误差值+3的引入很巧妙，肉眼无法观察，但完美解决了碰撞问题*/
    }
}
void HitBoard(Board *board,Ball *ball,int NO){
        if(ball->dy>0&&ball->x>=board[NO].x&&ball->x<=board[NO].x+board[NO].w&&ball->y+ball->radius>=board[NO].y-10&&ball->y+board[NO].HeadNode->next->radius<=board[NO].y+board[NO].h*2){
            ball->dx=ball->dx+board[NO].dx/3;
            ball->dy=-ball->dy+board[NO].dy/6;
            ball->element=board[NO].element;
            board[NO].CountPower_NewBall++;
            if(board[NO].CountPower_NewBall>=4&&board[NO].BallNum<=3){
                CreatBall(board[NO].HeadNode,board[NO].x+board[NO].w/2,board[NO].y-ball->radius*2,&board[NO]);
                board[NO].HaveNewBallCreated=true;
                board[NO].CountPower_NewBall=0;
            }
        }
            return ;
    }

void InitGameObject(){
    InitMap(level);
    InitBoard();
    InitBall();
}


int dist(int x1,int y1,int x2,int y2){
    return sqrt(abs((x1-x2)*(x1-x2))+((y1-y2)*(y1-y2)));
}


void Launch(Board *board){
    Ball *tmp=board->HeadNode->next;
    while(tmp!=NULL){
        if(tmp->dx==0&&tmp->dy==0){
            tmp->dx=0;
            tmp->dy=-10;
            return ;
        }
        tmp=tmp->next;
    }
}

void AdjustBoardLocation(int operation,Board *board,int times){
    if(operation==RIGHT){
        if(board->x+board->w<=board[1].x&&board->x+Board_Vx*times+board->w>=board[1].x&&((board->y+board->h>board[1].y&&board->y<=board[1].y)||board->y>=board[1].y&&board->y<board[1].y+board[1].h)){
            board->x=board[1].x-board->w;
        }else{
            board->x+=Board_Vx*times;
        }
    }else if(operation==LEFT){
        if(board->x>=board[1].x+board[1].w&&board->x-Board_Vx*times<=board[1].x+board->w&&((board->y+board->h>board[1].y&&board->y<=board[1].y)||board->y>=board[1].y&&board->y<board[1].y+board[1].h)){
            board->x=board[1].x+board[1].w;
        }else{
            board->x-=Board_Vx*times;
        }
    }else if(operation==DOWN){
        if(board->y<=board[1].y&&board->y+board->h+Board_Vy*times>=board[1].y&&((board->x+board->w>board[1].x&&board->x<=board[1].x)||board->x>=board[1].x&&board->x<board[1].x+board[1].w)){
            board->y=board[1].y-board->h;
        }else{
            board->y+=Board_Vy*times;
        }
    }else if(operation==UP){
        if(board->y>=board[1].y&&board->y-Board_Vy*times<=board[1].y+board[1].h&&((board->x+board->w>board[1].x&&board->x<=board[1].x)||board->x>=board[1].x&&board->x<board[1].x+board[1].w)){
            board->y=board[1].y+board->h;
        }else{
            board->y-=Board_Vy*times;
        }
    }
    if(board->x<=0){
            board->x=0;
            board->dx=0;
        }
        if(board->x+board->w>=Block*15){
            board->x=Block*15-board->w;
            board->dx=0;
        }
        if(board->y<=Block*14){
            board->y=Block*14;
            board->dy=0;
        }
        if(board->y+board->h>=Block*18){
            board->y=Block*18-board->h;
            board->dy=0;
        }
}

void InitMap_2(){

}

Uint32 VanishPower_Wall(Uint32 interval,void *param){
    GetPower_Wall=false;
    SDL_RemoveTimer(Power_ID[1]);
    return interval;
}

void HitWall(Board *board,Ball *ball,int NO){
    Ball *tmp=ball;
            if(tmp->x-tmp->radius<=0||tmp->x+tmp->radius>=Window_Width){// 左边碰撞或者右边碰撞
                tmp->dx=-tmp->dx;
            }else if(tmp->y-tmp->radius<=0){// 上边碰撞
                tmp->dy=-tmp->dy;
            }else if(tmp->y+tmp->radius>=Window_Depth-Block*0.4){// 下边碰撞
                if(GetPower_Wall){// 用playnum来维护有多少个挡板
                    tmp->dy=-tmp->dy;
                }else{// 没有墙壁
                    if(board[NO].BallNum==1){// 
                        board->HeadNode->next->dx=0;
                        board->HeadNode->next->dy=0;
                        board->HeadNode->next->element=board[NO].element;
                        board->HeadNode->next->x=board[NO].x+board[NO].w/2;
                        board->HeadNode->next->y=board[NO].y-board->HeadNode->next->radius*2;
                    }else{
                        DeleteBall(board[NO].HeadNode,tmp,&board[NO]);
                    }
                    CountPower_Wall++;
                }
                board[NO].CountPower_NewBall=0; 
            }         
}
