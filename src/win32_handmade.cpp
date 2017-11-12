#include "handmade.h"

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <xinput.h>
#include <dsound.h>

#include "win32_handmade.h"


//global
global_variable bool32 GlobalRunning;
global_variable bool32 GlobalPause;
global_variable win32_off_screen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 GlobalPerCountFrequency;

//-------------XinputGetState------------
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

//-------------XinputSetState------------
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

//direct sound
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name( LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
	if (Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}

}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
	debug_read_file_result Result = {};
	/*
	  Creates or opens a file or I/O device. The most commonly used I/O devices are as
	  follows: file, file stream, directory, physical disk, volume, console buffer, tape drive,
	  communications resource, mailslot, and pipe.
	*/
	HANDLE FileHandle = CreateFileA(
		Filename,
		GENERIC_READ,
		FILE_SHARE_READ,
		0,
		OPEN_EXISTING,
		0,
		0);
	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;
		if (GetFileSizeEx(FileHandle, &FileSize))
		{
			uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
			Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			if (Result.Contents)
			{
				DWORD BytesRead;
				if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) && (FileSize32 == BytesRead))
				{
					Result.ContentsSize = FileSize32;
				}
				else
				{
					DEBUGPlatformFreeFileMemory(Result.Contents);
					Result.Contents = 0;
				}
			}
			else
			{

			}
		}
		else
		{

		}
		CloseHandle(FileHandle);
	}
	else
	{

	}
	return (Result);
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
	bool32 Result = false;
	/*
	  Creates or opens a file or I/O device. The most commonly used I/O devices are as
	  follows: file, file stream, directory, physical disk, volume, console buffer, tape drive,
	  communications resource, mailslot, and pipe.
	*/
	HANDLE FileHandle = CreateFileA(
		Filename,
		GENERIC_WRITE,
		0,
		0,
		CREATE_ALWAYS,
		0,
		0);
		if (FileHandle != INVALID_HANDLE_VALUE) {
			DWORD BytesWritten;
					
			if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0)) 
			{
				// NOTE File read successfully
				Result = (BytesWritten == MemorySize);
			}
			else 
			{
				// TODO logging
			}
	
			CloseHandle(FileHandle);
		}
		else 
		{
			// TODO logging
		}
	
		return(Result);
}

struct win32_game_code
{
	HMODULE DLL;
	game_update_and_render *UpdateAndRender;
	game_get_sound_samples *GetSoundSamples;

	bool32 IsValid;
};

internal win32_game_code win32LoadGameCode(void)
{
	win32_game_code Result = {};

	CopyFile("handmade.exe", "handmade_temp.dll", FALSE);

	Result.DLL = LoadLibraryA("handmade_temp.dll");

	if (Result.DLL)
	{
		Result.UpdateAndRender = (game_update_and_render *)GetProcAddress(Result.DLL, "GameUpdateAndrender");
		Result.GetSoundSamples = (game_get_sound_samples *)GetProcAddress(Result.DLL, "GameGetSoundSamples");

		Result.IsValid = (Result.UpdateAndRender && Result.GetSoundSamples);
	}

	if (!Result.IsValid)
	{
		Result.UpdateAndRender = GameUpdateAndRenderStub;
		Result.GetSoundSamples = GameGetSoundSamplesStub;
	}

	return(Result);
}

internal void win32UnloadGameCode(win32_game_code *GameCode)
{
	if (GameCode->DLL)
	{
		FreeLibrary(GameCode->DLL);
		GameCode->DLL = 0;
	}

	GameCode->IsValid = false;
	GameCode->UpdateAndRender = GameUpdateAndRenderStub;
	GameCode->GetSoundSamples = GameGetSoundSamplesStub;
}

internal void win32LoadXinput(void)
{
	/*
	LoadLibrary
	Loads the specified module into the address space of the calling process.
	*/
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (!XInputLibrary)
	{
		XInputLibrary = LoadLibraryA("xinput1_3.dll");
	}
	if (XInputLibrary)
	{
		/*
		GetProcAddress
		Retrieves the address of an exported function or variable from the specified 
		dynamic-link library (DLL).
		*/
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		if (!XInputGetState)
		{
			XInputGetState = XInputGetStateStub;
		}

		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
		if (!XInputSetState)
		{
			XInputSetState = XInputSetStateStub;
		}
	}
}

/*
DirectSoundCreate
This function creates and initializes an IDirectSound interface.
*/
internal void Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
	if (DSoundLibrary)
	{
		direct_sound_create *DirectSoundCreate = (direct_sound_create *)
			GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		LPDIRECTSOUND DirectSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			/*
			WAVEFORMATEX Structure
			The WAVEFORMATEX structure defines the format
			of waveform-audio data.
			*/
			WAVEFORMATEX WaveFormat;
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;

			if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{

				/*
				The DSBUFFERDESC structure describes the characteristics of a 
				new buffer object. It is used by the 
				IDirectSound8::CreateSoundBuffer method 
				*/
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				/*
				IDirectSound8::CreateSoundBuffer Method
				The CreateSoundBuffer method creates a sound buffer object to 
				manage audio samples.
				*/
				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
				{
					HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
					
					if (SUCCEEDED(Error))
					{
						//set format
					}
					else
					{
						//diog
					}
				}
				else
				{
					//diog
				}
			}
			else
			{
				//diog
			}


			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;
			/*
			IDirectSound8::CreateSoundBuffer Method
			The CreateSoundBuffer method creates a sound buffer object to
			manage audio samples.
			*/
			HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);
			//LPDIRECTSOUNDBUFFER SecondaryBuffer;
			if (SUCCEEDED(Error))
			{

			}
		}
		else
		{
			//diog
		}
	}
	else
	{
		//diog.
	}
}

internal win32_window_dimension getWindowDimension(HWND Window)
{
	win32_window_dimension Result;

	RECT ClientRect;
	// Retrieves the coordinates of a window's client area
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return (Result);
}


//Device - Independent Bitmaps
internal void 
Win32ResizeDIBSection(win32_off_screen_buffer *Buffer, int Width, int Height)
{
	//release
	if (Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;

	int BytesPerPixel = 4;

	Buffer->BytesPerPixel = BytesPerPixel;

	/*
	The BITMAPINFOHEADER structure contains information about 
	the dimensions and color format of a DIB.
	*/
	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;
	
	int BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	Buffer->Pitch = Width * Buffer->BytesPerPixel;

	//RenderWeirdGradient(128, 0);
}

internal void 
Win32UpdateBufferInWindow(win32_off_screen_buffer *Buffer, HDC DeviceContext, 
	int WindowWidth, int WindowHeight)
{

	/*
	The StretchDIBits function copies the color data for a rectangle of pixels in
	a DIB, JPEG, or PNG image to the specified destination rectangle. 
	*/
	StretchDIBits(
		DeviceContext,
		0, 0, WindowWidth, WindowHeight,
		0, 0, Buffer->Width, Buffer->Height,
		Buffer->Memory,
		&Buffer->Info,
		DIB_RGB_COLORS,
		SRCCOPY);
}

//WindowProc callback function
LRESULT CALLBACK Win32WindowCallback( 
	HWND   Window, // handle to the window
	UINT   Message,
	WPARAM wParam,
	LPARAM lParam) 
{
	LRESULT Result = 0;

	switch (Message)
	{
		//Window Notifications
		case WM_SIZE:
		{
			//OutputDebugStringA display to debug window
		}break;

		case WM_DESTROY:
		{
			GlobalRunning = false;
			
		}break;

		case WM_CLOSE:
		{
			GlobalRunning = false;
			PostQuitMessage(0);
			
		}break;

		case WM_ACTIVATEAPP:
		{
			
		}break;

		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			Assert(!"Keyboard input came in through a non-dispatch message!");
		}break;
			

		case WM_PAINT:
		{
			/*
			The BeginPaint function prepares the specified window for 
			painting and fills a PAINTSTRUCT structure with information
			about the painting.
			*/
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			int x = Paint.rcPaint.left;
			int y = Paint.rcPaint.top;
			int Width = Paint.rcPaint.right - Paint.rcPaint.left;
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
			
			win32_window_dimension Dimension = getWindowDimension(Window);
			Win32UpdateBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
			EndPaint(Window, &Paint);

		}break;

		default:
		{
			Result = DefWindowProc(Window, Message, wParam, lParam);
			
		}break;
	}
	return Result;
}

internal void
win32ClearBuffer(win32_sound_output *SoundOutput) {
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(
        0, SoundOutput->SecondaryBufferSize,
        &Region1, &Region1Size,
        &Region2, &Region2Size,
        0
    ))) {
        // TODO Assert that Region1Size/Region2Sizse is valid
        uint8 *DestSample = (uint8 *)Region1;
        for(DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex) {
            *DestSample++ = 0;
        }

        DestSample = (uint8 *)Region2;
        for(DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex) {
            *DestSample++ = 0;
        }

        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal void 
Win32FillSoundBuffer(win32_sound_output *SoundOutput, 
DWORD ByteToLock, DWORD BytesToWrite, game_SoundOutputBuffer *SourceBuffer)
{
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;

	/*
	IDirectSoundBuffer8::Lock Method
	The Lock method readies all or part of the buffer for a data
	write and returns pointers to which data can be written.
	*/
	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
		&Region1, &Region1Size,
		&Region2, &Region2Size, 0)))
	{
		
		DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
		int16 *DestSample = (int16 *)Region1;
		int16 *SourceSample = SourceBuffer->Samples;
		for (DWORD SampleIndex = 0;
			SampleIndex < Region1SampleCount;
			++SampleIndex)
		{
			//TODO: Draw			
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}

		DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
		DestSample = (int16 *)Region2;
		for (DWORD SampleIndex = 0;
			SampleIndex < Region2SampleCount;
			++SampleIndex)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal void
Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state *OldState, game_button_state *NewState,
	DWORD ButtonBit)
{
	NewState->EndedDown = (XInputButtonState & ButtonBit) == ButtonBit;
	NewState->HalfTrasitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}


internal void
Win32ProcessKeyboardMessage(game_button_state *NewState, bool32 IsDown)
{
	NewState->EndedDown = IsDown;
	++NewState->HalfTrasitionCount;
}

internal void
win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state *OldState,
	DWORD ButtonBit,
	game_button_state *NewState)
{
	NewState->EndedDown = (XInputButtonState & ButtonBit) == ButtonBit;
	NewState->HalfTrasitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}


internal real32 
win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold)
{
	real32 Result = 0;

	if (Value < DeadZoneThreshold)
	{
		Result = (real32)Value / 32768.0f;
	}
	else if (Value > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
	{
		Result = (real32)Value / 32767.0f;
	}

	return (Result);

}

internal void
Win32ProcessPendingMessages(game_controller_input *KeyboardController)
{
	MSG Message;

	while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
	{
		switch (Message.message)
		{
			case WM_QUIT:
			{
				GlobalRunning = false;
			}break;

			case WM_SYSKEYUP:
			case WM_SYSKEYDOWN:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				//VK = Virtual Key Codes
				uint32 VKCode = (uint32)Message.wParam;
				bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
				bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);

				if (WasDown != IsDown)
				{
					if (VKCode == 'W')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);

					}
					else if (VKCode == 'A')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
					}
					else if (VKCode == 'S')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
					}
					else if (VKCode == 'D')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
					}
					else if (VKCode == 'Q')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
					}
					else if (VKCode == 'E')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
					}
					else if (VKCode == VK_UP)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
					}
					else if (VKCode == VK_LEFT)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
					}
					else if (VKCode == VK_DOWN)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);

					}
					else if (VKCode == VK_RIGHT)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
					}
					else if (VKCode == VK_ESCAPE)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
					}
					else if (VKCode == VK_SPACE)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
					}
#if HANDMADE_INTERNAL
					else if(VKCode == 'P') 
					{
						if(IsDown) 
						{
						    GlobalPause = !GlobalPause;
                        }
					}
#endif
				}
				bool32 AltKeyWasDown = (Message.lParam & (1 << 29));
				if ((VKCode == VK_F4) && AltKeyWasDown)
				{
					GlobalRunning = false;
				}
			}break;

			default:
			{

				TranslateMessage(&Message);
				DispatchMessage(&Message);
			}break;
		}
	}
}

inline LARGE_INTEGER
Win32GetWallCloack(void)
{
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return(Result);
}

inline real32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	real32 Result = ((real32)(End.QuadPart - Start.QuadPart) / (real32)GlobalPerCountFrequency);
	return (Result);
}

internal void 
win32DebugDrawvertical(win32_off_screen_buffer *BackBuffer, int X, int Top, int Bottom, uint32 Color)
{
	if (Top <= 0)
	{
		Top = 0;
	}

	if (Bottom > BackBuffer->Height)
	{
		Bottom = BackBuffer->Height;
	}

	if ((X >= 0) && (X < BackBuffer->Width))
	{
		uint8 *Pixel = ((uint8 *)BackBuffer->Memory + X*BackBuffer->BytesPerPixel + Top*BackBuffer->Pitch);
		for(int Y = 0; Y < Bottom; ++Y)
		{
			*(uint32 *)Pixel = Color;
			Pixel += BackBuffer->Pitch;
		}
	}
	
}

inline void
win32DrawSoundBufferMarker(win32_off_screen_buffer *BackBuffer, win32_sound_output *SoundOutput,
real32 C, int PadX, int PadY, int Top, int Bottom, DWORD Value, uint32 Color)
{
	//Assert(Value < SoundOutput->SecondaryBufferSize);
	real32 XReal32 = (C * (real32)Value);
	int X = PadX + (int)XReal32;
	win32DebugDrawvertical(BackBuffer, X, Top, Bottom, Color);
	
}

internal void
Win32DebugSyncDisplay(win32_off_screen_buffer *BackBuffer, int MarkerCount, 
	win32_debug_time_marker *Markers, int CurrentMarkerIndex, win32_sound_output *SoundOutput, 
	real32 TargetSecondsPerFrame)
{
	int PadX = 16;
	int PadY = 16;

	int LineHeight = 64;

	real32 C = (real32)(BackBuffer->Width - 2*PadX) / (real32)SoundOutput->SecondaryBufferSize;
	for (int MarkerIndex = 0; MarkerIndex < MarkerCount; ++MarkerIndex)
	{
		win32_debug_time_marker *ThisMarker = &Markers[MarkerIndex];
		Assert(ThisMarker->OutputPlayCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputWriteCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputLocation < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputByteCount < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipPlayCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipWriteCursor < SoundOutput->SecondaryBufferSize);

		DWORD PlayColor = 0xFFFFFFFF;
		DWORD WriteColor = 0xFFFF0000;

		int Top = PadY;
		int Bottom = PadY + LineHeight;
		if (MarkerIndex == CurrentMarkerIndex)
		{
			Top += LineHeight + PadY;
			Bottom += LineHeight + PadY;

			win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, PadY, Top, Bottom, ThisMarker->OutputPlayCursor, PlayColor);
			win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, PadY, Top, Bottom, ThisMarker->OutputWriteCursor, WriteColor);
			
			Top += LineHeight + PadY;
			Bottom += LineHeight + PadY;

			win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, PadY, Top, Bottom, ThisMarker->OutputLocation, PlayColor);
			win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, PadY, Top, Bottom, ThisMarker->OutputByteCount + ThisMarker->OutputByteCount, WriteColor);
		
		}
		
		win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, PadY, Top, Bottom, ThisMarker->FlipPlayCursor, PlayColor);
		win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, PadY, Top, Bottom, ThisMarker->FlipWriteCursor, WriteColor);
	}
}

//entry point
int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow)
{

	//win32_game_code Game = win32LoadGameCode();
	//win32UnloadGameCode(&GameCode);

	LARGE_INTEGER PerfCountFrequancyResult;
	QueryPerformanceFrequency(&PerfCountFrequancyResult);
	GlobalPerCountFrequency = PerfCountFrequancyResult.QuadPart;

	UINT DesiredSchedulerMS = 1;
	bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

	win32LoadXinput();

	//WNDCLASS structure
	WNDCLASS WindowClass = {};

	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32WindowCallback;
	WindowClass.hInstance = hInstance;
	//WindowClass.hIcon;
	WindowClass.lpszClassName = "HandmadeHeroWindowsClass";

#define MonitorRefreshHz 60
#define GameUpdateHz (MonitorRefreshHz / 2)
	real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz; //Seconds Elapsed

	// CreateWindowEx function
	if (RegisterClass(&WindowClass))
	{
		HWND Window =
			CreateWindowEx(
				0,						//Extended Window Styles
				WindowClass.lpszClassName,
				"Handmade Hero",			//Title 
				WS_OVERLAPPEDWINDOW | WS_VISIBLE,
				CW_USEDEFAULT,				//x position
				CW_USEDEFAULT,				//y position
				CW_USEDEFAULT,				//width
				CW_USEDEFAULT,				//height
				0,
				0,
				hInstance,
				0);
		if (Window != NULL)
		{
			HDC DeviceContext = GetDC(Window);

			win32_sound_output SoundOutput = {};
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.BytesPerSample = sizeof(int16) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
			SoundOutput.LatencySampleCount = 3*(SoundOutput.SamplesPerSecond / GameUpdateHz);
			SoundOutput.SafetyBytes = (SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample / GameUpdateHz) / 3;
			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			win32ClearBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			GlobalRunning = true;

#if 0
			while()
#endif

			int16 *Samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize,
				MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

#if HANDMADE_INTERNAL
				LPVOID BaseAddress = (LPVOID)Terabytes((uint64)2);
#else
				LPVOID BaseAddress = 0;
#endif
			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(64);
			GameMemory.TransientStorageSize = Gigabytes((uint64)4);

			uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
			GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, (size_t)TotalSize,
				MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			GameMemory.TransientStorage = ((uint8 *)GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize);

			if (Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
			{
				game_input Input[2] = {};
				game_input *NewInput = &Input[0];
				game_input *OldInput = &Input[1];

				//Represents a 64-bit signed integer value.
				LARGE_INTEGER LastCounter = Win32GetWallCloack();

				int DebugTimeMarkerIndex = 0;
				win32_debug_time_marker DebugTimeMarkers[GameUpdateHz / 2] = {0};

				DWORD AudioLatencyBytes = 0;
				real32 AudioLatencySeconds = 0;
				bool32 SoundIsValid = false;

				win32_game_code Game = win32LoadGameCode();
				uint32 LoadCounter = 0;

				//rdtsc returns the processor time stamp.
				uint64 LastCycleCount = __rdtsc();

				while (GlobalRunning)
				{
					if (LoadCounter++ > 120)
					{
						win32UnloadGameCode(&Game);
						Game = win32LoadGameCode();
						LoadCounter = 0;
					}

					game_controller_input *OldKeyboardController = GetController(OldInput, 0);
					game_controller_input *NewKeyboardController = GetController(NewInput, 0);
					game_controller_input ZeroController = {};

					*NewKeyboardController = ZeroController;
					NewKeyboardController->IsConnected = true;

					for (int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ++ButtonIndex)
					{
						NewKeyboardController->Buttons[ButtonIndex].EndedDown = OldKeyboardController->Buttons[ButtonIndex].EndedDown;
					}
					Win32ProcessPendingMessages(NewKeyboardController);

					DWORD MaxControllerCount = XUSER_MAX_COUNT;
					if (MaxControllerCount > ArrayCount(NewInput->Controllers) - 1)
					{
						MaxControllerCount = ArrayCount(NewInput->Controllers);
					}
					//XInput
					//used to determine if the controller is connected. 
					for (DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex)
					{
						DWORD OurControllerIndex = ControllerIndex + 1;
						game_controller_input *OldController = GetController(OldInput, OurControllerIndex);
						game_controller_input *NewController = GetController(NewInput, OurControllerIndex);

						XINPUT_STATE ControllerState;
						if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
						{
							//Controller connected

							//XINPUT_GAMEPAD structure
							XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

							//device digital buttons
							bool32 Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
							bool32 Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
							bool32 Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
							bool32 Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

							NewController->IsAnalog = true;
							NewController->StickAverageX = win32ProcessXInputStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
							NewController->StickAverageY = win32ProcessXInputStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

							if ((NewController->StickAverageX != 0.0f) || 
							(NewController->StickAverageY != 0.0f)) 
							{
								NewController->IsAnalog = true;
							}

							if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
							{
								NewController->StickAverageY = -1.0f;
								NewController->IsAnalog = false;
							}

							if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
							{
								NewController->StickAverageY = -1.0f;
								NewController->IsAnalog = false;
							}

							if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
							{
								NewController->StickAverageX = -1.0f;
								NewController->IsAnalog = false;
							}

							if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
							{
								NewController->StickAverageX = -1.0f;
								NewController->IsAnalog = false;
							}
							real32 Threshold = 0.5f;
                            Win32ProcessXInputDigitalButton(
                                (NewController->StickAverageX < -Threshold) ? 1 : 0,
                                &OldController->MoveLeft, &NewController->MoveLeft, 1
                            );
                            Win32ProcessXInputDigitalButton(
                                (NewController->StickAverageX > Threshold) ? 1 : 0,
                                &OldController->MoveRight, &NewController->MoveRight, 1
                            );
                            Win32ProcessXInputDigitalButton(
                                (NewController->StickAverageY < -Threshold) ? 1 : 0,
                                &OldController->MoveUp, &NewController->MoveUp, 1
                            );
                            Win32ProcessXInputDigitalButton(
                                (NewController->StickAverageY > Threshold) ? 1 : 0,
                                &OldController->MoveDown, &NewController->MoveDown, 1
                            );

							win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionDown,
								XINPUT_GAMEPAD_A, &NewController->ActionDown);
							win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionRight,
								XINPUT_GAMEPAD_B, &NewController->ActionRight);
							win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionLeft,
								XINPUT_GAMEPAD_X, &NewController->ActionLeft);
							win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionUp,
								XINPUT_GAMEPAD_Y, &NewController->ActionUp);
							win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->LeftShoulder,
								XINPUT_GAMEPAD_LEFT_SHOULDER, &NewController->LeftShoulder);
							win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->RightShoulder,
								XINPUT_GAMEPAD_RIGHT_SHOULDER, &NewController->RightShoulder);

							win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Start,
								XINPUT_GAMEPAD_START, &NewController->Start);
							win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Back,
								XINPUT_GAMEPAD_BACK, &NewController->Back);
							/*
							bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
							bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
							bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
							bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
							bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
							bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
							bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
							bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);
							*/


						}
						else
						{
							//not connected

						}
					}

					gameOffScreenBuffer Buffer = {};
					Buffer.Memory = GlobalBackBuffer.Memory;
					Buffer.Width = GlobalBackBuffer.Width;
					Buffer.Height = GlobalBackBuffer.Height;
					Buffer.Pitch = GlobalBackBuffer.Pitch;
					Game.UpdateAndRender(&GameMemory, NewInput, &Buffer);

					DWORD PlayCursor;
					DWORD WriteCursor;
					if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
					{
						if (!SoundIsValid)
						{
							SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
							SoundIsValid = true;
						}

						DWORD ByteToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize);
						DWORD ExpectedSoundBytesPerFrame = (SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample) / GameUpdateHz;
						
						DWORD ExpectedFrameBoundryByte = PlayCursor + ExpectedSoundBytesPerFrame;
						
						DWORD SafeWriteCursor = WriteCursor;
						if (SafeWriteCursor < PlayCursor)
						{
							SafeWriteCursor += SoundOutput.SecondaryBufferSize;
						}
						Assert(SafeWriteCursor >= PlayCursor);
						SafeWriteCursor += SoundOutput.SafetyBytes;

						bool32 AudioCardIsLowLatency = (SafeWriteCursor >= ExpectedFrameBoundryByte);

						DWORD TargetCursor = 0;
						if (AudioCardIsLowLatency)
						{
							TargetCursor = (ExpectedSoundBytesPerFrame + ExpectedFrameBoundryByte);
						}
						else
						{
							TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.SafetyBytes);
						}
						TargetCursor = (TargetCursor % SoundOutput.SecondaryBufferSize);

						DWORD BytesToWrite = 0;
						if (ByteToLock > TargetCursor)
						{
								BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
								BytesToWrite += TargetCursor;
						}
						else
						{
							BytesToWrite = TargetCursor - ByteToLock;
						}	

						
						game_SoundOutputBuffer SoundBuffer = {};
						SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
						SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
						SoundBuffer.Samples = Samples;

						Game.GetSoundSamples(&GameMemory, &SoundBuffer);
					
#if HANDMADE_INTERNAL

						win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
						Marker->OutputPlayCursor = PlayCursor;
						Marker->OutputWriteCursor = WriteCursor;
						Marker->OutputLocation = ByteToLock;
						Marker->OutputByteCount = BytesToWrite;

						DWORD UnwrappedWriteCursor = WriteCursor;
						if (UnwrappedWriteCursor < PlayCursor)
						{
							UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
						}
						AudioLatencyBytes = UnwrappedWriteCursor - PlayCursor;
						AudioLatencySeconds = (((real32)AudioLatencyBytes / (real32)SoundOutput.BytesPerSample) / (real32)SoundOutput.SamplesPerSecond);
						/*
						char TextBuffer[256];
						_snprintf_s(TextBuffer, "Millisec/frame: %d  %dFPS  %dCycles/Frame\n", msPerFrame, FPS, MCPF);
						OutputDebugStringA(TextBuffer);
						*/
#endif	
						Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);	
				}
				else
				{
					SoundIsValid = false;
				}

					
					//ReleaseDC(Window, DeviceContext);

					LARGE_INTEGER WorkCounter = Win32GetWallCloack();
					real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);
					
					real32 SecondsElapsedForFrame = WorkSecondsElapsed;

					if (SecondsElapsedForFrame < TargetSecondsPerFrame)
					{
							if (SleepIsGranular)
							{
								//Suspends the execution of the current thread until the time-out interval elapses.
								DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
								if (SleepMS > 0)
								{
									Sleep(SleepMS);
								}
							}
							//SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallCloack());
						
						real32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallCloack());

						if (TestSecondsElapsedForFrame < TargetSecondsPerFrame)
						{
							//LOG
						}

						while(SecondsElapsedForFrame < TargetSecondsPerFrame)
						{
							SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallCloack());
						}
					}
					else
					{
						//TODO Logging
					}

					win32_window_dimension Dimension = getWindowDimension(Window);
#if HANDMADE_INTERNAL
					Win32DebugSyncDisplay(&GlobalBackBuffer, ArrayCount(DebugTimeMarkers), DebugTimeMarkers, DebugTimeMarkerIndex - 1, &SoundOutput, TargetSecondsPerFrame);
#endif
					Win32UpdateBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);

#if HANDMADE_INTERNAL
					{
						DWORD PlayCursor;
						DWORD WriteCursor;

						if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
						{
							Assert(DebugTimeMarkerIndex < ArrayCount(DebugTimeMarkers));
							win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
							
							Marker->FlipPlayCursor = PlayCursor;
							Marker->FlipWriteCursor = WriteCursor;
						}
					}
#endif
					game_input *Temp = NewInput;
					NewInput = OldInput;
					OldInput = Temp;

					uint64 EndCycleCount = __rdtsc();
					uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
					LastCycleCount = EndCycleCount;

					real64 FPS = 0.0f;
					real64 MCPF = ((real64)CyclesElapsed / (1000.0f * 1000.0f));//megacycles perframe

					LARGE_INTEGER EndCounter = Win32GetWallCloack();
					LastCounter = EndCounter;
					

					char FPSBuffer[256];
					//wsprintf(FPSBuffer, "Millisec/frame: %d  %dFPS  %dCycles/Frame\n", msPerFrame, FPS, MCPF);
					OutputDebugStringA(FPSBuffer);

#if HANDMADE_INTERNAL
					++DebugTimeMarkerIndex;
					if (DebugTimeMarkerIndex == ArrayCount(DebugTimeMarkers))
					{
						DebugTimeMarkerIndex = 0;
					}
#endif
				}
				
			}
			else
			{
				//TODO
			}
		}
		else
		{
			//TODO: Logging
		}
	}
	else
	{
		//TODO: Logging
	}
	return(0);
}