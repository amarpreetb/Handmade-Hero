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
            uint8 Blue = (uint8)(X - XOffset);
            uint8 Green = (uint8)(Y + YOffset);
            *Pixel++ = ((Green << 8) | Blue);
        }

        Row += Buffer->Pitch;
    }
}



internal void 
gameUpdateAndRender(game_memory *Memory, game_input *Input, gameOffScreenBuffer *Buffer)
{
    Assert((&Input->Controllers[0].Start - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons) - 1));
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state *GameState = (game_state *)Memory->PermanentStorage;

	if (!Memory->IsInitialized)
	{
		char *Filename = __FILE__;

		debug_read_file_result File = DEBUGPlatformReadEntireFile(Filename); //Read the Enire File
		if (File.Contents)
		{
			DEBUGPlatformWriteEntireFile("test.out", File.ContentsSize, File.Contents);
			DEBUGPlatformFreeFileMemory(File.Contents);
		}

		GameState->ToneHz = 256;

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

internal void GameGetSoundSamples(game_memory *Memory, game_SoundOutputBuffer *SoundBuffer)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    GameOutputSound(SoundBuffer, GameState->ToneHz);
}