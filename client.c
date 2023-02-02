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

typedef struct Message{
    int board_x;
    int board_y;
    int board_dx;
    int board_dy;
    enum Element element; 
    bool isLaunch;
    bool isChangeColor;
}Message;

Board board[2];
Bullet BulletPack[2][100];
Brick map[Row+2][Col+2];
SDL_Window *Window=NULL;
SDL_Renderer *Renderer=NULL;
TTF_Font *font;
SDL_Color FontColor={0,0,0,255};
const int Window_Width=Block*15;
const int Window_Depth=Block*18;
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
int  dist(int x1,int y1,int x2,int y2);
void ElementalAttack(Ball *ball,Location location);
void DeleteBullet();
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
void HitBrick(int x,int y,Ball *ball);
void ChangeColor(Ball *ball,Location location);
void ChooseMod();
bool IsGameOver();
void DeleteBall(Ball *HeadNode,Ball *DesertedBall,Board *board);
void HitBoard(Ball *ball,Board *board);
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
    beginTimer();
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
    int recvbytes=recv(client_socket,&NO,MaxExcutable,0);//第三个参数是最大能接受的字节长度
    printf("revebytes=%d",recvbytes);
    if(NO==1){
        board[0].NO=1;
        board[1].NO=2;
    }else if(NO==2){
        board[0].NO=2;
        board[1].NO=1;
    }else{
        printf("decide who is first fail\n");
    }
    for(int i=0;i<PlayNum;i++){
        board[i].x=(board[i].NO-1)*Block*4+Board_start_x;
    }
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
        bool IsLaunch;
        while(1){
            int first=clock();
            int times;
            if(KeyValue[SDL_SCANCODE_LSHIFT]){
                times=2;
            }else{
                times=1;
            }
        if (KeyValue[SDL_SCANCODE_W]||KeyValue[SDL_SCANCODE_UP]){
            AdjustBoardLocation(UP,&board[0],times);
        }else if(KeyValue[SDL_SCANCODE_S]||KeyValue[SDL_SCANCODE_DOWN]){
            AdjustBoardLocation(DOWN,&board[0],times);
        }else{
            board[0].dy=0;
        }
        if(KeyValue[SDL_SCANCODE_A]||KeyValue[SDL_SCANCODE_LEFT]){
            AdjustBoardLocation(LEFT,&board[0],times);
        }else if(KeyValue[SDL_SCANCODE_D]||KeyValue[SDL_SCANCODE_RIGHT]){
            AdjustBoardLocation(RIGHT,&board[0],times);
        }else{
            board[0].dx=0;
        }
        if(KeyValue[SDL_SCANCODE_ESCAPE]){
            Quit();
        }
        if(KeyValue[SDL_SCANCODE_H]){
            Launch(&board[0]);
            IsLaunch=true;
        }
        //LimitBoard(&board[0]);
        while(SDL_PollEvent(&event)){
            if(event.type==SDL_QUIT){
                Quit();
            }else if(event.type==SDL_KEYDOWN){
                if(event.key.keysym.sym==SDLK_c){
                    board[0].element=(board[0].element+1)%5;
                }
            } 
        }
        if(PlayNum==2){
            Message MSG;
            MSG.board_x=board[0].x;
            MSG.board_y=board[0].y;
            MSG.board_dx=board[0].dx;
            MSG.board_dy=board[0].dy;
            MSG.element=board[0].element;
            if(IsLaunch){
                MSG.isLaunch=true;
            }else{
                MSG.isLaunch=false;
            }
            if(send(client_socket,&MSG,sizeof(Message),0)==-1){
                perror("send");
            }
        }
     SDL_Delay(FPS-(clock()-first)/CLOCKS_PER_SEC*1000);
        }
        
    }

//一直重复a的原因：getkeyboardstate函数是以事件为基础的，不删除事件，这个按下了a就会一直在消息队列里


int  MoveBall(void *data){//有三种碰撞检测，撞边界，撞板，撞砖块
while(1){
    int first=clock();
    for(int i=0;i<PlayNum;i++){
        Ball *tmp=board[i].HeadNode->next;
        while(tmp!=NULL){
            if(tmp->x-tmp->radius<=0||tmp->x+tmp->radius>=Block*15){// 左边碰撞或者右边碰撞
                tmp->dx=-tmp->dx;
            }else if(tmp->y-tmp->radius<=0){// 上边碰撞
                tmp->dy=-tmp->dy;
            }else if(tmp->y+tmp->radius>=Block*17.4){// 下边碰撞
                if(GetPower_Wall){// 用playnum来维护有多少个挡板
                    tmp->dy=-tmp->dy;
                }else{// 没有墙壁
                    if(board[i].BallNum==1){// 
                        board->HeadNode->next->dx=0;
                        board->HeadNode->next->dy=0;
                        board->HeadNode->next->element=board[i].element;
                        board->HeadNode->next->x=board[i].x+board[i].w/2;
                        board->HeadNode->next->y=board[i].y-board->HeadNode->next->radius*2;
                    }else{
                        DeleteBall(board[i].HeadNode,tmp,&board[i]);
                    }
                    board[i].CountPower_Wall++;
                }
                board[i].CountPower_NewBall=0;          
            }else{//(和砖块碰撞，有点复杂)
                HitBoard(tmp,board);
                HitBrick(tmp->x,tmp->y,tmp);
            }
            //
            tmp->x+=tmp->dx;
            tmp->y+=tmp->dy;
            //
            if(tmp->dx==0&&tmp->dy==0){
                tmp->x=board[i].x+board[i].w/2;
                tmp->y=board[i].y-tmp->radius*2;
            }
            // 
            tmp=tmp->next;// 有bug，如果delete了，就会有问题
            // 
        }
    }
    GetPower();
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
    newBall->dy=-10;// 还没决定好
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

/*Uint32 AnalyseMSG(Uint32 interval,void *param){
    int recvMSG;
    printf("enter\n");
    if(recv(client_socket,&recvMSG,sizeof(int),MSG_DONTWAIT)>=0){
        printf("recvmsg=%d\n",recvMSG);
        if(recvMSG==UP){
            board[1].dy=-Board_Vy;
            board[1].y+=board[1].dy;
        }else if(recvMSG==DOWN){
            board[1].dy=Board_Vy;
            board[1].y+=board[1].dy;
        }else{
            board[1].dx=0;
        }
        if(recvMSG==LEFT){
            board[1].dx=-Board_Vx;
            board[1].x+=board[1].dx;
        }else if(recvMSG==RIGHT){
            board[1].dx=Board_Vx;
            board[1].x+=board[1].dx;
        }else{
            board[1].dy=0;
        }
        if(recvMSG==BoardColorChange){
            board[1].element=(board[1].element+1)%5;
        }else if(recvMSG==DisConnected){
            printf("The Other Player Disconnected\n");
            SDL_RemoveTimer(Timer_ID[3]);
        }else if(recvMSG==LaunchBall){
            Launch(&board[1]);
        }
            LimitBoard(&board[1]);// 有个问题，send和recv是隶属于不同的流的吗
    }else{
        perror("recv");
    }
}*/

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
    SDL_CreateThread(MoveBall,"MoveBall",(void*)NULL);
    SDL_CreateThread(MoveBoard,"MoveBoard",(void*)NULL);
    if(PlayNum==2){
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

void HitBrick(int x,int y,Ball *ball){
    Location location;
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
        DestroyBrick();
        /*GetPower();*/
        /*这个地方误差值+3的引入很巧妙，肉眼无法观察，但完美解决了碰撞问题*/
    }else{
        DestroyBrick();
        return ;
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

void HitBoard(Ball *ball,Board *board){
    for(int i=0;i<PlayNum;i++){
        if(ball->dy>0&&ball->x>=board[i].x&&ball->x<=board[i].x+board[i].w&&ball->y+ball->radius>=board[i].y-10&&ball->y+board[i].HeadNode->next->radius<=board[i].y+board[i].h*2){
            ball->dx=ball->dx+board[i].dx/3;
            ball->dy=-ball->dy+board[i].dy/6;
            ball->element=board[i].element;
            board[i].CountPower_NewBall++;
            if(board[i].CountPower_NewBall==4&&board[i].BallNum<=3){
                CreatBall(board[i].HeadNode,board[i].x+board[i].w/2,board[i].y-ball->radius*2,board);
                board[i].CountPower_NewBall=0;
            }
            return ;
        }
    }
}

Uint32 VanishPower_Wall(Uint32 interval,void *param){
    GetPower_Wall=false;
    SDL_RemoveTimer(Power_ID[1]);
    return interval;
}

Uint32 VanishPower_Bullet(Uint32 interval,void *param){
    GetPower_Bullet=false;
    DeleteBullet(); 
    SDL_RemoveTimer(Power_ID[2]);
    SDL_RemoveTimer(Power_ID[3]);
    return interval;
}

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

Uint32 CreatAndMoveAndHitCheckBullet(Uint32 interval,void *param){
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
}

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
    DeleteBullet();
    InitGameObject();
}

void DeleteBullet(){
    for(int i=0;i<PlayNum;i++){
        for(int j=0;j<BulletPack[i][0].BulletNum;j++){
            BulletPack[i][j].status=0;
        }
    }
}

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
        board[i].CountPower_Bullet=0;
        board[i].CountPower_Wall=0;
        board[i].y=Board_start_y;
    }
}

void InitBall(){
    for(int i=0;i<PlayNum;i++){
        Ball *NewBall=CreatBall(board[i].HeadNode,Board_start_x+30,Board_start_y-2*Ball_radius,&board[i]);
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
        }
        tmp=tmp->next;
        return ;
    }
}

void LimitBoard(Board *board){
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

// 键盘卡顿主要是由于处理事件速度不快，导致停止按键后挤压了事件
// 后期再优化帧率把
// 碰撞的时候巧妙利用视觉残留，不用过于精准

int AnalyseMSG(void *data){
    while(1){
        Message MSG;
        int first=clock();
        if(recv(client_socket,&MSG,sizeof(Message),MSG_DONTWAIT)>=0){
            board[1].x=MSG.board_x;
            board[1].y=MSG.board_y;
            board[1].dx=MSG.board_dx;
            board[1].dy=MSG.board_dy;
            board[1].element=MSG.element;
            if(MSG.isLaunch){
                Launch(&board[1]);
            }
        }else{
            perror("recv");
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
        if(board->x+board->w+Board_Vx*times>=board[1].x&&board->x+Board_Vx*times<=board[1].x&&((board->y+board->h>board[1].y&&board->y<board[1].y)||board->y>board[1].y&&board->y<board[1].y+board[1].h)){
            board->x=board[1].x-board->w;
        //printf("enter1\n");
        }else{
            board->x+=Board_Vx*times;
        }
    }else if(operation==LEFT){
        if(board->x-Board_Vx*times>=board[1].x&&board->x-Board_Vx*times<=board[1].x+board->w&&((board->y+board->h>board[1].y&&board->y<board[1].y)||board->y>board[1].y&&board->y<board[1].y+board[1].h)){
            board->x=board[1].x+board[1].w;
            printf("enter2\n");
        }else{
            board->x-=Board_Vx*times;
        }
    }else if(operation==DOWN){
        if(board->y+Board_Vy*times<=board[1].y&&board->y+board->h+Board_Vy*times>=board[1].y&&((board->x+board->w>board[1].x&&board->x<board[1].x)||board->x>board[1].x&&board->x<board[1].x+board[1].w)){
            board->y=board[1].y-board->h;
        }else{
            board->y+=Board_Vy*times;
        }
    }else if(operation==UP){
        if(board->y-Board_Vy*times>=board[1].y&&board->y-Board_Vy*times<=board[1].y+board[1].h&&((board->x+board->w>board[1].x&&board->x<board[1].x)||board->x>board[1].x&&board->x<board[1].x+board[1].w)){
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

// 