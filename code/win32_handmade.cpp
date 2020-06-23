#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

#define internal static;
#define local_persist static;
#define global_variable static;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

struct win32_offscreen_buffer {
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct win32_window_size {
	int Width;
	int Height;
};

global_variable bool Running;
global_variable win32_offscreen_buffer BackBuffer;
global_variable LPDIRECTSOUNDBUFFER SecondarySoundBuffer;

// buncha junk to deal with loading XInput
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) {
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) {
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void Win32InitSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize) {
	// load library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

	if (DSoundLibrary) {
		// get directsound object
		direct_sound_create *DirectSoundCreate = (direct_sound_create *) GetProcAddress(DSoundLibrary, "DirectSoundCreate");
		LPDIRECTSOUND DirectSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;

			if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))) {
				// create a primary sound buffer
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0))) {
					if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat))) {
						OutputDebugStringA("Primary sound buffer format was set\n");
					} else {
						OutputDebugStringA("Problem setting format for primary sound buffer\n");
					}
				}

				// create a secondary sound buffer
				DSBUFFERDESC SecondaryBufferDescription = {};
				SecondaryBufferDescription.dwSize = sizeof(SecondaryBufferDescription);
				SecondaryBufferDescription.dwBufferBytes = BufferSize;
				SecondaryBufferDescription.lpwfxFormat = &WaveFormat;
				SecondaryBufferDescription.dwFlags = 0;

				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&SecondaryBufferDescription, &SecondarySoundBuffer, 0))) {
					OutputDebugStringA("Secondary sound buffer format was set\n");
				} else {
					OutputDebugStringA("Problem setting format for secondary sound buffer\n");
				}

				// start the sound playing			
			}
		}
		
	}
	
}

internal void Win32LoadXInput(void) {
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (!XInputLibrary) {
		XInputLibrary = LoadLibraryA("xinput1_3.dll");	
	}
	if (XInputLibrary) {
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
	}
}

// Done loading x input junk

internal win32_window_size Win32GetWindowDimensions(HWND Window) {
	win32_window_size result;
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	result.Width = ClientRect.right - ClientRect.left;
	result.Height = ClientRect.bottom - ClientRect.top;
	return(result);
}


internal void Win32DrawGradient(win32_offscreen_buffer *Buffer, int xOffset, int yOffset) {
	uint8 *Row = (uint8 *)Buffer->Memory;
	for (int Y = 0; Y < Buffer->Height; ++Y) {
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0; X < Buffer->Width; ++X) {
			uint8 blue = X + xOffset;
			uint8 green = Y + yOffset;
			uint8 red = 0;

			*Pixel++ = ((green << 8) | blue);
		}
		Row += Buffer->Pitch;
	}
}


internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int width, int height) {
	if (Buffer->Memory) {
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = width;
	Buffer->Height = height;
	Buffer->BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = Buffer->BytesPerPixel * (Buffer->Width * Buffer->Height);
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
	Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;
}

internal void Win32DisplayBuffer(HDC DeviceContext, int WindowWidth, int WindowHeight, win32_offscreen_buffer *Buffer, int x, int y, int width, int height) {
	StretchDIBits(
		DeviceContext,
		0, 0, WindowWidth, WindowHeight,
		0, 0, Buffer->Width, Buffer->Height,
		Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY
	);
}


LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
	LRESULT result = 0;
	switch(Message) {
		case WM_SIZE: {
			win32_window_size WindowSize = Win32GetWindowDimensions(Window);
			Win32ResizeDIBSection(&BackBuffer, WindowSize.Width, WindowSize.Height);
			OutputDebugStringA("WM_SIZE\n");
		} break;
		case WM_DESTROY: {
			OutputDebugStringA("WM_DESTROY\n");
			Running = false;
		} break;
		case WM_SYSKEYDOWN: 
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP: {
			uint32 VKCode = WParam;
			bool WasDown = ((LParam & (1 << 30)) != 0);
			bool IsDown = ((LParam & (1 << 31)) == 0);

			if (WasDown != IsDown)  {
				if (VKCode == 'W') { // W

				} else if (VKCode == 'A') { // A

				} else if (VKCode == 'S') { // S

				} else if (VKCode == 'D') { // D

				} else if (VKCode == 'Q') { // Q

				} else if (VKCode == 'E') { // E

				} else if (VKCode == VK_UP) {

				} else if (VKCode == VK_DOWN) {

				} else if (VKCode == VK_LEFT) {

				} else if (VKCode == VK_RIGHT) {

				} else if (VKCode == VK_ESCAPE) {

				} else if (VKCode == VK_SPACE) {

				}	
			}

			bool32 AltKeyWasDown = (LParam & (1 << 29));
			if ((VKCode == VK_F4) && AltKeyWasDown) { 
				Running = false;
			}
		} break;
		case WM_CLOSE: {
			OutputDebugStringA("WM_CLOSE\n");
			Running = false;
		} break;
		case WM_ACTIVATEAPP: {
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;
		case WM_PAINT: {
			win32_window_size WindowSize = Win32GetWindowDimensions(Window);

			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			int x = Paint.rcPaint.left;
			int y = Paint.rcPaint.top;
			int height = Paint.rcPaint.bottom - Paint.rcPaint.top;
			int width = Paint.rcPaint.right - Paint.rcPaint.left;
			Win32DisplayBuffer(DeviceContext, WindowSize.Width, WindowSize.Height, &BackBuffer, x, y, width, height);
			EndPaint(Window, &Paint);
		} break;
		default: {
			result = DefWindowProc(Window, Message, WParam, LParam);
		} break;
	}
	return(result);
}


int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR LpCmdLine, int ncmdShow) {
	Win32LoadXInput();

	WNDCLASS WindowClass = {};

	Win32ResizeDIBSection(&BackBuffer, 1280, 720);

	WindowClass.style = CS_HREDRAW|CS_VREDRAW;
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

			// Graphics test stuff
			int xOffset = 0;
			int yOffset = 0;

			// Sound stuff
			int SamplesPerSecond = 48000;			
			int ToneHz = 256;
			uint32 RunningSampleIndex = 0;
			int BytesPerSample = sizeof(int16) * 2;
			int SquareWavePeriod = SamplesPerSecond / ToneHz;
			int HalfSquareWavePeriod = SquareWavePeriod / 2;
			int SecondarySoundBufferSize = SamplesPerSecond * BytesPerSample;

			Win32InitSound(WindowHandle, SamplesPerSecond, SecondarySoundBufferSize);
			SecondarySoundBuffer->Play(0, 0, DSBPLAY_LOOPING);

			win32_window_size WindowSize = Win32GetWindowDimensions(WindowHandle);
			while(Running) {
				while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
					if (Message.message == WM_QUIT) {
						Running = false;	
					}
					TranslateMessage(&Message);
					DispatchMessage(&Message);	
				}

				// handle controller input
				for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex) {
					XINPUT_STATE ControllerState;
					if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS) {
						// tihs controller is plugged in
						XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
						bool DPadUp = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool DPadDown = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool DPadLeft = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool DPadRight = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool BtnStart = (Pad->wButtons & XINPUT_GAMEPAD_START);
						bool BtnBack = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool BtnLeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool BtnRightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool BtnA = (Pad->wButtons & XINPUT_GAMEPAD_A);
						bool BtnB = (Pad->wButtons & XINPUT_GAMEPAD_B);
						bool BtnX = (Pad->wButtons & XINPUT_GAMEPAD_X);
						bool BtnY = (Pad->wButtons & XINPUT_GAMEPAD_Y);
						int16 StickX = Pad->sThumbLX;
						int16 StickY = Pad->sThumbLY;

						if (BtnA) {
							yOffset++;
						}
					} else {
						// no controller here
					}
				}

				Win32DrawGradient(&BackBuffer, xOffset, yOffset);
				DWORD PlayCursor;
				DWORD WriteCursor;

				if (SUCCEEDED(SecondarySoundBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor))) {
					DWORD ByteToLock = RunningSampleIndex * BytesPerSample % SecondarySoundBufferSize;
					DWORD BytesToWrite;

					if (ByteToLock > PlayCursor) {
						BytesToWrite = (SecondarySoundBufferSize - ByteToLock);
						BytesToWrite += PlayCursor;
					} else {
						BytesToWrite = PlayCursor - ByteToLock;
					}

					VOID *Region1;
					DWORD Region1Size;
					VOID *Region2;
					DWORD Region2Size;
					int16 Volume = 8000;

					if (SUCCEEDED(SecondarySoundBuffer->Lock(ByteToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0))) {
						int16 *SampleOut = (int16 *)Region1;
						DWORD Region1SampleCount = Region1Size / BytesPerSample;
						for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex) {
							int16 SampleValue = ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2) ? Volume : -Volume;
							*SampleOut++ = SampleValue;
							*SampleOut++ = SampleValue;
						}
						
						DWORD Region2SampleCount = Region2Size / BytesPerSample;
						SampleOut = (int16 *)Region2;
						for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex) {
							int16 SampleValue = ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2) ? Volume : -Volume;
							*SampleOut++ = SampleValue;
							*SampleOut++ = SampleValue;
						}		
					}
				}

				HDC DeviceContext = GetDC(WindowHandle);
				Win32DisplayBuffer(DeviceContext, WindowSize.Width, WindowSize.Height, &BackBuffer, 0, 0, WindowSize.Width, WindowSize.Height);
				xOffset++;

				ReleaseDC(WindowHandle, DeviceContext);
			}
		}
	} else {

	}

	return(0);
}
