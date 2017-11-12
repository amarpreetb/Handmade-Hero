#include "handmade.h"

//Plateform Independent

 void GameOutputSound(game_state *GameState, game_SoundOutputBuffer *SoundBuffer, int ToneHz)
{
    int16 ToneVolume = 3000;
    int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;
    
    int16 *SampleOut = SoundBuffer->Samples;

    for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
        //TODO: Draw
        real32 SineValue = sinf(GameState->tSine);
        int16 SampleValue = (int16)(SineValue * ToneVolume);   

        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        GameState->tSine += 2.0f*Pi32*1.0f / (real32)WavePeriod;

        if (GameState->tSine > 2.0*Pi32)
        {
            GameState->tSine -= 2.0*Pi32;
        }
    }
}


 void
RenderWeirdGradient(gameOffScreenBuffer *Buffer, int XOffset, int YOffset) 
{
    // TODO lets see which is better
    uint8 *Row = (uint8 *)Buffer->Memory;
    for( int Y = 0 ; Y < Buffer->Height ; ++Y ) {
        uint32 *Pixel = (uint32 *)Row;
        for( int X = 0 ; X < Buffer->Width ; ++X ) {
            uint8 Blue = (uint8)(X - XOffset);
            uint8 Green = (uint8)(Y + YOffset);
            *Pixel++ = ((Green << 8) | Blue);
        }

        Row += Buffer->Pitch;
    }
}

extern "C" GAME_UPDATE_AND_RENDER(gameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Start - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons) - 1));
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state *GameState = (game_state *)Memory->PermanentStorage;

	if (!Memory->IsInitialized)
	{
		char *Filename = __FILE__;

		debug_read_file_result File = Memory->DEBUGPlatformReadEntireFile(Filename); //Read the Enire File
		if (File.Contents)
		{
			Memory->DEBUGPlatformWriteEntireFile("test.out", File.ContentsSize, File.Contents);
            Memory->DEBUGPlatformFreeFileMemory(File.Contents);
		}

        GameState->ToneHz = 256;
        GameState->tSine = 0.0f;

		Memory->IsInitialized = true;
	}

	for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
	{

		game_controller_input *Controller = &Input->Controllers[0];
		if (Controller->IsAnalog)
		{
			GameState->BlueOffSet += (int)(4.0f * Controller->StickAverageX);
			GameState->ToneHz = 256 + (int)(128.0f * Controller->StickAverageY);
		}
		else
		{
            if (Controller->MoveLeft.EndedDown)
            {
                GameState->BlueOffSet -= 1;
            }

            if (Controller->MoveRight.EndedDown)
            {
                GameState->BlueOffSet += 1;
            }

		}

		if (Controller->ActionDown.EndedDown)
		{
			GameState->GreenOffSet += 1;
		}
	}
		
		RenderWeirdGradient(Buffer, GameState->BlueOffSet, GameState->GreenOffSet);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    GameOutputSound(GameState, SoundBuffer, GameState->ToneHz);
}

#if HANDMADE_WIN32

#include <windows.h>

BOOL WINAPI DllMain(
   HINSTANCE hinstDLL,
   DWORD     fdwReason,
   LPVOID    lpvReserved)
{
    return(TRUE);
}

#endif