#include "handmade.h"

internal void
RenderWeirdGradient(gameOffScreenBuffer *Buffer, int XOffset, int YOffset
) {
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

internal void gameUpdateAndRender(gameOffScreenBuffer *Buffer, int BlueOffSet, int GreenOffSet)
{
    RenderWeirdGradient(Buffer, BlueOffSet, GreenOffSet);
}