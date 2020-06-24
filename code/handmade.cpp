#include "handmade.h"

internal void RenderGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset) {
	uint8 *Row = (uint8 *)Buffer->Memory;
	for (int Y = 0; Y < Buffer->Height; ++Y) {
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0; X < Buffer->Width; ++X) {
			uint8 blue = X + BlueOffset;
			uint8 green = Y + GreenOffset;
			uint8 red = 0;

			*Pixel++ = ((green << 8) | blue);
		}
		Row += Buffer->Pitch;
	}
}


internal void GameUpdateAndRender(game_offscreen_buffer *Buffer) {
	RenderGradient(Buffer, 0, 0);
}