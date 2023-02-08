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
#define Ball_radius 13
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
#define FPS 30
#define QuitGame 8
#define Shitf 9
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
    bool HaveNewBallCreated;
    bool HaveNewBallDeleted;
}BallMessage;
typedef struct Message{
    Brick map[Row+2][Col+2];
    Board board[2];
    BallMessage ballMessage[2][4];
    bool HaveNewBallCreated;
    bool HaveNewBallDeleted;
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

Board board[2];
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
int Power_ID[3];
int BrickLeft=55;
int CountPower_NewBall=0;
int CountPower_Wall=0;
int CountPower_Bullet=0;
bool GetPower_Wall=false;
bool GetPower_Bullet=false;
void BuildConnection(int argc,char *argv[]);
//void PaintFont(const char *text,int x,int y,int w,int h);
void InitAll();
Ball *CreatBall(Ball *HeadNode,int x,int y,Board *board);
void DrawMap();
void DrawBall();
void DrawBoard();
void DrawWall();
void DrawBullet();
void InitMap_1();
void InitMap_2();
void InitMap_3();
void InitMap_4();
void InitMap_5();
void InitGameObject();
void LimitBoard(Board *board);
void Launch(Board *board);
void Load();
void AnalyseMSG_Board(Message *MSG);
void AnalyseMSG_Ball(Message *MSG);
void AnalyseMSG_Map(Message *MSG);
void AnalyseMSG_BallOperation(Message *MSG);
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
void Pause();
void AdjustBoardLocation(int operation,Board *board,int times);
void PrintFont(const char*text);
Uint32 VanishPower_Wall(Uint32 interval,void *param);
Uint32 VanishPower_Bullet(Uint32 interval,void *param);
Uint32 CreatAndMoveAndHitCheckBullet(Uint32 interval,void *param);
int Update(void *data);
void InitMap(int level);
void Draw(SDL_Surface *surface,int x,int y,int w,int h);
void HitBrick(Board *board,Ball *ball,int NO);
void ChangeColor(Ball *ball,Location location);
void ChooseMod();
bool IsGameOver();
void DeleteBall(Ball *HeadNode,Ball *DesertedBall,Board *board);
void HitBoard(Board *board,Ball *ball,int NO);
/*Uint32 AnalyseMSG(Uint32 interval,void *param);*/
int AnalyseMSG(void *data);
int MoveBoard(void *data);
int MoveBall(void *data);
SDL_Surface *BrickSurface[5];
SDL_Surface *BallSurface[5];
SDL_Texture *BallTexture[5];
SDL_Surface *BoardSurface[5];
SDL_Texture *BoardTexture[5];
SDL_Surface *MainBackgroundSurface;
SDL_Texture *MainBackgroundTexture;
SDL_Rect BackgroundRect;
const Uint8 *KeyValue;
int PlayNum;
void beginTimer();
void closeTimer();
void Quit();
int main(int argc,char *argv[]){
    InitAll();
    KeyValue = SDL_GetKeyboardState(NULL);
    if(PlayNum==2){
        BuildConnection(argc,argv);
    }
    beginTimer(&PlayNum);
    while(!IsGameOver());
    /*closeTimer();
    Quit();*/
    
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
    int NO;
    if(recv(client_socket,&NO,sizeof(int),0)>=0){
        printf("You are Player%d\n",NO);
    }
    Message MSG;
    if(recv(client_socket,&MSG,sizeof(Message),0)>=0){
        printf("recv InitMsg succeed\n");
    }
    AnalyseMSG_BallOperation(&MSG);
    AnalyseMSG_Map(&MSG);
    printf("enter1\n");
    AnalyseMSG_Board(&MSG);
    printf("enter2\n");
    AnalyseMSG_Ball(&MSG);
    printf("enter3\n");
    return;
}
void InitAll(){
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_EVERYTHING);//初始化
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();
    Window=SDL_CreateWindow("Sheep",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,Window_Width,Window_Depth,SDL_WINDOW_SHOWN);
    if(Window==NULL){
        return;
    }
    Renderer=SDL_CreateRenderer(Window,-1,SDL_RENDERER_ACCELERATED);
    //font=TTF_OpenFont(,25);
    Load();
    ChooseMod();
    Initgame();
}

/*void PaintFont(const char *text,int x,int y,int w,int h){
    SDL_Surface Surface=TTF_RenderUTF8_Blended(font,text,FontColor);
    SDL_Texture Texture=SDL_CreateTextureFromSurface(Surface);
    SDL_Rect Rect={x,y,w,h};
    SDL_RenderCopy(Renderer,Texture,NULL,&Rect);
    SDL_DestroyTexture(Texture);
}*/

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

int Update(void *data){
    while(1){
        long int first=clock();
        SDL_RenderClear(Renderer);
        DrawMap();
        DrawBall();
        DrawBoard();                  
        if(GetPower_Wall){
            DrawWall();
        }   
    /*if(GetPower_Bullet){
        DrawBullet();
        printf("wronginto bullet\n");
    }*/
        SDL_RenderPresent(Renderer);
        SDL_Delay(FPS-(clock()-first)/CLOCKS_PER_SEC*1000);
    }
    
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
                Draw(BrickSurface[map[i][j].element],Block*(j-1),Block*(i-1),Block,Block);
            }
        }
    }
}
void DrawBall(){
    for(int i=0;i<PlayNum;i++){
        Ball *tmp=board[i].HeadNode->next;
        while(tmp!=NULL){
           Draw(BallSurface[tmp->element],tmp->x,tmp->y,tmp->radius*2,tmp->radius*2);
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
        if(PlayNum==1){
        while(1){
            int first=clock();
            int times;
            if(KeyValue[SDL_SCANCODE_LSHIFT]){
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
                if(event.key.keysym.sym==SDLK_c){
                    board[0].element=(board[0].element+1)%5;
                }
            } 
        }
        SDL_Delay(FPS-(clock()-first)/CLOCKS_PER_SEC*1000);
        }
        if(PlayNum==2){
            SendMSG(NULL);
        }
     
        }
        
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
        if(CountPower_Wall>=10*PlayNum){// 长时间的
        GetPower_Wall=true;
        Power_ID[1]=SDL_AddTimer(5000,VanishPower_Wall,NULL);
        }
    }
    
    /*if(BrickLeft<=5&&GetPower_Bullet==false){//场上方块数量很少
        GetPower_Bullet=true;
        Power_ID[2]=SDL_AddTimer(60,CreatAndMoveAndHitCheckBullet,NULL);
        Power_ID[3]=SDL_AddTimer(5000,VanishPower_Bullet,NULL);
    }*/
 
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

void beginTimer(){
    SDL_CreateThread(Update,"Update",(void*)NULL);
    if(PlayNum==1){
        SDL_CreateThread(MoveBall,"MoveBall",(void*)NULL);// moveball include hitcheck(so messy)
        SDL_CreateThread(MoveBoard,"MoveBoard",(void*)NULL);
    }
    if(PlayNum==2){
        SDL_CreateThread(SendMSG,"SendMSG",(void*)NULL);
        SDL_CreateThread(AnalyseMSG,"AnalyseMSG",(void*)NULL);
    }
}

void closeTimer(){
    for(int i=0;i<4;i++){
        SDL_RemoveTimer(i);
    }
    
}

void Load(){
    char BrickImageName[30];
    char BallImageName[30];
    char BoardImageName[30];
    for(int i=0;i<5;i++){
        sprintf(BrickImageName,"Image/Brick%d.png",i+1);
        BrickSurface[i]=IMG_Load(BrickImageName);
        sprintf(BallImageName,"Image/Ball%d.png",i+1);
        BallSurface[i]=IMG_Load(BallImageName);
        sprintf(BoardImageName,"Image/Board%d.png",i+1);
        BoardSurface[i]=IMG_Load(BoardImageName);
        memset(BrickImageName,0,sizeof BrickImageName);
        memset(BallImageName,0,sizeof BallImageName);
        memset(BoardImageName,0,sizeof BoardImageName);
    }
    MainBackgroundSurface=IMG_Load("Image/MainBackground.jpeg");
    MainBackgroundTexture=SDL_CreateTextureFromSurface(Renderer,MainBackgroundSurface);
    BackgroundRect.h=Block*18;
    BackgroundRect.w=Block*15;
    BackgroundRect.x=0;
    BackgroundRect.y=0;
}


// 可以使用Timer函数来代替SDL——delay(是必须，用SDL_Addtimer),update,move
// 判断元素反应，用音效来区分

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

void HitBoard(Board *board,Ball *ball,int NO){
        if(ball->dy>0&&ball->x>=board[NO].x&&ball->x<=board[NO].x+board[NO].w&&ball->y+ball->radius>=board[NO].y-10&&ball->y+board[NO].HeadNode->next->radius<=board[NO].y+board[NO].h*2){
            ball->dx=ball->dx+board[NO].dx/3;
            ball->dy=-ball->dy+board[NO].dy/6;
            ball->element=board[NO].element;
            board[NO].CountPower_NewBall++;
            if(board[NO].CountPower_NewBall>=4&&board[NO].BallNum<=3){
                CreatBall(board[NO].HeadNode,board[NO].x+board[NO].w/2,board[NO].y-Ball_radius*2,&board[NO]);
                board[NO].HaveNewBallCreated=true;
                board[NO].CountPower_NewBall=0;
            }
        }
            return ;
    }

Uint32 VanishPower_Wall(Uint32 interval,void *param){
    GetPower_Wall=false;
    SDL_RemoveTimer(Power_ID[1]);
    return interval;
}

/*Uint32 VanishPower_Bullet(Uint32 interval,void *param){
    GetPower_Bullet=false;
    //DeleteBullet(); 
    SDL_RemoveTimer(Power_ID[2]);
    SDL_RemoveTimer(Power_ID[3]);
    return interval;
}*/

void ChooseMod(){
    SDL_RenderClear(Renderer);
    SDL_RenderCopy(Renderer,MainBackgroundTexture,NULL,&BackgroundRect);
    SDL_RenderPresent(Renderer);
    SDL_Event event;
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

/*Uint32 CreatAndMoveAndHitCheckBullet(Uint32 interval,void *param){
    // Creat
    int NewNO[2];
    for(int i=0;i<PlayNum;i++){
        NewNO[i]=BulletPack[i][0].BulletNum+1;
    }
    
    for(int i=0;i<PlayNum;i++){
        BulletPack[i][NewNO[i]].status=1;
        BulletPack[i][NewNO[i]].BulletDistance=20;
        BulletPack[i][NewNO[i]].BulletLength=20;
        BulletPack[i][NewNO[i]].x=board[i].x+5;
        BulletPack[i][NewNO[i]].y=board[i].y;
        BulletPack[i][0].BulletNum++;
    }
    // Move
    for(int i=0;i<PlayNum;i++){
        for(int j=0;j<BulletPack[i][0].BulletNum-1;j++){
            if(BulletPack[i][j].status==1){
                BulletPack[i][j].y+=BulletPack[i][j].dy;
            }
        }
    }
    // Hitcheck
    for(int i=0;i<PlayNum;i++){
        for(int j=0;j<BulletPack[i][NewNO[i]].BulletNum;j++){
            if(BulletPack[i][j].status==1){
                int col=BulletPack[i][j].x/Block+1;
                int row=BulletPack[i][j].y/Block+1;
                if(BulletPack[i][j].y<=0){
                    map[row][col].status=0;
                }else if(map[row][col].status==1){
                    map[row][col].HP-=1;
                }
            }
        }
    }
}*/

bool IsGameOver(){
    if(BrickLeft<=0){
        if(level=5){
            //Win();
            return 1;
        }else{
            level++;
            Initgame();
            return 0;
        }
    }else{
        return 0;
    }
}

void Initgame(){
    InitGameObject();
}

/*void DeleteBullet(){
    for(int i=0;i<PlayNum;i++){
        for(int j=0;j<BulletPack[i][0].BulletNum;j++){
            BulletPack[i][j].status=0;
        }
    }
}*/

void Quit(){
    for(int i=0;i<5;i++){
        SDL_DestroyTexture(BoardTexture[i]);
        SDL_DestroyTexture(BallTexture[i]);
    }
    for(int i=0;i<5;i++){
        SDL_FreeSurface(BoardSurface[i]);
        SDL_FreeSurface(BallSurface[i]);
        SDL_FreeSurface(BrickSurface[i]);
    }
    SDL_DestroyTexture(MainBackgroundTexture);
    SDL_DestroyRenderer(Renderer);
    SDL_DestroyWindow(Window);
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
        }else{
            board[i].x=Block*i*4+Block*4;
        }
    }
}

void InitBall(){
    for(int i=0;i<PlayNum;i++){
        Ball *NewBall=CreatBall(board[i].HeadNode,board[i].x+board[i].w/2,board[i].y-Ball_radius*2,&board[i]);
    }
}

void Pause(){
    SDL_RenderClear(Renderer);
    SDL_RenderCopy(Renderer,BallTexture[0],NULL,&BackgroundRect);
    SDL_RenderPresent(Renderer);
    SDL_Event event;
    while(SDL_WaitEvent(&event)){
        switch(event.type){
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym){
                    case SDLK_TAB:
                    return ;
                    case SDLK_ESCAPE:
                        Quit();
                        break;
                    default:
                        break;
                }
                break;
            case SDL_QUIT:
                Quit();
                break;
            default:
                break;
        }
    }
}

/*void PrintFont(const char*text){
    SDL_Surface Font=
}*/

void InitMap_2(){

};

void DrawWall(){
    Draw(BrickSurface[4],0,Block*17.8,Block*16,Block*0.2);
}

void DrawBullet(){

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

int AnalyseMSG(void *data){
    while(1){
        Message MSG;
        int first=clock();
        if(recv(client_socket,&MSG,sizeof(Message),0)>=0){
            AnalyseMSG_BallOperation(&MSG);
            AnalyseMSG_Board(&MSG);
            AnalyseMSG_Ball(&MSG);
            AnalyseMSG_Map(&MSG);
        }
        SDL_Delay(FPS-(clock()-first)/CLOCKS_PER_SEC*1000);
    }
}

// 小心这个错误，两个程序中的内存位置是不一样的
/* if(PlayNum==2){
            Message MSG;
            MSG.board=&board[0];
            if(IsLaunch){
                MSG.isLaunch=true;
            }else{
                MSG.isLaunch=false;
            }
            if(send(client_socket,&MSG,sizeof(Message),0)==-1){
                perror("send");
            }
        }*/

void AdjustBoardLocation(int operation,Board *board,int times){
    if(operation==RIGHT){
        
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

int SendMSG(void *data){
    MessageFromClient tmp;
    MessageFromClient *MSG=&tmp;
    SDL_Event event;
    while(1){
        int first=clock();
        int HaveMSGSend=false;
        int tmpcolor=MSG->ChangeColor;
        memset(MSG,0,sizeof(MessageFromClient));
        MSG->ChangeColor=tmpcolor;// 每一次都要清空缓冲区
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
            }if(KeyValue[SDL_SCANCODE_Q]){
            MSG->ChangeColor=fire;
            }if(KeyValue[SDL_SCANCODE_E]){
            MSG->ChangeColor=water;
            }if(KeyValue[SDL_SCANCODE_C]){
            MSG->ChangeColor=thunder;
            }if(KeyValue[SDL_SCANCODE_V]){
            MSG->ChangeColor=ice;
            }if(KeyValue[SDL_SCANCODE_R]){
            MSG->ChangeColor=normal;
            }if(KeyValue[SDL_SCANCODE_ESCAPE]){
            MSG->IsQuitGame=true;
            }if(KeyValue[SDL_SCANCODE_LSHIFT]){
            MSG->IsShift=true;
            }if(KeyValue[SDL_SCANCODE_H]){
                printf("i click the H\n");
            MSG->IsLaunchBall=true;
            }
        }
            
        while(SDL_PollEvent(&event)){
                if(event.type==SDL_QUIT){
                    HaveMSGSend=true;
                    MSG->IsQuitGame=QuitGame;
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
        int interval;
        if((interval=FPS-(clock()-first)/CLOCKS_PER_SEC*1000)>0){
            SDL_Delay(interval);
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
    if(board->y+board->h>=Window_Depth){
        board->y=Window_Depth-board->h;
        board->dy=0;
    }
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
            printf("wrong delete\n");
            Ball *tmp=board[i].HeadNode->next->next;
            free(board[i].HeadNode->next);
            board[i].HeadNode->next=tmp;
            board[i].BallNum-=1;
        }
}
}