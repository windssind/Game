#include<stdio.h>
#include<netdb.h>
#include<math.h>
#define MaxExcutable 100
#define Width 30
#define Length 60
#define Col 8
#define Row 8
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
    struct Ball *next;
}Ball;
typedef struct Board{
    int dx;
    int dy;
    int x;
    int y;
    int w;
    int h;
}Board;

Brick map[Row][Col];
SDL_Window *Window=NULL;
SDL_Renderer *Renderer=NULL;
TTF_Font *font;
SDL_Color FontColor={0,0,0,255};
const int Window_Width=Length*13;
const int Window_Depth=20*Width;
int client_socket;
void BuildConnection(int argc,char *argv[]);
void PaintFont(const char *text,int x,int y,int w,int h);
void InitAll();
void Load();
void Update();
void InitMap(int level);
void Draw(SDL_Surface *surface,int x,int y,int w,int h);
void RenderDrawCircle(int x,int y,int radius);
int main(int argc,char *argv[]){
    BuildConnection(argc,argv);
    InitAll();
    while(true){
        Update();
        MoveBoard();
        MoveBall();
        IsGameOver();
    }
    void Quit();
    
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
    SDL_Init(SDL_INIT_VIDEO);//初始化
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();
    Window=SDL_CreateWindow("Sheep",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,Window_Width,Window_Depth,SDL_WINDOW_SHOWN);
    if(Window==NULL){
        return;
    }
    Renderer=SDL_CreateRenderer(Window,-1,SDL_RENDERER_ACCELERATED);
    font=TTF_OpenFont(,25);
    Load();
    InitMap();
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

void Update(){
    SDL_RenderClear(Renderer);
    DrawMap();
    DrawBall();
    DrawBoard();
    SDL_RenderPresent(Renderer);
}

void Draw(SDL_Surface *surface,int x,int y,int w,int h){
    SDL_Texture texture=SDL_CreateTextureFromSurface(Renderer,surface);
    SDL_Rect rect={x,y,w,h};
    SDL_RenderCopy(Renderer,texture,NULL,&rect);
    SDL_DestroyTexture(texture);
}
void DrawMap(){
    for(int i=0;i<Row;i++){
        for(int j=0;j<Col;j++){
            Draw(..........);
        }
    }
}
void DrawBall(){
    // Load中记得使用SDL_SetRenderDrawColor来设置render的颜色
    RenderDrawCircle();
}

void DrawBoard(){
    Draw(.......);
}
void RenderDrawCircle(int x,int y,int radius){
    for(float degree=0;degree<M_PI;degree+=0.1){
        int x1=x+cos(degree)*radius;
        int y1=y+sin(degree)*radius;
        int x2=x+cos(degree+0.1)*radius;
        int y2=y+cos(degree+0.1)*radius;
        SDL_RenderDrawLine(Renderer,x1,y1,x2,y2);
    }
}

void MoveBoard(){
    SDL_Event event;
    if(borad.type==1){//就是自己
        if(!SDL_PollEvent(&event)){
            return ;
        }
    }else{
        int rc=recv(client_socket,&event,sizeof(SDL_Event),MSG_DONTWAIT);
        if(rc==){
            return ;
        }
    }
    switch(event.type){
                case SDL_QUIT:
                    Quit();
                    break;
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym){
                        case SDLK_LEFT:
                            board.x-=board.dx;
                            break;
                        case SDLK_RIGHT:
                            board.x+=board.dx;
                            break;
                    case SDLK_UP:
                        board.y-=board.dy;
                        break;
                    case SDLK_DOWN:
                        board.y+=board.dy;
                        break;
                    default:
                        break;
                }   
        }SDL_USEREVENT
}

void MoveBall(){
    if(ball.x+ball.radius>=.......||ball.x-ball.radius<=){
        // 左边碰撞
    }else if(){
        // 右边碰撞
    }else if(){
        // 下边碰撞
    }else if(){
        // 上边碰撞？(和砖块碰撞，有点复杂)
        HitBrick(.../*要传进去跟被撞击的砖块有关的信息*/);
    }
    ball.x+=ball.dx;
    ball.y+=ball.dy;
}

void Hitbrick(){
    ChangeColor();
    ElementalAttack();
    DestroyBrick();
    GetPower();
}
 void GetPower(){
    if(.....全局变量维护){
        // 连击数到达一定的combo就可以触发释放额外的小球(所以小球应该是malloc出来的)
        //利用SDL_Addtimer来定时，到了时间节点时候就closetimer，并且用一个全局变量
    }
    if(......){// 到达一定的combo数两个效果触发一个

    }
    if(.....){//场上方块数量很少

    }
 }
Ball *CreatLinkedList(){
    Ball *HeadNode=(Ball*)malloc(sizeof(Ball));
    HeadNode->next=NULL;
    return HeadNode;
}
Ball *CreatBall(Ball *HeadNode,int x,int y,Board board){//用链表结构
    Ball *newBall=(Ball*)malloc(sizeof(Ball));
    newBall->x=x;
    newBall->y=y;
    newBall->dx=board.dx;
    newBall->dy=......;// 还没决定好
    newBall->radius=.......;
    //次序不重要，直接用头插法就行了
    Ball *tmp=HeadNode->next;
    HeadNode->next=newBall;
    newBall->next=tmp;
}
void DeleteBall(Ball *headNode,Ball *DesertedBall){
    if(ballNum==1){
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