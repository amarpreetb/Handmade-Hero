#if !defined(WIN32_HANDMADE_H)

struct win32_off_screen_buffer
{
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct win32_window_dimension
{
	int Width;
	int Height;
};

struct win32_sound_output
{
	int SamplesPerSecond;
	uint32 RunningSampleIndex;
	int BytesPerSample;
	//int SecondaryBufferSize;
	DWORD SecondaryBufferSize;
	DWORD SafetyBytes;
	real32 tSine;
	int LatencySampleCount;
};

struct win32_debug_time_marker
{
	DWORD OutputPlayCursor;
	DWORD OutputWriteCursor;

	DWORD OutputLocation;
	DWORD OutputByteCount;
	DWORD FlipPlayCursor;
	DWORD FlipWriteCursor;

};

#define  WIN32_HANDMADE_H
#endif