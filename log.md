SDL_delay you should better add:if(....>0){
    SDL_Delay(),or you will meet unbehavior. it is safe
}

Q:SDL_CreatTextureFromSUrface是否耗时长？能否刚开始就弄好所有的Texture？//会不会性能更好？