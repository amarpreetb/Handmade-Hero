#if !define(HANDMADE_H)

struct gameOffScreenBuffer
{
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

void gameUpdateAndRender(gameOffScreenBuffer *Buffer, int BlueOffSet, int GreenOffSet);

#define HANDMADE_H
#endif