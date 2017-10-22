#if !defined(HANDMADE_H)

struct gameOffScreenBuffer
{
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

struct game_SoundOutputBuffer
{
    int SamplesPerSecond;
    int SampleCount;
    int16 *Samples;
};

internal void 
gameUpdateAndRender(gameOffScreenBuffer *Buffer, int BlueOffSet, int GreenOffSet,
     game_SoundOutputBuffer *SoundBuffer);

#define HANDMADE_H
#endif