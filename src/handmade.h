#if !defined(HANDMADE_H)

/*

*/

#if HANDMADE_SLOW //Performance
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value) * 1824)
#define Megabytes(Value) (Kilobytes(Value) * 1824)
#define Gigabytes(Value) (Megabytes(Value) * 1824)
#define Terabytes(Value) (Gigabytes(Value) * 1824)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

struct gameOffScreenBuffer
{
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

struct game_memory
{
    bool32 IsInitialized;
    uint64 PermanentStorageSize;
    void *PermanentStorage;

    uint64 TransientStorageSize;
    void *TransientStorage;
};

struct game_SoundOutputBuffer
{
    int SamplesPerSecond;
    int SampleCount;
    int16 *Samples;
};

struct game_button_state
{
    int HalfTrasitionCount;
    bool32 EndedDown;
};

struct game_controller_input
{
    bool32 IsAnalog;

    real32 StartX;
    real32 StartY;

    real32 MinX;
    real32 MinY;

    real32 MaxX;
    real32 MaxY;

    real32 EndX;
    real32 EndY;

    union
    {
        game_button_state Buttons[6];
        struct
        {
            game_button_state Up;
            game_button_state Down;
            game_button_state Left;
            game_button_state Right;
            game_button_state LeftShoulder;
            game_button_state RightShoulder;
        };
    };
};

struct game_input
{

    game_controller_input Controllers[4];
};

internal void 
gameUpdateAndRender(game_memory *Memory, game_input *Input,
      gameOffScreenBuffer *Buffer, game_SoundOutputBuffer *SoundBuffer);

struct game_state 
{
    int ToneHz;
    int GreenOffSet;
    int BlueOffSet;

};

#define HANDMADE_H
#endif