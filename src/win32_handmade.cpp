#include <windows.h>
#include <stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;


typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#define internal static
#define local_persist static
#define global_variable static

//global
global_variable bool Running;
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;

internal void RenderWeirdGradient(int xOffSet, int yOffSet)
{
	int Width = BitmapWidth;
	int Height = BitmapHeight;

	int Pitch = Width * BytesPerPixel;
	uint8 *Row = (uint8 *)BitmapMemory;

	for (int y = 0; y < BitmapHeight; ++y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int x = 0; x < BitmapWidth; ++x)
		{
			uint8 Blue = (x + xOffSet);

			uint8 Green = (y + yOffSet);

			*Pixel++ = ((Green << 8) | Blue);
		}

		Row += Pitch;
	}
}

//Device - Independent Bitmaps
internal void Win32ResizeDIBSection(int Width, int Height)
{
	//release
	if (BitmapMemory)
	{
		VirtualFree(BitmapMemory, 0, MEM_RELEASE);
	}

	BitmapWidth = Width;
	BitmapHeight = Height;

	/*
	The BITMAPINFOHEADER structure contains information about 
	the dimensions and color format of a DIB.
	*/
	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = BitmapWidth;
	BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;
	
	int BitmapMemorySize = (Width * Height) * BytesPerPixel;
	BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	//RenderWeirdGradient(128, 0);
}

internal void Win32UpdateWindow(HDC DeviceContext, RECT *ClientRect,int x, int y, int Width, int Height)
{
	/*
	The RECT structure defines the coordinates of the upper-left and lower-right corners of a rectangle.
	*/
	int WindowWidth = ClientRect->right - ClientRect->left;
	int WindowHeight = ClientRect->bottom - ClientRect->top;

	/*
	The StretchDIBits function copies the color data for a rectangle of pixels in
	a DIB, JPEG, or PNG image to the specified destination rectangle. 
	*/
	StretchDIBits(
		DeviceContext,
		0, 0, BitmapWidth, BitmapHeight,
		0, 0, WindowWidth, WindowHeight,
		BitmapMemory,
		&BitmapInfo,
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
			
			RECT ClientRect;
			// Retrieves the coordinates of a window's client area
			GetClientRect(Window, &ClientRect); 
			int Width = ClientRect.right - ClientRect.left;
			int Height = ClientRect.bottom - ClientRect.top;
			Win32ResizeDIBSection(Width, Height);
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
			
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);
			Win32UpdateWindow(DeviceContext, &ClientRect, x, y, Width, Height);
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
	//WNDCLASS structure
	WNDCLASS WindowClass = {};

	WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	WindowClass.lpfnWndProc = Win32WindowCallback;
	WindowClass.hInstance = hInstance;
	//WindowClass.hIcon;
	WindowClass.lpszClassName = "HandmadeHeroWindowsClass";

	// CreateWindowEx function
	if (RegisterClass(&WindowClass))
	{
		HWND Window =
			CreateWindowEx(
				0,							//Extended Window Styles
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
			int xOffSet = 0;
			int yOffSet = 0;
			
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
				RenderWeirdGradient(xOffSet, yOffSet);

				HDC DeviceContext = GetDC(Window);
				RECT ClientRect;
				GetClientRect(Window, &ClientRect);
				int WindowWidth = ClientRect.right - ClientRect.left;
				int WindowHeight = ClientRect.bottom - ClientRect.top;
				Win32UpdateWindow(DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);
				ReleaseDC(Window, DeviceContext);

				++xOffSet;
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
