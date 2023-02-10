#include<stdio.h>
void heel(){
    printf("%d\n",brick);
}
int main(){
    static int brick=5;
    heel();
}