#include<stdio.h>
#include<netdb.h>
#include<math.h>
#include<stdbool.h>
#define MaxExcutable 100
#define Block 60
#define Board_Vx 10
#define Board_Vy 10
#define Board_h 15
#define Board_w 170
#define Board_start_x Block*4
#define Board_start_y Block*12
#define Ball_radius 18
#define Col 18
#define Row 8
#define UP 1
#define LEFT 2
#define DOWN 3
#define RIGHT 4
#define BoardColorChange 5
#define DisConnected 6
#define LaunchBall 7
#define QuitGame 8
#define Shift 9
#define MaxHp 4
#define FPS 25
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
    int CountPower_NewBall;
    int dx;
    int dy;
    int x;
    int y;
    int w;
    int h;
    int NO;//
    bool HaveNewBallCreated;
    bool HaveNewBallDeleted;
}Board;
typedef struct Location{
    int x;
    int y;
}Location;

typedef struct BallMessage{
    int x;
    int y;
    int dx;
    int dy;
    enum Element element;
    bool HaveNewBallCreated;
    bool HaveNewBallDeleted;
}BallMessage;
typedef  struct MusicMessage{
    bool Music_HitBoard;
    bool Music_CreatNewBall;
}MusicMessage;
typedef struct Message{
    Brick map[Row+2][Col+2];
    Board board[2];
    BallMessage ballMessage[2][4];
    MusicMessage musicMessage;
    bool HaveNewBallCreated;
    bool HaveNewBallDeleted;
    bool IsNextLevel;
    bool IsWin;
    bool IsLose;
    bool GetPower_Wall;
    time_t TimeLeft;
    int Score;
}Message;
typedef struct MessageFromClient{
    bool IsUP;
    bool IsDown;
    bool IsLeft;
    bool IsRight;
    bool IsQuitGame;
    int  ChangeColor;
    bool IsShift;
    bool IsLaunchBall;
}MessageFromClient;

Board board[2];// two board
int socket_listen;// 监听的套接字
int Player_socket[PlayNum];// 服务于 player 的套接字
void WaitForConnection();
void RecvMSG(int NO,MessageFromClient *MSG);
void CopyMap(Message *MSG);
void CopyBoard(Message *MSG);
void CopyBall(Message *MSG);
void AnalyseMSG(int NO,MessageFromClient *instruction);
int Thread_send(void *data,time_t TimeLeft,Message MSG);
int Thread_listen(void *data);
void SendMSG(int NO,Message *MSG);
void HitBoard(Board *board,Ball *ball,int NO,Message *MSG);
void HitBrick(Board *board,Ball *ball,int NO);
void HitWall(Board *board,Ball *ball,int NO);
void InitBall();
void InitBoard();
void InitMap(int level);
void ReInitGameobject(int level);
void InitGameObject();
void LimitBoard(Board *board);
void Server_Quit();
void BallNumChange(Message *MSG);
bool IsVictory();
bool NextLevel(time_t *BeginTime);
bool IsLose(time_t *BeginTime);
void Server_Win();
void Server_Lose();
void Server_NextLevel();
Brick map[Row+2][Col+2];
Board board[PlayNum];
int level=1;
int Score=0;
int Power_ID[3];
int BrickLeft=0;
int CountPower_Wall=0;
bool GetPower_Wall=false;
//void PaintFont(const char *text,int x,int y,int w,int h);
Ball *CreatBall(Ball *HeadNode,int x,int y,Board *board);
void InitMap_1();
void InitMap_2();
void InitMap_3();
void InitMap_4();
void InitMap_5();
void LimitBoard(Board *board);
void Launch(Board *board);
int  dist(int x1,int y1,int x2,int y2);
void ElementalAttack(Ball *ball,Location location);
void DestroyBrick();
void GetPower();
void AdjustBoardLocation(int operation,Board *board,int times);
Uint32 VanishPower_Wall(Uint32 interval,void *param);
void HitBrick(Board *board,Ball *ball,int NO);
void ChooseMod();
void DeleteBall(Ball *HeadNode,Ball *DesertedBall,Board *board);
/*Uint32 AnalyseMSG(Uint32 interval,void *param);*/
int MoveBoard(void *data);
int MoveBall(void *data,Message *MSG);
const Uint8 *KeyValue;
const int Window_Width=Block*18;
const int Window_Depth=Block*15;
int main(){
    InitGameObject();
    WaitForConnection();
    time_t BeginTime=time(NULL);
    while(1){
        printf("Brickleft=%d\n",BrickLeft);
        Message MSG;
        long int first=clock();
        MoveBall(NULL,&MSG);
        Thread_listen(NULL);
        Thread_send(NULL,240/PlayNum-(time(NULL)-BeginTime),MSG);
        if(IsVictory()) Server_Win();
        else if(NextLevel(&BeginTime))  Server_NextLevel();
        else if(IsLose(&BeginTime))  Server_Lose(NULL);
        if((FPS-(clock()-first)*1000/CLOCKS_PER_SEC)>0){
            SDL_Delay(FPS-(clock()-first)*1000/CLOCKS_PER_SEC);
        }
        memset(&MSG,0,sizeof(Message));
    }
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
        if(sendbytes==-1){
            fprintf(stderr,"Though Connection Established,the good news failed");
            exit(1);
        }
    }
    // init news
    for(int i=0;i<PlayNum;i++){
        board[i].HaveNewBallCreated=false;
        board[i].HaveNewBallDeleted=false;
    }
    Message MSG;
    MSG.IsWin=false;
    MSG.IsLose=false;
    MSG.IsNextLevel=false;
    BallNumChange(&MSG);
    CopyMap(&MSG);
    CopyBoard(&MSG);
    CopyBall(&MSG);
    for(int i=0;i<PlayNum;i++){
        SendMSG(i,&MSG);
    }
    printf("send MSG init win\n");
}

int Thread_listen(void *data){
    struct timeval interval;
    interval.tv_sec=0;
    interval.tv_usec=30;
    fd_set set;
    FD_ZERO(&set);
    for(int i=0;i<PlayNum;i++){
        FD_SET(Player_socket[i],&set);
    }
        MessageFromClient MSG;
        fd_set readset=set;
        if(select(Player_socket[0]>Player_socket[1]?Player_socket[0]+1:Player_socket[1]+1,&readset,NULL,NULL,&interval)>0){
            for(int i=0;i<PlayNum;i++){
                if(FD_ISSET(Player_socket[i],&readset)){
                    RecvMSG(i,&MSG);
                    // recv then analyse
                        AnalyseMSG(i,&MSG);
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

 void AnalyseMSG(int NO,MessageFromClient *instruction){
    //printf("SERVER:recv msg from NO=%d\n",NO);
    int times=(instruction->IsShift==true?2:1);
    if(instruction->IsUP){
        AdjustBoardLocation(UP,&board[NO],times);
    }if(instruction->IsDown){
        AdjustBoardLocation(DOWN,&board[NO],times);
    }if(instruction->IsLeft){
        AdjustBoardLocation(LEFT,&board[NO],times);
    }if(instruction->IsRight){
        AdjustBoardLocation(RIGHT,&board[NO],times);
    }if(instruction->IsQuitGame){
        Server_Quit();
    }if(instruction->IsLaunchBall){
        printf("launch board ready\n");
        Launch(&board[NO]);
    }if((!instruction->IsUP)&&(!instruction->IsDown)){
        board[NO].dy=0;
    }if((!instruction->IsLeft)&&(!instruction->IsRight)){
        board[NO].dx=0;
    }
    if(instruction->ChangeColor>=0&&instruction->ChangeColor<=4){
        board[NO].element=instruction->ChangeColor;
    }
}

 void RecvMSG(int NO,MessageFromClient *MSG){
        int recvbytes=recv(Player_socket[NO],MSG,sizeof(MessageFromClient),0);
        if(recvbytes==0){
        fprintf(stderr,"Player%d disconnected\n ",NO+1);
        for(int i=0;i<PlayNum;i++){
            close(Player_socket[i]);
        }
        }else if(recvbytes==-1){
            fprintf(stderr,"recv Player%d fail\n",NO==0?1:2);
        }
    }


 int Thread_send(void *data,time_t TimeLeft,Message MSG){
        BallNumChange(&MSG);
        CopyMap(&MSG);
        CopyBoard(&MSG);
        CopyBall(&MSG);
        MSG.IsWin=false;
        MSG.IsLose=false;
        MSG.IsNextLevel=false;
        MSG.GetPower_Wall=(GetPower_Wall==true?true:false);
        MSG.Score=Score;
        MSG.TimeLeft=TimeLeft;
        for(int i=0;i<PlayNum;i++){
            SendMSG(i,&MSG);
        }
    }

 void SendMSG(int NO,Message *MSG){
    int sendbytes=send(Player_socket[NO],MSG,sizeof(Message),MSG_DONTWAIT);
    if(sendbytes==0&&Player_socket[NO]!=-1){
        fprintf(stderr,"Player%d\n disconnected",NO+1);
        close(Player_socket[NO]);
        Player_socket[NO]=-1;
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
        //printf("board[%d].elem=%d\n",i,MSG->board[i].element);
    }
 }

 void CopyBall(Message *MSG){
    for(int i=0;i<PlayNum;i++){
        Ball *tmp=board[i].HeadNode->next;
        for(int j=0;j<board[i].BallNum;j++){
            MSG->ballMessage[i][j].dx=tmp->dx;
            MSG->ballMessage[i][j].dy=tmp->dy;
            MSG->ballMessage[i][j].x=tmp->x;
            MSG->ballMessage[i][j].y=tmp->y;
            MSG->ballMessage[i][j].element=tmp->element;
            tmp=tmp->next;
        }
    }
 }

 void Initgame(){
    InitMap(level);
    InitBoard();
    InitBall();
 }



void InitMap(int level){
    switch (level){
    case 1:
        InitMap_1();
        break;
    case 2:
        InitMap_2();
        printf("initmap2\n");
        break;
    case 3:
        InitMap_3();
        printf("initmap3\n");
        break;
    case 4:
        InitMap_4();
        break;
    case 5:
        InitMap_5();
        break;
    default:
        break;
    }
    return ;
}

//一直重复a的原因：getkeyboardstate函数是以事件为基础的，不删除事件，这个按下了a就会一直在消息队列里


int  MoveBall(void *data,Message *MSG){//有三种碰撞检测，撞边界，撞板，撞砖块
    for(int i=0;i<PlayNum;i++){
        Ball *tmp=board[i].HeadNode->next;
        while(tmp!=NULL){
            HitWall(board,tmp,i);
            HitBoard(board,tmp,i,MSG);
            HitBrick(board,tmp,i);
            if(tmp->dx==0&&tmp->dy==0){
                tmp->x=board[i].x+board[i].w/2;
                tmp->y=board[i].y-tmp->radius*2;
                tmp->element=board[i].element;
            }else{
                tmp->x+=tmp->dx;
                tmp->y+=tmp->dy;
            }
            DestroyBrick();
            tmp=tmp->next;// 有bug，如果delete了，就会有问题
        }
        GetPower();
    }
}
void GetPower(){
    for(int i=0;i<PlayNum;i++){
        if(CountPower_Wall>=3){// 长时间的
        GetPower_Wall=true;
        Power_ID[1]=SDL_AddTimer(5000,VanishPower_Wall,NULL);
        CountPower_Wall=0;
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
        Ball *tmp=HeadNode;
        while(tmp->next!=DesertedBall){
            tmp=tmp->next;
        }
        // 到这一步
        tmp->next=DesertedBall->next;
        free(DesertedBall);
        board->BallNum--;
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
            Score+=10;
            ToBeAttacked->HP-=3;
        }else{// 火火相碰
            Score+=3;
            printf("1\n");
            ToBeAttacked->HP-=1;
        }
    }else if(ToBeAttacked->element==water){// 砖块是水元素
        if(ball->element==fire){
            Score+=10;
            ToBeAttacked->HP-=3;
        }else if(ball->element==thunder){
            for(int i=location.y-1;i<=location.y+1;i++){
                for(int j=location.x-1;j<=location.x+1;j++){
                    if(map[i][j].status==1){
                        Score+=3;
                        printf("2\n");
                        map[i][j].HP-=2;
                    }
                }
            }
        }else{
            Score+=3;
            printf("3\n");
            ToBeAttacked->HP-=1;
        }
    }else if(ToBeAttacked->element==thunder){// 砖块是雷元素
        if(ball->element==fire){
            Score+=10;
            ToBeAttacked->HP-=3;
        }else if(ball->element==water){
            for(int i=location.y-1;i<=location.y+1;i++){
                for(int j=location.x-1;j<=location.x+1;j++){
                    if(map[i][j].status==1){
                        Score+=3;
                        printf("4\n");
                        map[i][j].HP-=2;
                    }
                }
            }
        }else if(ball->element==ice){
            Score+=10;
            ball->dy=-ball->dy;
            ToBeAttacked->HP-=4;
        }else{
            Score+=2;
            ToBeAttacked->HP-=1;
        }
    }else if(ToBeAttacked->element==ice){
        if(ball->element==fire){
            Score+=10;
            ToBeAttacked->HP-=3;
        }else if(ball->element==thunder){
            Score+=10;
            ball->dy=-ball->dy;
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
    for(int i=0;i<=Row+1;i++){
        for(int j=0;j<=Col+1;j++){
            if(map[i][j].status==1&&map[i][j].HP<=0){
                map[i][j].status=0;
                BrickLeft--;
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
    if(col>0&&col<=Col+1&&row>=0&&row<Row+1){// 有碰撞的可能
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
void HitBoard(Board *board,Ball *ball,int NO,Message *MSG){
    for(int i=0;i<PlayNum;i++){
        if(ball->dy>0&&ball->x>=board[i].x&&ball->x<=board[i].x+board[i].w&&ball->y+ball->radius>=board[i].y-10&&ball->y+board[i].HeadNode->next->radius<=board[i].y+board[i].h*2){
            ball->dx=ball->dx+board[i].dx/3;
            ball->dy=-ball->dy+board[i].dy/6;
            ball->element=board[i].element;
            board[i].CountPower_NewBall++;
            MSG->musicMessage.Music_HitBoard=true;
            if(board[i].CountPower_NewBall>=4&&board[i].BallNum<=3){
                CreatBall(board[i].HeadNode,board[i].x+board[i].w/2,board[i].y-ball->radius*2,&board[i]);
                board[i].HaveNewBallCreated=true;
                board[i].CountPower_NewBall=0;
            }else{
                board[i].HaveNewBallCreated=false;
            }

    }
    }
}


int dist(int x1,int y1,int x2,int y2){
    return sqrt(abs((x1-x2)*(x1-x2))+((y1-y2)*(y1-y2)));
}


void Launch(Board *board){
    Ball *tmp=board->HeadNode->next;
    while(tmp!=NULL){
        if(tmp->dx==0&&tmp->dy==0){
            printf("enter launchball\n");
            tmp->dx=0;
            tmp->dy=-10;
            return ;
        }
        tmp=tmp->next;
    }
}

void AdjustBoardLocation(int operation,Board *board_control,int times){
    int The_Other;
    The_Other=(board_control==&board[0]?1:0);
    if(operation==RIGHT){
        board_control->dx=Board_Vx*times;
        if(board_control->x+board_control->w<=board[The_Other].x&&board_control->x+Board_Vx*times+board_control->w>=board_control[The_Other].x&&((board_control->y+board_control->h>board[The_Other].y&&board_control->y<=board[The_Other].y)||board_control->y>=board[The_Other].y&&board_control->y<board[The_Other].y+board[The_Other].h)){
            board_control->x=board[The_Other].x-board_control->w;
        }else{
            board_control->x+=board_control->dx;
        }
    }else if(operation==LEFT){
        board_control->dx=-Board_Vx*times;
        if(board_control->x>=board[The_Other].x+board[The_Other].w&&board_control->x-Board_Vx*times<=board[The_Other].x+board_control->w&&((board_control->y+board_control->h>board[The_Other].y&&board_control->y<=board[The_Other].y)||board_control->y>=board[The_Other].y&&board_control->y<board[The_Other].y+board[The_Other].h)){
            board_control->x=board[The_Other].x+board[The_Other].w;
        }else{
            board_control->x+=board_control->dx;
        }
    }else if(operation==DOWN){
        board_control->dy=Board_Vy*times;
        if(board_control->y<=board[The_Other].y&&board_control->y+board_control->h+Board_Vy*times>=board[The_Other].y&&((board_control->x+board_control->w>board[The_Other].x&&board_control->x<=board[The_Other].x)||board_control->x>=board[The_Other].x&&board_control->x<board[The_Other].x+board[The_Other].w)){
            board_control->y=board[The_Other].y-board_control->h;
        }else{
            board_control->y+=board_control->dy;
        }
    }else if(operation==UP){
        board_control->dy=-Board_Vy*times;
        if(board_control->y>=board[The_Other].y&&board_control->y-Board_Vy*times<=board[The_Other].y+board[The_Other].h&&((board_control->x+board_control->w>board[The_Other].x&&board_control->x<=board[The_Other].x)||board_control->x>=board[The_Other].x&&board_control->x<board[The_Other].x+board[The_Other].w)){
            board_control->y=board[The_Other].y+board_control->h;
        }else{
            board_control->y+=board_control->dy;
        }
    }
    LimitBoard(board_control);
}

void InitMap_2(){
    for(int i=0;i<Row+2;i++){
        for(int j=0;j<Col+2;j++){
            if((j==2&&(i==4||i==5))||(j==3&&(i==3||i==6))||(j==4&&(i==2||i==7))){
                map[i][j].status=1;
                map[i][j].element=fire;
                map[i][j].HP=MaxHp;
                BrickLeft++;
            }else if((j==5&&(i==2||i==7))||(j==6&&(i==3||i==6))||(j==7&&(i==4||i==5))){
                map[i][j].status=1;
                map[i][j].element=water;
                map[i][j].HP=MaxHp;
                BrickLeft++;
            }else if(j==12&&(i>=2&&i<=8)){
                map[i][j].status=1;
                map[i][j].element=thunder;
                map[i][j].HP=MaxHp;
                BrickLeft++;
            }else if((j==13&&(i==2||i==5))||(j==14&&(i==2||i==5))||(j==14&&(i==3||i==4))){
                map[i][j].status=1;
                map[i][j].element=ice;
                map[i][j].HP=MaxHp;
                BrickLeft++;
            }else{
                map[i][j].status=0;
            }
    }
    printf("map2:brickleft=%d\n",BrickLeft);
};
}

Uint32 VanishPower_Wall(Uint32 interval,void *param){
    GetPower_Wall=false;
    SDL_RemoveTimer(Power_ID[1]);
    return interval;
}

void HitWall(Board *board,Ball *ball,int NO){
    Ball *tmp=ball;
            if((tmp->x-tmp->radius<=0&&tmp->dx<0)||(tmp->x+tmp->radius>=Window_Width&&tmp->dx>0)){// 左边碰撞或者右边碰撞
                tmp->dx=-tmp->dx;
            }else if(tmp->y-tmp->radius<=0&&tmp->dy<0){// 上边碰撞
                tmp->dy=-tmp->dy;
            }else if(tmp->y+tmp->radius>=Window_Depth-Block*0.6&&tmp->dy>0){// 下边碰撞
                if(GetPower_Wall){// 用playnum来维护有多少个挡板
                    tmp->dy=-tmp->dy;
                }else{// 没有墙壁
                    if(board[NO].BallNum==1){// 
                        board[NO].HeadNode->next->dx=0;
                        board[NO].HeadNode->next->dy=0;
                        board[NO].HeadNode->next->element=board[NO].element;
                        board[NO].HeadNode->next->x=board[NO].x+board[NO].w/2;
                        board[NO].HeadNode->next->y=board[NO].y-Ball_radius*2;
                    }else{
                        DeleteBall(board[NO].HeadNode,tmp,&board[NO]);
                        board[NO].HaveNewBallDeleted=true;
                    }
                    CountPower_Wall++;
                }
                board[NO].CountPower_NewBall=0; 
            }         
}

void InitGameObject(){
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
        CountPower_Wall=0;
        board[i].y=Board_start_y;
        board[i].x=Block*4*i+Board_start_x;
        board[i].HaveNewBallCreated=false;
        board[i].HaveNewBallDeleted=false;
    }
}

void InitBall(){
    for(int i=0;i<PlayNum;i++){
        Ball *NewBall=CreatBall(board[i].HeadNode,board[i].x+board[i].w/2,board[i].y-Ball_radius*2,&board[i]);
    }
}



void InitMap_5(){
    for(int i=0;i<=Row+1;i++){
        for(int j=0;j<=Col+1;j++){
            if(i==0||i==Row+1||j==0||j==Col+1){
                map[i][j].status=0;
            }else if(i==1||i==j||i+j==10||i+j==19||i==j-9){
                map[i][j].HP=MaxHp;
                map[i][j].status=1;
                BrickLeft++;
                if(i==1) map[i][j].element=thunder;
                else if(i==j) map[i][j].element=fire;
                else if(i+j==10)  map[i][j].element=water;
                else if(i+j==19)  map[i][j].element=thunder;
                else if(i==j-9)  map[i][j].element=ice;
            }else{
                map[i][j].status=0;
            }
        }
    }
}

void LimitBoard(Board *board){
    if(board->x<=0){
        board->x=0;
        board->dx=0;
    }
    if(board->x+board->w>=Window_Width){
        board->x=Window_Width-board->w;
        board->dx=0;
    }
    if(board->y<=Window_Depth-Block*3){
        board->y=Window_Depth-Block*3;
        board->dy=0;
    }
    if(board->y+board->h>=Window_Depth-Block*0.5){
        board->y=Window_Depth-board->h-Block*0.5;
        board->dy=0;
    }
}

void Server_Quit(){
    close(socket_listen);
    for(int i=0;i<PlayNum;i++){
        close(Player_socket[i]);
    }
}

void BallNumChange(Message *MSG){
    for(int i=0;i<PlayNum;i++){
        if(board[i].HaveNewBallCreated){
            MSG->ballMessage[i][0].HaveNewBallCreated=true;
            MSG->musicMessage.Music_CreatNewBall=true;
        }else{
            MSG->ballMessage[i][0].HaveNewBallCreated=false;
            MSG->musicMessage.Music_CreatNewBall=false;
        }
        if(board[i].HaveNewBallDeleted){
            MSG->ballMessage[i][0].HaveNewBallDeleted=true;
        }else{
            MSG->ballMessage[i][0].HaveNewBallDeleted=false;
        }
        board[i].HaveNewBallCreated=false;
        board[i].HaveNewBallDeleted=false;
    }    
}

bool NextLevel(time_t *BeginTime){
    if(level<5&&BrickLeft<=0){
        level++;
        *BeginTime=time(NULL);
        return 1;
    }else if(level==5&&BrickLeft<=0){
        level++;
        return 0;
    }
    else{
        return 0;
    }
}

bool IsVictory(){
    if(level>=6){
        return 1;
    }else{
        return 0;
    }
}

bool IsLose(time_t *BeginTime){
    if(difftime(time(NULL),*BeginTime)>=240/PlayNum){
        return true ;
    }else {
        return false;
    }
}

void Server_Win(){
    Message MSG;
    memset(&MSG,0,sizeof(Message));
    MSG.IsWin=true;
    for(int i=0;i<PlayNum;i++){
        send(Player_socket[i],&MSG,sizeof(Message),0);
    }
    Server_Quit();
}

void Server_Lose(){
    Message MSG;
    memset(&MSG,0,sizeof(Message));
    MSG.IsLose=true;
    for(int i=0;i<PlayNum;i++){
        send(Player_socket[i],&MSG,sizeof(Message),0);
    }
    Server_Quit();
}
void InitMap_3(){
    for(int i=0;i<=Row+1;i++){
        for(int j=0;j<=Col+1;j++){
           if((j==2&&(i>=3&&i<=7))||(j==3&&i==5)||(j==4&&(i==4||i==6))||(j==5&&(i==3||i==7))){
                map[i][j].HP=MaxHp;
                map[i][j].element=fire;
                map[i][j].status=1;
                BrickLeft++;
           }else if((j==7&&(i>=3&&i<=7))||(i==7&&(j==8||j==9))){
                map[i][j].HP=MaxHp;
                map[i][j].element=water;
                map[i][j].status=1;
                BrickLeft++;
           }else if((j==11&&(i>=3&&i<=7))||((j==12||j==13)&&(i==3||i==5||i==7))){
                map[i][j].HP=MaxHp;
                map[i][j].element=thunder;
                map[i][j].status=1;
                BrickLeft++;
           }else if((j==15&&(i>=3&&i<=7))||((j==16||j==17)&&(i==3||i==5||i==7))){
                map[i][j].HP=MaxHp;
                map[i][j].element=ice;
                map[i][j].status=1;
                BrickLeft++;
           }else{
                map[i][j].status=0;
           }
        }
    }
}

void ReInitGameobject(int level){
    for(int i=0;i<PlayNum;i++){
        while(board[i].HeadNode->next!=NULL){
            DeleteBall(board[i].HeadNode,board[i].HeadNode->next,&board[i]);
        }
        free(board[i].HeadNode);
    }
    BrickLeft=0;
    InitMap(level);
    InitBoard();
    InitBall();
}

void InitMap_4(){
     for(int i=0;i<=Row+1;i++){
        for(int j=0;j<=Col+1;j++){
            if((i==1&&(j!=0&&j!=Col+1))||((j>i&&j<Col+1-i)&&i!=Row+1&&i!=0)){
                map[i][j].HP=MaxHp;
                map[i][j].status=1;
                BrickLeft++;
                if(i>=1&&i<=2) map[i][j].element=ice;
                else if(i>=3&&i<=4)  map[i][j].element=fire;
                else if(i>=5&&i<=6)   map[i][j].element=thunder;
                else if(i>=7&&i<=8)   map[i][j].element=water;
            }else {
                map[i][j].status=0;
            }
        }
    }
}

void InitMap_1(){
    for(int i=0;i<=Row+1;i++){
        for(int j=0;j<=Col+1;j++){
            if(i==0||j==0||i==Row+1||j==Col+1){
                map[i][j].status=0;
            }else if((i==2||i==3)&&(j>=2&&j<=6)){
                map[i][j].element=fire;
                map[i][j].HP=MaxHp;
                map[i][j].status=1;
                BrickLeft++;
            }else if((i==4||i==5)&&(j>=8&&j<=12)){
                map[i][j].element=water;
                map[i][j].HP=MaxHp;
                map[i][j].status=1;
                BrickLeft++;
            }else if((i==2||i==3)&&(j>=13&&j<=17)){
                map[i][j].element=thunder;
                map[i][j].HP=MaxHp;
                map[i][j].status=1;
                BrickLeft++;
            }else if((i==6||i==7)&&(j>=2&&j<=6)){
                map[i][j].element=ice;
                map[i][j].HP=MaxHp;
                map[i][j].status=1;
                BrickLeft++;
            }else if((i==6||i==7)&&(j>=13&&j<=17)){
                map[i][j].element=normal;
                map[i][j].HP=MaxHp;
                map[i][j].status=1;
                BrickLeft++;
            }
        }
    }
}

void Server_NextLevel(){
    Message MSG;
    memset(&MSG,0,sizeof(Message));
    ReInitGameobject(level);
    MSG.IsNextLevel=true;
    printf("enter wrong\n");
    for(int i=0;i<PlayNum;i++){
        send(Player_socket[i],&MSG,sizeof(Message),0);
    }
}