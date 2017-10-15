#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

//global
global_variable bool Running;
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDeviceContext;

//Device - Independent Bitmaps
internal void Win32ResizeDIBSection(int Width, int Height)
{
	if (BitmapHandle)
	{
		/*
		The DeleteObject function deletes a logical pen, brush, font, bitmap, 
		region, or palette, freeing all system resources associated with the 
		object. 
		*/
		DeleteObject(BitmapHandle);
	}
	
	if (BitmapDeviceContext != 0)
	{
		/*
		The CreateCompatibleDC function creates a memory device context (DC) 
		compatible with the specified device.
		*/
		BitmapDeviceContext = CreateCompatibleDC(0);
		
	}
	/*
	The BITMAPINFOHEADER structure contains information about 
	the dimensions and color format of a DIB.
	*/
	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = Width;
	BitmapInfo.bmiHeader.biHeight = Height;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;
	
	/*
	The CreateDIBSection function creates a DIB that 
	applications can write to directly.
	*/
	BitmapHandle = CreateDIBSection(
		BitmapDeviceContext,
		&BitmapInfo,
		DIB_RGB_COLORS,
		&BitmapMemory,
		0,0);
}

internal void Win32UpdateWindow(HDC DeviceContext, int x, int y, int Width, int Height)
{
	/*
	The StretchDIBits function copies the color data for a rectangle of pixels in
	a DIB, JPEG, or PNG image to the specified destination rectangle. 
	*/
	StretchDIBits(
		DeviceContext,
		x, y, Width, Height,
		x, y, Width, Height,
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
			Win32UpdateWindow(DeviceContext, x, y, Width, Height);
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
		HWND WindowHandle =
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
		if (WindowHandle != NULL)
		{
			Running = true;
			MSG Message;
			while (Running)
			{
				BOOL MessageResult = GetMessage(&Message, 0, 0, 0);

				if (MessageResult > 0)
				{
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}
				else
				{
					break;
				}
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
