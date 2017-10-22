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
gameUpdateAndRender(game_input *Input,  gameOffScreenBuffer *Buffer, game_SoundOutputBuffer *SoundBuffer)
{
    local_persist int BlueOffSet = 0;
    local_persist int GreenOffSet = 0;
    local_persist int ToneHz = 256;

    game_controller_input *Input0 = &Input->Controllers[0];
    if (Input0->IsAnalog)
    {
        BlueOffSet += (int)4.0f*(Input0->EndX);
        ToneHz = 256 + (int)(128.0f*(Input0->EndY));
    }
    else
    {

    }

    if (Input0->Down.EndedDown)
    {
        GreenOffSet += 1;
    }

    GameOutputSound(SoundBuffer, ToneHz);
    RenderWeirdGradient(Buffer, BlueOffSet, GreenOffSet);
}