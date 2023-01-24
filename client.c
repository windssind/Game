#include<stdio.h>
#include<netdb.h>
#include<math.h>
#include<stdbool.h>
#define MaxExcutable 100
#define Block 40
#define Board_Vx 5
#define Board_Vy 5
#define Board_h 10
#define Board_w 60
#define Board_start_x Block*7
#define Board_start_y Block*14
#define Ball_radius 5
#define Col 15
#define Row 8
#define UP 1
#define LEFT 2
#define DOWN 3
#define RIGHT 4
#define BoardColorChange 5
#define MaxHp 4
#define FPS 50
#undef main
#include"SDL2/SDL.h"
#include"SDL2/SDL_image.h"
#include"SDL2/SDL_ttf.h"
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
    int dx;
    int dy;
    int x;
    int y;
    int w;
    int h;
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

Board board[2];
Bullet BulletPack[2][100];
Brick map[Row][Col];
SDL_Window *Window=NULL;
SDL_Renderer *Renderer=NULL;
TTF_Font *font;
SDL_Color FontColor={0,0,0,255};
const int Window_Width=Block*15;
const int Window_Depth=Block*18;
int client_socket;
int level;
int Timer_ID[5];
int Power_ID[3];
int BallNum;
int BrickLeft;
int CountPower_NewBall=0;
int CountPower_Wall=0;
int CountPower_Bullet=0;
int PlayNum;
bool GetPower_NewBall;
bool GetPower_Wall;
bool GetPower_Bullet;
int BallNum=0;
void BuildConnection(int argc,char *argv[]);
//void PaintFont(const char *text,int x,int y,int w,int h);
void InitAll();
Ball *CreatBall(Ball *HeadNode,int x,int y);
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
void Load();
void ElementalAttack(Ball *ball,Location location);
void DeleteBullet();
void Initgame();
void InitBall();
void InitBoard();
void Pause();
Uint32 VanishPower_Wall(Uint32 interval,void *param);
Uint32 VanishPower_Bullet(Uint32 interval,void *param);
Uint32 CreatAndMoveAndHitCheckBullet(Uint32 interval,void *param);
Uint32 Update(Uint32 interval,void *param);
void InitMap(int level);
void Draw(SDL_Surface *surface,int x,int y,int w,int h);
void HitBrick(int x,int y,Ball *ball);
void ChangeColor(Ball *ball,Location location);
void ChooseMod();
bool IsGameOver();
void DeleteBall(Ball *HeadNode,Ball *DesertedBall);
void HitBoard(Ball *ball,Board *board);
Uint32 AnalyseMSG(Uint32 interval,void *param);
Uint32 MoveBoard(Uint32 interval,void *param);
Uint32 MoveBall(Uint32 interval,void *param);
SDL_Surface *BrickSurface[5];
SDL_Texture *BrickTexture[5];
SDL_Surface *BallSurface[5];
SDL_Texture *BallTexture[5];
SDL_Surface *BoardSurface[5];
SDL_Texture *BoardTexture[5];
SDL_Surface *MainBackgroundSurface;
SDL_Texture *MainBackgroundTexture;
SDL_Rect BackgroundRect;
Ball *HeadNode;
void beginTimer();
void closeTimer();
void Quit();
int main(int argc,char *argv[]){
    ChooseMod();
    if(PlayNum==2){
        BuildConnection(argc,argv);
    }
    InitAll();
    beginTimer();
    while(!IsGameOver());
    closeTimer();
    Quit();
    
}
void BuildConnection(int argc,char *argv[]){
    if(argc<3){ 
        fprintf(stderr,"two few arguments\n");
        exit(1);
    }
    struct addrinfo *result,hints;
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
    char buf[MaxExcutable];
    int recvbytes=recv(client_socket,&buf,MaxExcutable,0);//第三个参数是最大能接受的字节长度
    printf("revebytes=%d",recvbytes);
    /*PaintFont(&buf,Length*6,Width*10,Length*2,Width*2);
    SDL_RenderPresent(Renderer);
    SDL_Delay(1000);*/
    printf("%s",buf);
    return;
}
void InitAll(){
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER);//初始化
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();
    Window=SDL_CreateWindow("Sheep",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,Window_Width,Window_Depth,SDL_WINDOW_SHOWN);
    if(Window==NULL){
        return;
    }
    Renderer=SDL_CreateRenderer(Window,-1,SDL_RENDERER_ACCELERATED);
    //font=TTF_OpenFont(,25);
    Load();
    InitGameObject();
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
}

Uint32 Update(Uint32 interval,void *param){
    SDL_RenderClear(Renderer);
    DrawMap();
    DrawBall();
    DrawBoard();
    if(GetPower_Wall){
        DrawWall();
    }
    if(GetPower_Bullet){
        DrawBullet();
    }
    SDL_RenderPresent(Renderer);
    return interval;
}

void Draw(SDL_Surface *surface,int x,int y,int w,int h){
    SDL_Texture *texture=SDL_CreateTextureFromSurface(Renderer,surface);
    SDL_Rect rect={x,y,w,h};
    SDL_RenderCopy(Renderer,texture,NULL,&rect);
    SDL_DestroyTexture(texture);
}
void DrawMap(){
    for(int i=0;i<Row;i++){
        for(int j=0;j<Col;j++){
            if(map[i][j].status==1){
                Draw(BrickSurface[map[i][j].element],Block*Col,Block*Row,Block,Block);
            }
        }
    }
}
void DrawBall(){
    Ball *tmp=HeadNode->next;
    while(tmp!=NULL){
        Draw(BallSurface[tmp->element],tmp->x,tmp->y,tmp->radius*2,tmp->radius*2);
    }
}

void DrawBoard(){
    for(int i=0;i<2;i++){
        Draw(BoardSurface[4],board[i].x,board[i].y,board[i].w,board[i].h);
    }
}

Uint32 MoveBoard(Uint32 interval,void *param){
    SDL_Event event;
    int MSG;
    while(SDL_PollEvent(&event)){
        const Uint8 *state= SDL_GetKeyboardState(NULL);
        if(event.type == SDL_KEYDOWN || event.type == SDL_KEYUP){
            if ((state[SDL_SCANCODE_UP]||state[SDL_SCANCODE_W])&& (!(state[SDL_SCANCODE_DOWN]||state[SDL_SCANCODE_S]))){
                board[0].y-=board[0].dy;
                MSG=UP;
            }else if((!(state[SDL_SCANCODE_UP]||state[SDL_SCANCODE_W]))&&(state[SDL_SCANCODE_DOWN]||state[SDL_SCANCODE_S])){
                board[0].y+=board[0].dy;
                MSG=DOWN;
            }else if((state[SDL_SCANCODE_LEFT]||state[SDL_SCANCODE_A])&&(!(state[SDL_SCANCODE_RIGHT]||state[SDL_SCANCODE_D]))){
                board[0].x-=board[0].dx;
                MSG=LEFT;
            }else if((!(state[SDL_SCANCODE_LEFT]||state[SDL_SCANCODE_A]))&&(state[SDL_SCANCODE_RIGHT]||state[SDL_SCANCODE_D])){
                board[0].x+=board[0].dx;
                MSG=RIGHT;
            }else{
                switch (event.key.keysym.sym){
                    case SDLK_ESCAPE:
                        Quit();
                        break;
                    case SDLK_q:
                        board[0].element=(board[0].element+1)%5;
                        MSG=BoardColorChange;
                    case SDLK_TAB:
                        Pause();
                    default:
                        break;
                }
                return interval;
            }
            if(send(client_socket,&MSG,sizeof(int),0)==-1){
                fprintf(stderr,"send MSG fail");
                return interval;
            }
        }else if(event.type==SDL_QUIT){
            Quit();
        }
    }
    return interval;
}


Uint32 MoveBall(Uint32 interval,void *param){//有三种碰撞检测，撞边界，撞板，撞砖块
    Ball *tmp=HeadNode->next;
    while(tmp!=NULL){
        if(tmp->x-tmp->radius<=0||tmp->x+tmp->radius>=Block*15){// 左边碰撞或者右边碰撞
            tmp->dx=-tmp->dx;
        }else if(tmp->y-tmp->radius<=0){// 上边碰撞
            tmp->dy=-tmp->dy;
        }else if(tmp->y+tmp->radius>=Block*18){// 下边碰撞
            if(GetPower_Wall){// 用playnum来维护有多少个挡板
                tmp->dy=-tmp->dy;
            }else{// 没有墙壁
                if(BallNum==1){
                    BallNum--;
                    CreatBall(HeadNode,board[rand()%PlayNum].x,board[rand()%PlayNum].y);
                }else{
                    BallNum--;
                    DeleteBall(HeadNode,tmp);
                }
                CountPower_Wall++;
            }          
        }else{//(和砖块碰撞，有点复杂)
            HitBoard(tmp,board);
            HitBrick(tmp->x,tmp->y,tmp);
        }
        tmp->x+=tmp->dx;
        tmp->y+=tmp->dy;
        tmp=tmp->next;// 有bug，如果delete了，就会有问题
    }
    
    return interval;
}
 void GetPower(){
    if(CountPower_NewBall==5){// 一次性的
        // 连击数到达一定的combo就可以触发释放额外的小球(所以小球应该是malloc出来的)
        //利用SDL_Addtimer来定时，到了时间节点时候就closetimer，并且用一个全局变
        GetPower_NewBall=true;
        int No=rand()%PlayNum;
        CreatBall(HeadNode,board[No].x,board[No].y);
        CountPower_NewBall=0;
    }
    if(CountPower_Wall==3){// 长时间的
        GetPower_Wall=true;
        SDL_AddTimer(5000,VanishPower_Wall,NULL);
        CountPower_Wall=0;
    };
    if(BrickLeft<=5&&GetPower_Bullet==false){//场上方块数量很少
        GetPower_Bullet=true;
        Power_ID[2]=SDL_AddTimer(60,CreatAndMoveAndHitCheckBullet,NULL);
        Power_ID[3]=SDL_AddTimer(5000,VanishPower_Bullet,NULL);
    }
 }
Ball *CreatLinkedList(){
    Ball *HeadNode=(Ball*)malloc(sizeof(Ball));
    HeadNode->next=NULL;
    return HeadNode;
}
Ball *CreatBall(Ball *HeadNode,int x,int y){//用链表结构
    Ball *newBall=(Ball*)malloc(sizeof(Ball));
    newBall->x=x;
    newBall->y=y;
    newBall->dx=0;
    newBall->dy=15;// 还没决定好
    newBall->radius=10;
    newBall->element=normal;
    //次序不重要，直接用头插法就行了
    Ball *tmp=HeadNode->next;
    HeadNode->next=newBall;
    newBall->next=tmp;
    BallNum++;
    return newBall;
}
void DeleteBall(Ball *headNode,Ball *DesertedBall){
    if(BallNum==1){
        return ;
    }else{
        Ball *tmp=headNode;
        while(tmp->next!=DesertedBall){
            tmp=tmp->next;
        }
        // 到这一步
        tmp->next=DesertedBall->next;
        free(DesertedBall);
    }
}

Uint32 AnalyseMSG(Uint32 interval,void *param){
    int recvMSG;
    recv(client_socket,&recvMSG,sizeof(int),MSG_DONTWAIT);// 有个问题，send和recv是隶属于不同的流的吗
    switch (recvMSG){
    case UP:
        board[1].y-=board[1].dy;
        break;
    case DOWN:
        board[1].y+=board[1].dy;
        break;
    case LEFT:
        board[1].x-=board[1].dx;
        break;
    case RIGHT:
        board[1].x+=board[1].dx;
        break;
    case BoardColorChange:
        board[1].element=(board[1].element+1)%5;
    default:
        break;
    }
}

void InitMap_1(){
    for(int i=0;i<Row;i++){
        for(int j=i;j<Col-i;j++){
            map[i][j].HP=MaxHp;
            map[i][j].element=rand()%5;
            map[i][j].status=1;
        }
    }
}

void beginTimer(){
    Timer_ID[0]=SDL_AddTimer(FPS,Update,NULL);
    Timer_ID[1]=SDL_AddTimer(FPS,MoveBall,NULL);
    Timer_ID[2]=SDL_AddTimer(FPS,MoveBoard,NULL);
    Timer_ID[3]=SDL_AddTimer(FPS,AnalyseMSG,NULL);
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
    for(int i=0;i<4;i++){
        sprintf(BrickImageName,"/Image/Brick%d",i+1);
        BrickSurface[i]=IMG_Load(BrickImageName);
        sprintf(BallImageName,"/Image/Ball%d",i+1);
        BallSurface[i]=IMG_Load(BallImageName);
        sprintf(BoardImageName,"/Image/Board%d",i+1);
        BoardSurface[i]=IMG_Load(BoardImageName);
        memset(BrickImageName,0,sizeof BrickImageName);
        memset(BallImageName,0,sizeof BallImageName);
        memset(BoardImageName,0,sizeof BoardImageName);
    }
    MainBackgroundSurface=IMG_Load("/Image/MainBackground");
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
        if(ball->x>=Block*(col-1)&&ball->x<=Block*col&&ball->y-ball->radius<=Block*(row-1)&&map[row][col].status==1){//上边碰撞
            location.x=col;
            location.y=row-1;
            ball->dy=-ball->dy;
        }else if(ball->x>=Block*(col-1)&&ball->x<=Block*col&&ball->y+ball->radius>=Block*(row+1)&&map[row][col].status==1){// 下边碰撞
            location.x=col;
            location.y=row+1;
            ball->dy=-ball->dy;
        }else if(ball->y>=Block*(row-1)&&ball->y<=Block*row&&ball->x-ball->radius<=Block*(col-1)&&map[row][col].status==1){
            location.x=col-1;
            location.y=row;
            ball->dx=-ball->dx;
        }else if(ball->y>=Block*(row-1)&&ball->y<=Block*row&&ball->x+ball->radius>=Block*(col+1)&&map[row][col].status==1){
            location.x=col+1;
            location.y=row;
            ball->dx=-ball->dx;
        }else{
            return ;
        }
        ChangeColor(ball,location);
        ElementalAttack(ball,location);
        GetPower();
    }else{
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
    for(int i=0;i<=1;i++){
        if(ball->x>=board[i].x&&ball->x<=board[i].x+board[i].w&&ball->y+ball->radius>=board[i].y&&ball->y+ball->radius<=board[i].y+board[i].h){
            ball->dx=ball->dx+board->dx/2;
            ball->dy=-ball->dy+board->dy/2;
            ball->element=board[i].element;
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
    InitMap(level);
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
        SDL_DestroyTexture(BrickTexture[i]);
    }
    SDL_DestroyTexture(MainBackgroundTexture);
    SDL_DestroyRenderer(Renderer);
    SDL_DestroyWindow(Window);
    SDL_Quit();
}

void InitGameObject(){
    InitMap(level);
    InitBall();
    InitBoard();
}

void InitBoard(){
    for(int i=0;i<PlayNum;i++){
        board[i].dx=Board_Vx;
        board[i].dy=Board_Vy;
        board[i].element=rand()%5;
        board[i].h=Board_h;
        board[i].w=Board_w;
        board[i].x=Board_start_x;
        board[i].y=Board_start_y;
    }
}

void InitBall(){
    HeadNode=(Ball*)malloc(sizeof (Ball));
    for(int i=0;i<PlayNum;i++){
        CreatBall(HeadNode,Board_start_x+Board_w/2,Board_start_y-2*Ball_radius);
    }
}

void Pause(){
    SDL_RenderClear(Renderer);
    SDL_RenderCopy(Renderer,PauseTexture,NULL,&BackgroundRect);
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
            case SDL_Quit:
                Quit();
                break;
            default:
                break;
        }
    }
}