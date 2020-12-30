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


internal void GameOutputSound(game_sound_output_buffer *SoundBuffer, int hz) {
	local_persist real32 tSine;
	int16 ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond / hz;

	int16 *SampleOut = SoundBuffer->Samples;
	for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex) {
		real32 SineValue = sinf(tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		tSine += 2.0f * Pi32 * 1.0f / (real32)WavePeriod;
	}
}


internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer) {
	local_persist int xOffset = 0;
	local_persist int yOffset = 0;
	local_persist int hz = 256;

	GameOutputSound(SoundBuffer, hz);
	RenderGradient(Buffer, xOffset, yOffset);
}