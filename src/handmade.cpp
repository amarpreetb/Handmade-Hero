#include "handmade.h"

//Plateform Independent

internal void GameOutputSound(game_SoundOutputBuffer *SoundBuffer, int ToneHz)
{
    local_persist real32 tSine;
    int16 ToneVolume = 3000;
    int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;
    
    int16 *SampleOut = SoundBuffer->Samples;

    for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
        //TODO: Draw
        real32 SineValue = sinf(tSine);
        int16 SampleValue = (int16)(SineValue * ToneVolume);   

        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tSine += 2.0f*Pi32*1.0f / (real32)WavePeriod;
    }
}


internal void
RenderWeirdGradient(gameOffScreenBuffer *Buffer, int XOffset, int YOffset) 
{
    // TODO lets see which is better
    uint8 *Row = (uint8 *)Buffer->Memory;
    for( int Y = 0 ; Y < Buffer->Height ; ++Y ) {
        uint32 *Pixel = (uint32 *)Row;
        for( int X = 0 ; X < Buffer->Width ; ++X ) {
            uint8 Blue = (X - XOffset);
            uint8 Green = (Y + YOffset);
            *Pixel++ = ((Green << 8) | Blue);
        }

        Row += Buffer->Pitch;
    }
}

internal void 
gameUpdateAndRender(gameOffScreenBuffer *Buffer, 
    int BlueOffSet, int GreenOffSet, game_SoundOutputBuffer *SoundBuffer, int ToneHz)
{
    GameOutputSound(SoundBuffer, ToneHz);
    RenderWeirdGradient(Buffer, BlueOffSet, GreenOffSet);
}