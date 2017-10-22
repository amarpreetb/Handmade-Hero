
#include <stdint.h>
#include<math.h>
//#include <stdio.h>
//#include <XInput.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

//IEEE Standard for Floating-Point Arithmetic (IEEE 754)
typedef  float real32;
typedef double real64;

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

#include "handmade.h"
#include "handmade.cpp"

#include <malloc.h>
#include <windows.h>
#include <stdio.h>
#include <xinput.h>
#include <dsound.h>

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

//global
global_variable bool Running;
global_variable win32OffScreenBuffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

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
			LPDIRECTSOUNDBUFFER SecondaryBuffer;
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

internal win32WindowDimension getWindowDimension(HWND Window)
{
	win32WindowDimension Result;

	RECT ClientRect;
	// Retrieves the coordinates of a window's client area
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return (Result);
}


//Device - Independent Bitmaps
internal void 
Win32ResizeDIBSection(win32OffScreenBuffer *Buffer, int Width, int Height)
{
	//release
	if (Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;
	Buffer->BytesPerPixel = 4;

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
Win32UpdateWindow(win32OffScreenBuffer *Buffer, HDC DeviceContext, 
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
			Running = false;
			
		}break;

		case WM_CLOSE:
		{
			Running = false;
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
			//VK = Virtual Key Codes
			uint32 VKCode = wParam;
			bool WasDown = ((lParam & (1 << 30)) != 0);
			bool IsDown = ((lParam & (1 << 31)) == 0);

			if (WasDown != IsDown)
			{
				if (VKCode == 'W')
				{

				}
				else if (VKCode == 'A')
				{

				}
				else if (VKCode == 'S')
				{

				}
				else if (VKCode == 'D')
				{

				}
				else if (VKCode == 'Q')
				{

				}
				else if (VKCode == 'E')
				{

				}
				else if (VKCode == VK_UP)
				{

				}
				else if (VKCode == VK_LEFT)
				{

				}
				else if (VKCode == VK_DOWN)
				{

				}
				else if (VKCode == VK_RIGHT)
				{

				}
				else if (VKCode == VK_ESCAPE)
				{
					OutputDebugStringA("ESCAPE\n");
					if (IsDown)
					{
						OutputDebugStringA("IsDown\n");
					}
					if (WasDown)
					{
						OutputDebugStringA("WasDown\n");
					}
				}
				else if (VKCode == VK_SPACE)
				{

				}
			}

			bool32 AltKeyWasDown = (lParam & (1 << 29));
			if ((VKCode == VK_F4) && AltKeyWasDown)
			{
				Running = false;
			}
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
			
			win32WindowDimension Dimension = getWindowDimension(Window);
			Win32UpdateWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
			EndPaint(Window, &Paint);

		}break;

		default:
		{
			Result = DefWindowProc(Window, Message, wParam, lParam);
			
		}break;
	}
	return Result;
}

struct win32SoundOutput
{
	int SamplesPerSecond;
	int ToneHz; //Cycles per sec
	int16 ToneVolume;
	uint32 RunningSampleIndex;
	int WavePeriod;
	int BytesPerSample;
	int SecondaryBufferSize;
	real32 tSine;
	int LatencySampleCount;
};

internal void
win32ClearBuffer(win32SoundOutput *SoundOutput) {
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
Win32FillSoundBuffer(win32SoundOutput *SoundOutput, 
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

//entry point
int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow)
{

	LARGE_INTEGER PerfCountFreaquancyResult;
	QueryPerformanceFrequency(&PerfCountFreaquancyResult);
	int64 PerfCountFrequency = PerfCountFreaquancyResult.QuadPart;

	win32LoadXinput();

	//WNDCLASS structure
	WNDCLASS WindowClass = {};

	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

	WindowClass.style = CS_HREDRAW | CS_VREDRAW ;
	WindowClass.lpfnWndProc = Win32WindowCallback;
	WindowClass.hInstance = hInstance;
	//WindowClass.hIcon;
	WindowClass.lpszClassName = "HandmadeHeroWindowsClass";

	// CreateWindowEx function
	if (RegisterClass(&WindowClass))
	{
		HWND Window =
			CreateWindowEx(
				0,						//Extended Window Styles
				WindowClass.lpszClassName,
				"Handmade Hero",			//Title 
				WS_OVERLAPPEDWINDOW|WS_VISIBLE,
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
			Running = true;
			int BlueOffSet = 0;
			int GreenOffSet = 0;
			
			win32SoundOutput SoundOutput = {};

			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.ToneHz = 256; //Cycles per sec
			SoundOutput.ToneVolume = 3000; //sound volume
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/ SoundOutput.ToneHz;
			SoundOutput.BytesPerSample = sizeof(int16) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			win32ClearBuffer(&SoundOutput);
			//Win32FillSoundBuffer(&SoundOutput, 0, SoundOutput.SecondaryBufferSize);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			Running = true;

			//Represents a 64-bit signed integer value.
			LARGE_INTEGER LastCounter;

			/*
			QueryPerformanceCounter function
			Retrieves the current value of the performance counter, which is a
			high resolution (<1us) time stamp that can be used for
			time-interval measurements.
			*/
			QueryPerformanceCounter(&LastCounter);

			//rdtsc returns the processor time stamp.
			int64 LastCycleCount = __rdtsc();

			while (Running)
			{			
				MSG Message;

				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					if (Message.message == WM_QUIT)
					{
						Running = false;
					}
					
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}

				//XInput
				//used to determine if the controller is connected. 
				for (DWORD ControllerIndex = 0; ControllerIndex< XUSER_MAX_COUNT; ++ControllerIndex)
				{
					XINPUT_STATE ControllerState;
					if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
					{
						//Controller connected

						//XINPUT_GAMEPAD structure
						XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

						//device digital buttons
						bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
						bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
						bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
						bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
						bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);


						int16 StickX = Pad->sThumbLX; //Left thumbstick x-axis value
						int16 StickY = Pad->sThumbLY; //Left thumbstick x-axis value
						
						//BlueOffSet =+ StickX >> 12;
						//GreenOffSet =+ StickY >> 12;
					}
					else
					{
						//not connected

					}
				}

				DWORD ByteToLock;
				DWORD BytesToWrite;
				DWORD PlayCursor;
				DWORD WriteCursor;
				DWORD TargetCursor;
				bool32 SoundIsValid = false;

				if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
				{
					ByteToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize);
					
					TargetCursor = ((PlayCursor + (SoundOutput.LatencySampleCount*SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize);
					
					if (ByteToLock > PlayCursor)
					{
						BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
						BytesToWrite += PlayCursor;
					}
					else
					{
						BytesToWrite = PlayCursor - ByteToLock;
					}

					SoundIsValid = true;

				}

				int16 Samples[40000 * 2];

				game_SoundOutputBuffer SoundBuffer = {};
				SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
				SoundBuffer.SampleCount= BytesToWrite / SoundOutput.BytesPerSample;
				SoundBuffer.Samples = Samples;
                
                gameOffScreenBuffer Buffer = {};
                Buffer.Memory = GlobalBackBuffer.Memory;
                Buffer.Width = GlobalBackBuffer.Width;
                Buffer.Height = GlobalBackBuffer.Height;
                Buffer.Pitch = GlobalBackBuffer.Pitch;
                gameUpdateAndRender(&Buffer, BlueOffSet, GreenOffSet, &SoundBuffer, SoundOutput.ToneHz);
				//RenderWeirdGradient(&GlobalBackBuffer, BlueOffSet, GreenOffSet);

		
				if (SoundIsValid)
				{
					Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
				}

				HDC DeviceContext = GetDC(Window);
				win32WindowDimension Dimension = getWindowDimension(Window);
				Win32UpdateWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
				ReleaseDC(Window, DeviceContext);

				++BlueOffSet;
				//++GreenOffSet;

				int64 EndCycleCount = __rdtsc();

				LARGE_INTEGER EndCounter;
				QueryPerformanceCounter(&EndCounter);

				int64 CyclesElapsed = EndCycleCount - LastCycleCount;
				int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
				int32 msPerFrame = (int32)(((1000 * CounterElapsed) / PerfCountFrequency));
				//int64 msPerFrame = ((1000 * CounterElapsed) / PerfCountFrequency);
				int32 FPS = PerfCountFrequency / CounterElapsed;
				int32 MCPF = (int32)(CyclesElapsed / (1000 * 1000));//megacycles perframe
                /*
				char Buffer[256];
				wsprintf(Buffer, "Millisec/frame: %d  %dFPS  %dCycles/Frame\n", msPerFrame, FPS, MCPF);
				OutputDebugStringA(Buffer);
				*/
				LastCycleCount = EndCycleCount;
				LastCounter = EndCounter;

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