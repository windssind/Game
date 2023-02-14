#include<stdio.h>
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
#define MaxHp 4
#define FPS 25
#define QuitGame 8
#define Shitf 9
#undef main
#include"SDL2/SDL.h"
#include"SDL2/SDL_image.h"
#include"SDL2/SDL_ttf.h"
#include"SDL2/SDL_thread.h"
#include"SDL2/SDL_mixer.h"
#include<time.h>
#include<math.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include<unistd.h>
#include<netdb.h>
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
    bool Play;
}MessageFromClient;

Board board[2];
int Power_ID[3];
/*Bullet BulletPack[2][100];*/
Brick map[Row+2][Col+2];
SDL_Window *Window=NULL;
SDL_Renderer *Renderer=NULL;
TTF_Font *font;
SDL_Color FontColor={0,0,0,255};
const int Window_Width=Block*18;
const int Window_Depth=Block*15;
int client_socket;
int level=1;
int BrickLeft=0;
int CountPower_Wall=0;
bool GetPower_Wall=false;
void BuildConnection(int argc,char *argv[]);
//void PaintFont(const char *text,int x,int y,int w,int h);
void InitAll();
Ball *CreatBall(Ball *HeadNode,int x,int y,Board *board);
void DrawMap();
void DrawBall();
void DrawBoard();
void DrawWall();
void InitMap_1();
void InitMap_2();
void InitMap_3();
void InitMap_4();
void InitMap_5();
void TellStory();
void TellRule();
void PrintFont(const char*text,int x,int y,int w,int h);
void InitGameObject();
void ReInitGameobject(int level);
void LimitBoard(Board *board);
void Launch(Board *board);
void Load();
void AnalyseMSG_Board(Message *MSG);
void AnalyseMSG_Ball(Message *MSG);
void AnalyseMSG_Map(Message *MSG);
void AnalyseMSG_BallOperation(Message *MSG);
void WaitForPlayer();
void Client_Win();
void Client_Lose(time_t *BeginTime);
void HitWall(Board *board,Ball *ball,int NO);
int SendMSG(void *data);
int  dist(int x1,int y1,int x2,int y2);
void ElementalAttack(Ball *ball,Location location);
/*void DeleteBullet();*/
void DestroyBrick();
void Initgame();
void InitBall();
void InitBoard();
void GetPower();
bool IsVictory();
bool NextLevel(time_t *BeginTime);
bool IsLose(time_t *BeginTime);
void AdjustBoardLocation(int operation,Board *board,int times);
void PrintFont(const char*text,int x,int y,int w,int h);
Uint32 VanishPower_Wall(Uint32 interval,void *param);
int Update(void *data,time_t TimeLeft,int level,int Score);
void InitMap(int level);
void Draw(SDL_Surface *surface,int x,int y,int w,int h);
void HitBrick(Board *board,Ball *ball,int NO);
void ChooseMod();
void DeleteBall(Ball *HeadNode,Ball *DesertedBall,Board *board);
void HitBoard(Board *board,Ball *ball,int NO);
/*Uint32 AnalyseMSG(Uint32 interval,void *param);*/
int AnalyseMSG(void *data,time_t *TimeLeft,int *Score);
int MoveBoard(void *data);
int MoveBall(void *data);
SDL_Surface *BrickSurface[5];
SDL_Texture *BrickTexture[5];
SDL_Surface *BallSurface[5];
SDL_Texture *BallTexture[5];
SDL_Surface *BoardSurface[5];
SDL_Surface *PlayBackgroundSurface;
SDL_Texture *PlayBackgroundTexture;
SDL_Surface *WinBackgroundSurface;
SDL_Texture *WinBackgroundTexture;
SDL_Surface *LoseBackgroundSurface;
SDL_Texture *LoseBackgroundTexture;
SDL_Surface *WaitBackgroundSurface;
SDL_Texture *WaitBackgroundTexture;
SDL_Surface *ChooseBackgroundSurface;
SDL_Texture *ChooseBackgroundTexture;
SDL_Surface *TipBackgroundSurface;
SDL_Texture *TipBackgroundTexture;
SDL_Surface *IntroductionBackgroundSurface;
SDL_Texture *IntroductionBackgroundTexture;
SDL_Surface *RecordBackgroundSurface;
SDL_Texture *RecordBackgroundTexture;
Mix_Chunk *music[11];
SDL_Rect BackgroundRect;
const Uint8 *KeyValue;
int PlayNum;
int Score=0;
void Quit();
int main(int argc,char *argv[]){
    srand(time(NULL));
    InitAll();
    KeyValue = SDL_GetKeyboardState(NULL);
    if(PlayNum==2){
        BuildConnection(argc,argv);
        
    }
    time_t BeginTime=time(NULL);
    Mix_PlayChannel(-1,music[2],0);
    while(1){
        if(PlayNum==1){
            printf("Brickleft=%d\n",BrickLeft);
            MoveBall(NULL);
            MoveBoard(NULL);
            Update(NULL,240/PlayNum-(time(NULL)-BeginTime),level,Score);
            if(IsVictory()) Client_Win();
            else if(NextLevel(&BeginTime))  ReInitGameobject(level);
            else if(IsLose(&BeginTime))  Client_Lose(&BeginTime);
        }else if(PlayNum==2){
            time_t TimeLeft;
            int Score;
            SendMSG(NULL);
            for(int i=0;i<10;i++){
                AnalyseMSG(NULL,&TimeLeft,&Score);
            }
            Update(NULL,TimeLeft,level,Score);
        }
        SDL_Delay(FPS);
    }  
}
void BuildConnection(int argc,char *argv[]){
    if(argc<3){ 
        fprintf(stderr,"two few arguments\n");
        exit(1);
    }
    struct addrinfo *result,hints;
    memset(&hints,0,sizeof hints);
    hints.ai_socktype=SOCK_STREAM;
    if(getaddrinfo(argv[1],argv[2],&hints,&result)!=0){
        fprintf(stderr,"getaddrinfo failed\n");
        exit(1);
    }
    client_socket=socket(result->ai_family,result->ai_socktype,result->ai_protocol);
    if(client_socket==-1){
        fprintf(stderr,"creat a socket failed\n");
        exit(1);
    }
    if(connect(client_socket,result->ai_addr,result->ai_addrlen)==-1){
        fprintf(stderr,"connect failed\n");
        exit(1);
    }
    //建立好连接之后，就可以释放了
    freeaddrinfo(result);
    // 已经建立好连接了，现在开始接受信息
    WaitForPlayer();
    int NO;
    if(recv(client_socket,&NO,sizeof(int),0)>=0){
        printf("You are Player%d\n",NO);
    }else{
        printf("recv fail\n");
    }
    return;
}
void InitAll(){
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_EVERYTHING);//初始化
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();
    Mix_Init(MIX_INIT_OGG);
    Window=SDL_CreateWindow("Sheep",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,Window_Width+Block*9,Window_Depth,SDL_WINDOW_SHOWN);
    if(Window==NULL){
        return;
    }
    Renderer=SDL_CreateRenderer(Window,-1,SDL_RENDERER_ACCELERATED);
    //font=TTF_OpenFont(,25);
    Load();
    TellStory();
    TellRule();
    ChooseMod();
    Initgame();
    Mix_PlayChannel(-1,music[10],-1);
}

void InitMap(int level){
    switch (level){
    case 1:
        InitMap_1();
        break;
    case 2:
        InitMap_2();
        break;
    case 3:
        InitMap_3();
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

int Update(void *data,time_t TimeLeft,int level,int Score){
        SDL_RenderClear(Renderer);
        Draw(PlayBackgroundSurface,0,0,Block*18,Block*15);
        Draw(RecordBackgroundSurface,Block*18,0,Block*9,Block*15);
        char str[20];
        PrintFont(SDL_itoa((int)(TimeLeft),str,10),Block*20,Block*8,Block*1.5,Block*1.5);
        PrintFont(SDL_itoa(level,str,10),Block*20,Block*10.8,Block*1.5,Block*1.5);
        PrintFont(SDL_itoa(Score,str,10),Block*20,Block*13.8,Block*1.5,Block*1.5);
        DrawMap();
        DrawBall();
        DrawBoard();            
        if(GetPower_Wall){
            DrawWall();
        }
        SDL_RenderPresent(Renderer);
    }
    

void Draw(SDL_Surface *surface,int x,int y,int w,int h){
    SDL_Texture *texture=SDL_CreateTextureFromSurface(Renderer,surface);
    SDL_Rect rect={x,y,w,h};
    SDL_RenderCopy(Renderer,texture,NULL,&rect);
    SDL_DestroyTexture(texture);
}
void DrawMap(){
    for(int i=0;i<Row+2;i++){
        for(int j=0;j<Col+2;j++){
            if(map[i][j].status==1){
                SDL_Rect BrickRect={Block*(j-1),Block*(i-1),Block,Block};
                SDL_RenderCopy(Renderer,BrickTexture[map[i][j].element],NULL,&BrickRect);
            }
        }
    }
}
void DrawBall(){
    for(int i=0;i<PlayNum;i++){
        Ball *tmp=board[i].HeadNode->next;
        while(tmp!=NULL){
            SDL_Rect BallRect={tmp->x,tmp->y,tmp->radius*2,tmp->radius*2};
            SDL_RenderCopy(Renderer,BallTexture[tmp->element],NULL,&BallRect);
            tmp=tmp->next;
        }
    }
}

void DrawBoard(){
    for(int i=0;i<PlayNum;i++){
        Draw(BrickSurface[board[i].element],board[i].x,board[i].y,board[i].w,board[i].h);
    }
}

int MoveBoard(void *data){
        SDL_Event event;
            int times;
            if(KeyValue[SDL_SCANCODE_SPACE]){
                times=2;
            }else{
                times=1;
            }
        if (KeyValue[SDL_SCANCODE_W]||KeyValue[SDL_SCANCODE_UP]){
            board[0].dy=-Board_Vy*times;
            board[0].y+=board[0].dy;
        }else if(KeyValue[SDL_SCANCODE_S]||KeyValue[SDL_SCANCODE_DOWN]){
            board[0].dy=Board_Vy*times;
            board[0].y+=board[0].dy;
        }else{
            board[0].dy=0;
        }
        if(KeyValue[SDL_SCANCODE_A]||KeyValue[SDL_SCANCODE_LEFT]){
            board[0].dx=-Board_Vx*times;
            board[0].x+=board[0].dx;
        }else if(KeyValue[SDL_SCANCODE_D]||KeyValue[SDL_SCANCODE_RIGHT]){
            board[0].dx=Board_Vx*times;
            board[0].x+=board[0].dx;
        }else{
            board[0].dx=0;
        }
        if(KeyValue[SDL_SCANCODE_ESCAPE]){
            Quit();
        }
        if(KeyValue[SDL_SCANCODE_H]){
            Launch(&board[0]);
        }
        LimitBoard(&board[0]);
        while(SDL_PollEvent(&event)){
            if(event.type==SDL_QUIT){
                Quit();
            }else if(event.type==SDL_KEYDOWN){
                if(event.key.keysym.sym==SDLK_q) board[0].element=0;
                if(event.key.keysym.sym==SDLK_e) board[0].element=1;
                if(event.key.keysym.sym==SDLK_c) board[0].element=2;
                if(event.key.keysym.sym==SDLK_v) board[0].element=3;
                if(event.key.keysym.sym==SDLK_r) board[0].element=4;
            } 
        }
     
        }

//一直重复a的原因：getkeyboardstate函数是以事件为基础的，不删除事件，这个按下了a就会一直在消息队列里


int  MoveBall(void *data){//有三种碰撞检测，撞边界，撞板，撞砖块
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
                tmp->element=board[i].element;
            }
            DestroyBrick();
            tmp=tmp->next;// 有bug，如果delete了，就会有问题
        }
        GetPower();
    }
}
void GetPower(){
        if(CountPower_Wall>=3){// 长时间的
        GetPower_Wall=true;
        Power_ID[1]=SDL_AddTimer(5000,VanishPower_Wall,NULL);
        CountPower_Wall=0;
        }
    }
 
Ball *CreatBall(Ball *HeadNode,int x,int y,Board *board){//用链表结构
    Ball *newBall=(Ball*)malloc(sizeof(Ball));
    newBall->x=x;
    newBall->y=y;
    newBall->dx=0;
    newBall->dy=0;// 还没决定好
    newBall->radius=Ball_radius;
    newBall->element=board->element;
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

void Load(){
    char BrickImageName[30];
    char BallImageName[30];
    char BoardImageName[30];
    for(int i=0;i<5;i++){
        sprintf(BrickImageName,"tmp_image/Brick%d.png",i+1);
        BrickSurface[i]=IMG_Load(BrickImageName);
        BrickTexture[i]=SDL_CreateTextureFromSurface(Renderer,BrickSurface[i]);
        sprintf(BallImageName,"Image/Ball%d.png",i+1);
        BallSurface[i]=IMG_Load(BallImageName);
        BallTexture[i]=SDL_CreateTextureFromSurface(Renderer,BallSurface[i]);
        sprintf(BoardImageName,"Image/Board%d.png",i+1);
        BoardSurface[i]=IMG_Load(BoardImageName);
        memset(BrickImageName,0,sizeof BrickImageName);
        memset(BallImageName,0,sizeof BallImageName);
        memset(BoardImageName,0,sizeof BoardImageName);
    }
    if(Mix_OpenAudio(MIX_DEFAULT_FREQUENCY,MIX_DEFAULT_FORMAT,MIX_DEFAULT_CHANNELS,4096)==-1) printf("openduio wrong\n");
    char MusicName[30];
    for(int i=0;i<11;i++){
        sprintf(MusicName,"tmp_image/Music%d.wav",i+1);
        music[i]=Mix_LoadWAV(MusicName);
        printf("%s\n",Mix_GetError());
        memset(MusicName,0,sizeof MusicName);
    }
    printf("%s\n",Mix_GetError());
    PlayBackgroundSurface=IMG_Load("Image/PlayBackground.png");
    PlayBackgroundTexture=SDL_CreateTextureFromSurface(Renderer,PlayBackgroundSurface);
    WinBackgroundSurface=IMG_Load("Image/WinBackground.png");
    WinBackgroundTexture=SDL_CreateTextureFromSurface(Renderer,WinBackgroundSurface);
    LoseBackgroundSurface=IMG_Load("Image/LoseBackground.png");
    LoseBackgroundTexture=SDL_CreateTextureFromSurface(Renderer,LoseBackgroundSurface);
    WaitBackgroundSurface=IMG_Load("Image/WaitBackground.png");
    WaitBackgroundTexture=SDL_CreateTextureFromSurface(Renderer,WaitBackgroundSurface);
    ChooseBackgroundSurface=IMG_Load("Image/ChooseBackground.png");
    ChooseBackgroundTexture=SDL_CreateTextureFromSurface(Renderer,ChooseBackgroundSurface);
    TipBackgroundSurface=IMG_Load("Image/TipBackground.png");
    TipBackgroundTexture=SDL_CreateTextureFromSurface(Renderer,TipBackgroundSurface);
    IntroductionBackgroundSurface=IMG_Load("Image/IntroductionBackground.png");
    IntroductionBackgroundTexture=SDL_CreateTextureFromSurface(Renderer,IntroductionBackgroundSurface);
    RecordBackgroundSurface=IMG_Load("Image/RecordBackground.png");
    RecordBackgroundTexture=SDL_CreateTextureFromSurface(Renderer,RecordBackgroundSurface);
    BackgroundRect.h=Block*15;
    BackgroundRect.w=Block*18;
    BackgroundRect.x=0;
    BackgroundRect.y=0;
    font=TTF_OpenFont("font.TTF",50);
    
}


// 可以使用Timer函数来代替SDL——delay(是必须，用SDL_Addtimer),update,move
// 判断元素反应，用音效来区分

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
    }
}


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
                        map[i][j].HP-=2;
                    }
                }
            }
        }else{
            Score+=3;
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
    for(int i=0;i<=Row;i++){
        for(int j=0;j<=Col;j++){
            if(map[i][j].status==1&&map[i][j].HP<=0){
                map[i][j].status=0;
                BrickLeft--;
            }
        }
    }
}

void HitBoard(Board *board,Ball *ball,int NO){
        if(ball->dy>0&&ball->x>=board[NO].x&&ball->x<=board[NO].x+board[NO].w&&ball->y+ball->radius>=board[NO].y-10&&ball->y+board[NO].HeadNode->next->radius<=board[NO].y+board[NO].h*2){
            ball->dx=ball->dx+board[NO].dx/3;
            ball->dy=-ball->dy+board[NO].dy/6;
            ball->element=board[NO].element;
            board[NO].CountPower_NewBall++;
            Mix_PlayChannel(-1,music[rand()%3+3],0);
            if(board[NO].CountPower_NewBall>=4&&board[NO].BallNum<=3){
                CreatBall(board[NO].HeadNode,board[NO].x+board[NO].w/2,board[NO].y-Ball_radius*2,&board[NO]);
                board[NO].HaveNewBallCreated=true;
                board[NO].CountPower_NewBall=0;
                Mix_PlayChannel(-1,music[rand()%6+2],0);
            }
        }
            return ;
    }

Uint32 VanishPower_Wall(Uint32 interval,void *param){
    GetPower_Wall=false;
    printf("vanish power\n");
    SDL_RemoveTimer(Power_ID[1]);
    return interval;
}

void ChooseMod(){
    SDL_RenderClear(Renderer);
    SDL_RenderCopy(Renderer,ChooseBackgroundTexture,NULL,&BackgroundRect);
    SDL_RenderPresent(Renderer);
    int channel=Mix_PlayChannel(-1,music[1],0);
    SDL_Event event;
    while(1){
        while(SDL_WaitEvent(&event)){
        switch(event.type){
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym){
                    case SDLK_a:
                        PlayNum=1;
                        return ;
                    case SDLK_d:
                        PlayNum=2;
                        return ;
                    default:
                        break;
                }
                break;
            case SDL_QUIT:
                Quit();
            default:
                break;
        }
    }
    }
    Mix_HaltChannel(channel);
    
}



void Initgame(){
    InitGameObject();
}


void Quit(){
    for(int i=0;i<5;i++){
        SDL_FreeSurface(BoardSurface[i]);
        SDL_DestroyTexture(BallTexture[i]);
        SDL_FreeSurface(BallSurface[i]);
        SDL_FreeSurface(BrickSurface[i]);
        SDL_DestroyTexture(BrickTexture[i]);
    }
    for(int i=0;i<11;i++){
        Mix_FreeChunk(music[i]);
    }
    SDL_FreeSurface(PlayBackgroundSurface);
    SDL_FreeSurface(WaitBackgroundSurface);
    SDL_FreeSurface(WinBackgroundSurface);
    SDL_FreeSurface(LoseBackgroundSurface);
    SDL_FreeSurface(ChooseBackgroundSurface);
    SDL_FreeSurface(RecordBackgroundSurface);
    SDL_FreeSurface(TipBackgroundSurface);
    SDL_FreeSurface(IntroductionBackgroundSurface);
    SDL_DestroyTexture(PlayBackgroundTexture);
    SDL_DestroyTexture(WaitBackgroundTexture);
    SDL_DestroyTexture(WinBackgroundTexture);
    SDL_DestroyTexture(LoseBackgroundTexture);
    SDL_DestroyTexture(ChooseBackgroundTexture);
    SDL_DestroyTexture(TipBackgroundTexture);
    SDL_DestroyTexture(RecordBackgroundTexture);
    SDL_DestroyTexture(IntroductionBackgroundTexture);
    SDL_DestroyRenderer(Renderer);
    SDL_DestroyWindow(Window);
    if(PlayNum==2){
        close(client_socket);
    }
    SDL_Quit();
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
        board[i].HaveNewBallCreated=false;
        if(PlayNum==1){
            board[0].x=Board_start_x+Block*3;
        }
    }
}

void InitBall(){
    for(int i=0;i<PlayNum;i++){
        Ball *NewBall=CreatBall(board[i].HeadNode,board[i].x+board[i].w/2,board[i].y-Ball_radius*2,&board[i]);
    }
}


void PrintFont(const char*text,int x,int y,int w,int h){
    SDL_Surface *FontSurface=TTF_RenderUTF8_Blended(font,text,FontColor);
    SDL_Texture *FontTexture=SDL_CreateTextureFromSurface(Renderer,FontSurface);
    SDL_Rect FontRect={x,y,w,h};
    SDL_RenderCopy(Renderer,FontTexture,NULL,&FontRect);
    SDL_FreeSurface(FontSurface);
    SDL_DestroyTexture(FontTexture);
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

void DrawWall(){
    Draw(BrickSurface[4],0,Block*14.6,Block*18.,Block*0.4);
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


// 键盘卡顿主要是由于处理事件速度不快，导致停止按键后挤压了事件
// 后期再优化帧率把
// 碰撞的时候巧妙利用视觉残留，不用过于精准

int AnalyseMSG(void *data,time_t *TimeLeft,int *Score){
        Message MSG;
        memset(&MSG,0,sizeof(Message));
        if(recv(client_socket,&MSG,sizeof(Message),MSG_DONTWAIT)>=0){
            // is win?
            if(MSG.IsWin==true) Client_Win();
            else if(MSG.IsLose==true) Client_Lose(NULL);
            else if(MSG.IsNextLevel==true) {ReInitGameobject(++level);return 1;}
            GetPower_Wall=(MSG.GetPower_Wall==true?true:false);
            *Score=MSG.Score;
            *TimeLeft=MSG.TimeLeft;
            if(MSG.musicMessage.Music_CreatNewBall==true)  Mix_PlayChannel(-1,music[rand()%2+6],0);
            if(MSG.musicMessage.Music_HitBoard==true)   Mix_PlayChannel(-1,music[rand()%3+3],0);
            AnalyseMSG_BallOperation(&MSG);
            AnalyseMSG_Board(&MSG);
            AnalyseMSG_Ball(&MSG);
            AnalyseMSG_Map(&MSG);
            //printf("board[0].x=%d board[1].x=%d\n",board[0].x,board[1].x);
        }
}


void AdjustBoardLocation(int operation,Board *board,int times){
    if(operation==RIGHT){
        if(board->x>=board[1].x+board[1].w&&board->x-Board_Vx*times<=board[1].x+board->w&&((board->y+board->h>board[1].y&&board->y<=board[1].y)||board->y>=board[1].y&&board->y<board[1].y+board[1].h)){
            board->x=board[1].x+board[1].w;
        }else{
            board->x-=Board_Vx*times;
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
    if(board->x+board->w>=Block*18){
        board->x=Block*18-board->w;
        board->dx=0;
    }
    if(board->y<=Block*14){
        board->y=Block*14;
        board->dy=0;
    }
    if(board->y+board->h>=Block*14.5){
        board->y=Block*14.5-board->h;
        board->dy=0;
    }
}

int SendMSG(void *data){
    static int i=0;
    i++;
    MessageFromClient tmp;
    MessageFromClient *MSG=&tmp;
    SDL_Event event;
        //int first=clock();
        int HaveMSGSend=false;
        memset(MSG,0,sizeof(MessageFromClient));
        if((KeyValue[SDL_SCANCODE_W]||KeyValue[SDL_SCANCODE_S]||KeyValue[SDL_SCANCODE_A]||KeyValue[SDL_SCANCODE_D]||KeyValue[SDL_SCANCODE_Q]||KeyValue[SDL_SCANCODE_E]||KeyValue[SDL_SCANCODE_C]||KeyValue[SDL_SCANCODE_V]||KeyValue[SDL_SCANCODE_R]||KeyValue[SDL_SCANCODE_LSHIFT]||KeyValue[SDL_SCANCODE_ESCAPE]||KeyValue[SDL_SCANCODE_H])){
            HaveMSGSend=true;
            if(KeyValue[SDL_SCANCODE_W]){
            MSG->IsUP=true;
            }if(KeyValue[SDL_SCANCODE_S]){
            MSG->IsDown=true;
            }if(KeyValue[SDL_SCANCODE_A]){
            MSG->IsLeft=true;
            }if(KeyValue[SDL_SCANCODE_D]){
            MSG->IsRight=true;
            }if(KeyValue[SDL_SCANCODE_R]){
            MSG->ChangeColor=normal;
            }else MSG->ChangeColor=10;
            if(KeyValue[SDL_SCANCODE_Q]){
            MSG->ChangeColor=fire;
            }if(KeyValue[SDL_SCANCODE_E]){
            MSG->ChangeColor=water;
            }if(KeyValue[SDL_SCANCODE_C]){
            MSG->ChangeColor=thunder;
            }if(KeyValue[SDL_SCANCODE_V]){
            MSG->ChangeColor=ice;
            }if(KeyValue[SDL_SCANCODE_ESCAPE]){
            MSG->IsQuitGame=true;
            }if(KeyValue[SDL_SCANCODE_SPACE]){
            MSG->IsShift=true;
            }if(KeyValue[SDL_SCANCODE_H]){
            MSG->IsLaunchBall=true;
            }
        }else{
            MSG->ChangeColor=10;
        }
            
        while(SDL_PollEvent(&event)){
                if(event.type==SDL_QUIT){
                    HaveMSGSend=true;
                    MSG->IsQuitGame=QuitGame;
                    Quit();
            }
        }
        if(HaveMSGSend){
            int sendbytes=send(client_socket,MSG,sizeof (MessageFromClient) ,MSG_DONTWAIT);
        if(sendbytes==-1){
            printf("send msg to server failed\n");
        }else if(sendbytes==0){
            printf("disconnected\n");
        }
        }
}   


    

// 自己思考问题时候，“次序感“还是太若了，就比如adjustloacation，这种判断应该先是预测会不会超过，如果会，补齐，而不是超过了在退回

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

void HitWall(Board *board,Ball *ball,int NO){
    Ball *tmp=ball;
            if((tmp->x-tmp->radius<=0&&tmp->dx<0)||(tmp->x+tmp->radius>=Window_Width&&tmp->dx>0)){// 左边碰撞或者右边碰撞
                tmp->dx=-tmp->dx;
            }else if(tmp->y-tmp->radius<=0&&tmp->dy<0){// 上边碰撞
                tmp->dy=-tmp->dy;
            }else if(tmp->y+tmp->radius>=Window_Depth-Block*0.6){// 下边碰撞
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

void AnalyseMSG_Board(Message *MSG){
    for(int i=0;i<PlayNum;i++){
        Ball *tmp=board[i].HeadNode;
        board[i]=MSG->board[i];
        board[i].HeadNode=tmp;
    }
}

void AnalyseMSG_Ball(Message *MSG){
    for(int i=0;i<PlayNum;i++){
        Ball *tmp=board[i].HeadNode->next;
        for(int j=0;j<MSG->board[i].BallNum;j++){
            tmp->dx=MSG->ballMessage[i][j].dx;
            tmp->dy=MSG->ballMessage[i][j].dy;
            tmp->y=MSG->ballMessage[i][j].y;
            tmp->x=MSG->ballMessage[i][j].x;
            tmp->element=MSG->ballMessage[i][j].element;
            tmp=tmp->next;
        }
    }
}

void AnalyseMSG_Map(Message *MSG){
   for(int i=0;i<Row+2;i++){
        for(int j=0;j<Col+2;j++){
            map[i][j]=MSG->map[i][j];
        }
   }
}

void AnalyseMSG_BallOperation(Message *MSG){
    for(int i=0;i<PlayNum;i++){
        if(MSG->ballMessage[i][0].HaveNewBallCreated==true){
            CreatBall(board[i].HeadNode,board[i].x+board[i].w/2,board[i].y-Ball_radius*2,&board[i]);
            printf("creat ball succeed\n");
        }
        /*Ball *tmp=board[i].HeadNode->next;
        while(tmp!=NULL){
            printf("")
        }*/
        if(MSG->ballMessage[i][0].HaveNewBallDeleted==true){
            Ball *tmp=board[i].HeadNode->next->next;
            free(board[i].HeadNode->next);
            board[i].HeadNode->next=tmp;
            board[i].BallNum-=1;
        }
}
}

void WaitForPlayer(){
    SDL_RenderClear(Renderer);
    SDL_RenderCopy(Renderer,WaitBackgroundTexture,NULL,&BackgroundRect);
    SDL_RenderPresent(Renderer);
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

bool IsVictory(){
    if(level>=6){
        return 1;
    }else{
        return 0;
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

void Client_Win(){
        //delete thread
        SDL_RenderClear(Renderer);
        SDL_RenderCopy(Renderer,WinBackgroundTexture,NULL,&BackgroundRect);
        SDL_RenderPresent(Renderer);
        SDL_Event event;
        while(1){
            while(SDL_PollEvent(&event)){
                if(event.type==SDL_KEYDOWN){
                    if(event.key.keysym.sym==SDLK_ESCAPE){
                        Quit();
                    }
                }else if(event.type==SDL_QUIT){
                    Quit();
                }
            }
        }
        }

void Client_Lose(time_t *BeginTime){
    printf("LOse\n");
        SDL_RenderClear(Renderer);
        SDL_RenderCopy(Renderer,LoseBackgroundTexture,NULL,&BackgroundRect);
        SDL_RenderPresent(Renderer);
        printf("Unable to update\n");
        Mix_PlayChannel(-1,music[9],0);
        printf("Unable to music\n");
        SDL_Event event;
        while(1){
            if(SDL_PollEvent(&event)){
                if(event.type==SDL_KEYDOWN){
                    switch(event.key.keysym.sym){
                        case SDLK_ESCAPE:
                            Quit();
                            break;
                        case SDLK_RETURN:
                            return ;
                            break;
                        default:
                            break;
                    }
                }else if(event.type==SDL_QUIT){
                    Quit();
                }
            }
    }
}

bool IsLose(time_t *BeginTime){
    if(difftime(time(NULL),*BeginTime)>=240/PlayNum){
        return true ;
    }else {
        return false;
    }
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

void TellStory(){
    SDL_RenderClear(Renderer);
    SDL_RenderCopy(Renderer,IntroductionBackgroundTexture,NULL,&BackgroundRect);
    SDL_RenderPresent(Renderer);
    SDL_Event event;
    while(1){
        while(SDL_PollEvent(&event)){
            if(event.type==SDL_KEYDOWN){
                if(event.key.keysym.sym==SDLK_ESCAPE){
                    Quit();
                }else if(event.key.keysym.sym==SDLK_RETURN){
                    return ;
                }
            }else if(event.type==SDL_QUIT){
                Quit();
            }
        }
    }
}

void TellRule(){
    SDL_RenderClear(Renderer);
    SDL_RenderCopy(Renderer,TipBackgroundTexture,NULL,&BackgroundRect);
    SDL_RenderPresent(Renderer);
    int channel=Mix_PlayChannel(-1,music[0],0);
    SDL_Event event;
    while(1){
        while(SDL_PollEvent(&event)){
            if(event.type==SDL_KEYDOWN){
                if(event.key.keysym.sym==SDLK_ESCAPE){
                    Quit();
                }else if(event.key.keysym.sym==SDLK_RETURN){
                    return ;
                }
            }else if(event.type==SDL_QUIT){
                Quit();
            }
        }
    }
    Mix_HaltChannel(channel);
}

