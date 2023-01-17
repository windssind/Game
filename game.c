#include<stdio.h>
#include<netdb.h>

int main(){
    BuildConnection();
    InitAll();
    while(true){
        Move(P1);
        Move(P2);
        Hitcheck();
        Win();
        Lose();
    }
    
}