struct game_offscreen_buffer {
	void *Memory;
	int Width;
	int Height;
	int Pitch;
};

struct game_sound_output_buffer {
	int SamplesPerSecond;
	int SampleCount;
	int16 *Samples;
};

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, int xOffset, int yOffset, game_sound_output_buffer *SoundBuffer, int hz);
