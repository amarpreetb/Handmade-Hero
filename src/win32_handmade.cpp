
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


#include "handmade.cpp"


#include <malloc.h>
#include <windows.h>
#include <stdio.h>
#include <xinput.h>
#include <dsound.h>

#include "win32_handmade.h"
//global
global_variable bool GlobalRunning;
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


internal debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename)
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
			Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (Result.Contents)
			{
				DWORD BytesRead;
				if (ReadFile(FileHandle, Result.Contents, FileSize.QuadPart, &BytesRead, 0) && (FileSize32 == BytesRead))
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

internal void DEBUGPlatformFreeFileMemory(void *Memory)
{

	if (Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}

}

internal bool32 DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize, void *Memory)
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

internal void *PlatformLoadFile(char *Filename)
{
	return 0;
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

	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
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
			GlobalRunning = true;

			win32SoundOutput SoundOutput = {};
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.BytesPerSample = sizeof(int16) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			win32ClearBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			GlobalRunning = true;

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

				while (GlobalRunning)
				{
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

							real32 X = win32ProcessXInputStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
							
							real32 Y = win32ProcessXInputStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
					
							//int16 StickX = (real32)Pad->sThumbLX; //Left thumbstick x-axis value
							//int16 StickY = (real32)Pad->sThumbLY; //Left thumbstick x-axis value

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

					//int16 Samples[40000 * 2];

					game_SoundOutputBuffer SoundBuffer = {};
					SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
					SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
					SoundBuffer.Samples = Samples;

					gameOffScreenBuffer Buffer = {};
					Buffer.Memory = GlobalBackBuffer.Memory;
					Buffer.Width = GlobalBackBuffer.Width;
					Buffer.Height = GlobalBackBuffer.Height;
					Buffer.Pitch = GlobalBackBuffer.Pitch;
					gameUpdateAndRender(&GameMemory, NewInput, &Buffer, &SoundBuffer);

					if (SoundIsValid)
					{
						Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
					}

					HDC DeviceContext = GetDC(Window);
					win32WindowDimension Dimension = getWindowDimension(Window);
					Win32UpdateWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
					ReleaseDC(Window, DeviceContext);



					int64 EndCycleCount = __rdtsc();

					LARGE_INTEGER EndCounter;
					QueryPerformanceCounter(&EndCounter);

					uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
					int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
					real64 msPerFrame = (((1000.0f * (real64)CounterElapsed) / (real64)PerfCountFrequency));
					//int64 msPerFrame = ((1000 * CounterElapsed) / PerfCountFrequency);
					real64 FPS = (real64)PerfCountFrequency / (real64)CounterElapsed;
					real64 MCPF = ((real64)CyclesElapsed / (1000.0f * 1000.0f));//megacycles perframe
#if 0
					char Buffer[256];
					wsprintf(Buffer, "Millisec/frame: %d  %dFPS  %dCycles/Frame\n", msPerFrame, FPS, MCPF);
					OutputDebugStringA(Buffer);
#endif				
					LastCycleCount = EndCycleCount;
					LastCounter = EndCounter;

					game_input *Temp = NewInput;
					NewInput = OldInput;
					OldInput = Temp;

				}

			}
			else{
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