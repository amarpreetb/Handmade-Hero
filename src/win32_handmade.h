#if !defined(WIN32_HANDMADE_H)

struct win32OffScreenBuffer
{
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel = 4;
};

struct win32WindowDimension
{
	int Width;
	int Height;
};

struct win32SoundOutput
{
	int SamplesPerSecond;
	uint32 RunningSampleIndex;
	int BytesPerSample;
	int SecondaryBufferSize;
	real32 tSine;
	int LatencySampleCount;
};

#define  WIN32_HANDMADE_H
#endif