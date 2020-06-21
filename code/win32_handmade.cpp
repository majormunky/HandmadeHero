#include <windows.h>

#define internal static;
#define local_persist static;
#define global_variable static;


global_variable bool Running;
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;

internal void Win32ResizeDIBSection(int width, int height) {

	if (BitmapMemory) {
		VirtualFree(BitmapMemory, 0, MEM_RELEASE);
	}

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = width;
	BitmapInfo.bmiHeader.biHeight = height;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;

	int BytesPerPixel = 4;
	int BitmapMemorySize = BytesPerPixel * (width * height);
	BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32UpdateWindow(HDC DeviceContext, int x, int y, int width, int height) {

	StretchDIBits(
		DeviceContext,
		x, y, width, height,
		x, y, width, height,
		0, 0, DIB_RGB_COLORS, SRCCOPY
	);
}


LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
	LRESULT result = 0;
	switch(Message) {
		case WM_SIZE: {
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);
			int width = ClientRect.right - ClientRect.left;
			int height = ClientRect.bottom - ClientRect.top;
			Win32ResizeDIBSection(width, height);
			OutputDebugStringA("WM_SIZE\n");
		} break;
		case WM_DESTROY: {
			OutputDebugStringA("WM_DESTROY\n");
			Running = false;
		} break;
		case WM_CLOSE: {
			OutputDebugStringA("WM_CLOSE\n");
			Running = false;
		} break;
		case WM_ACTIVATEAPP: {
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;
		case WM_PAINT: {
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			int x = Paint.rcPaint.left;
			int y = Paint.rcPaint.top;
			int height = Paint.rcPaint.bottom - Paint.rcPaint.top;
			int width = Paint.rcPaint.right - Paint.rcPaint.left;
			Win32UpdateWindow(DeviceContext, x, y, width, height);
			EndPaint(Window, &Paint);
		} break;
		default: {
			result = DefWindowProc(Window, Message, WParam, LParam);
		} break;
	}
	return(result);
}


int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR LpCmdLine, int ncmdShow) {
	WNDCLASS WindowClass = {};

	WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = hInstance;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClass(&WindowClass)) {
		HWND WindowHandle = CreateWindowEx(0, WindowClass.lpszClassName, "Handmade Hero",
			WS_OVERLAPPEDWINDOW|WS_VISIBLE,
			CW_USEDEFAULT,CW_USEDEFAULT,
			CW_USEDEFAULT,CW_USEDEFAULT,
			0, 0, hInstance, 0
		);
		if (WindowHandle) {
			MSG Message;
			Running = true;
			while(Running) {
				BOOL MessageResult = GetMessage(&Message, 0, 0, 0);	
				if (MessageResult > 0) {
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				} else {
					break;
				}
			}
		}
	} else {

	}

	return(0);
}
