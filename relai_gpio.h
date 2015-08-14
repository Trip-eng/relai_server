#ifndef RELAI_GPIO_H
#define RELAI_GPIO_H

void initRelai() ;
void setRelaiState(int relai, int state);
int getRelaiState(int relai);
void sendTWI(long data);

#endif
