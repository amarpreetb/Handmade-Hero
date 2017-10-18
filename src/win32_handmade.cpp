#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>
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

#define internal static
#define local_persist static
#define global_variable static

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
global_variable LPDIRECTSOUNDBUFFER SecondaryBuffer;

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
			HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &SecondaryBuffer, 0);
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

internal void 
RenderWeirdGradient(win32OffScreenBuffer *Buffer, int BlueOffSet, int GreenOffSet)
{

	uint8 *Row = (uint8 *)Buffer->Memory;

	for (int y = 0; y < Buffer->Height; ++y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int x = 0; x < Buffer->Width; ++x)
		{
			uint8 Blue = (x + BlueOffSet);

			uint8 Green = (y + GreenOffSet);

			*Pixel++ = ((Green << 8) | Blue);

			//somethin weird
			//uint8 Red = ((x - (GreenOffSet / 2))*(y + (BlueOffSet / 4)) / 8);
			//*Pixel++ = ((Red << 16) | (Green << 8) | Blue);
		}

		Row += Buffer->Pitch;
	}
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
			OutputDebugStringA("wm_size\n");//OutputDebugStringA display to debug window
		}break;

		case WM_DESTROY:
		{
			Running = false;
			OutputDebugStringA("wm_destroy\n");
		}break;

		case WM_CLOSE:
		{
			Running = false;
			PostQuitMessage(0);
			OutputDebugStringA("wm_close\n");
		}break;

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("wm_activateapp\n");
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
			OutputDebugStringA("default\n");
		}break;
	}
	return Result;
}

//entry point
int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow)
{
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

			int SamplesPerSecond = 48000;
			int BlueOffSet = 0;
			int GreenOffSet = 0;
			int Hz = 256; //Cycles per sec
			uint32 RunningSampleIndex = 0;
			//int SquareWaveCounter = 0;
			int SquareWavePeriod = SamplesPerSecond/Hz;
			int HalfSquareWavePeriod = SquareWavePeriod / 2;
			int BytesPerSample = sizeof(int16) * 2;
			int SecondaryBufferSize = SamplesPerSecond * BytesPerSample;

			
			Win32InitDSound(Window, SamplesPerSecond, SecondaryBufferSize);
			SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

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
				RenderWeirdGradient(&GlobalBackBuffer, BlueOffSet, GreenOffSet);

				DWORD PlayCursor;
				DWORD WriteCursor;

				/*
				IDirectSoundBuffer8::GetCurrentPosition Method
				The GetCurrentPosition method retrieves the position of the play 
				and write cursors in the sound buffer.
				*/
				if (SUCCEEDED(SecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
				{

					DWORD BytesToLock = RunningSampleIndex * BytesPerSample % SecondaryBufferSize;
					DWORD BytesToWrite;
					
					
					if (BytesToLock > PlayCursor)
					{
						BytesToWrite = (SecondaryBufferSize - BytesToLock);
						BytesToWrite += PlayCursor;
					}
					else
					{
						BytesToWrite = PlayCursor - BytesToLock;
					}
					VOID *Region1;
					DWORD Region1Size;
					VOID *Region2;
					DWORD Region2Size;

					/*
					IDirectSoundBuffer8::Lock Method
					The Lock method readies all or part of the buffer for a data
					write and returns pointers to which data can be written.
					*/
					if (SUCCEEDED(SecondaryBuffer->Lock(WriteCursor, BytesToWrite, 
						&Region1, &Region1Size,
						&Region2, &Region2Size, 0)))
					{

						int16 *SampleOut = (int16 *)Region1;
						DWORD Region1SampleCount = Region1Size / BytesPerSample;
						for (DWORD SampleIndex = 0;
							SampleIndex < Region1SampleCount;
							++SampleIndex)
						{
							
							int16 SampleValue = ((RunningSampleIndex++ > HalfSquareWavePeriod) % 2) ? 1600 : -16000;
							*SampleOut++ = SampleValue;
							*SampleOut++ = SampleValue;
							
						}

						DWORD Region2SampleCount = Region2Size / BytesPerSample;
						for (DWORD SampleIndex = 0;
							SampleIndex < Region2SampleCount;
							++SampleIndex)
						{
							
							int16 SampleValue = ((RunningSampleIndex++ > HalfSquareWavePeriod) % 2) ? 1600 : -16000;
							*SampleOut++ = SampleValue;
							*SampleOut++ = SampleValue;
				
						}

						SecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
					}
				}
				HDC DeviceContext = GetDC(Window);
				win32WindowDimension Dimension = getWindowDimension(Window);
				Win32UpdateWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
				ReleaseDC(Window, DeviceContext);

				++BlueOffSet;
				//++GreenOffSet;
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
