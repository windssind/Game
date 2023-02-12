SDL_delay you should better add:if(....>0){
    SDL_Delay(),or you will meet unbehavior. it is safe
}