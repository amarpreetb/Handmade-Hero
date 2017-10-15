#include <windows.h>

//WindowProc callback function
LRESULT CALLBACK WindowCallback( 
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
			OutputDebugStringA("wm_destroy\n");
		}break;

		case WM_CLOSE:
		{
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
			static DWORD Operation = WHITENESS;
			PatBlt(DeviceContext, x, y, Width, Height, Operation);
			EndPaint(Window, &Paint);

			if (Operation == WHITENESS)
			{
				Operation = BLACKNESS;
			}
			else
			{
				Operation = WHITENESS;
			}
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
	WindowClass.lpfnWndProc = WindowCallback;
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
			MSG Message;
			for (;;)
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
