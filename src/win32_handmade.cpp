#include <windows.h>

//entry point
int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow)
{
	/*Displays a modal dialog box that contains a 
	system icon, a set of buttons, and a brief 
	application-specific message.
	*/
	MessageBox(0, "Message", "Title", 
		MB_CANCELTRYCONTINUE | MB_ICONEXCLAMATION);//button and icon
	return(0);
}
