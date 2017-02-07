#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef float real32;
typedef double real64;

typedef int32 bool32;

#define internal static
#define local_persist static
#define global_variable static

struct Win32Bitmap {
	BITMAPINFO Info;
	void* Memory;
	int Width;
	int Height;
	int Pitch;
};

#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef XINPUT_GET_STATE(_Win32XInputGetState);
XINPUT_GET_STATE(XInputGetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable _Win32XInputGetState* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define XINPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef XINPUT_SET_STATE(_Win32XInputSetState);
XINPUT_SET_STATE(InputSetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable _Win32XInputSetState* XInputSetState_ = InputSetStateStub;
#define XInputSetState XInputSetState_

#define DSOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
typedef DSOUND_CREATE(_Win32DirectSoundCreate);

global_variable bool GlobalRunning;
global_variable bool SoundIsPlaying;
global_variable Win32Bitmap GlobalBitmap;
global_variable LPDIRECTSOUNDBUFFER GlobalSoundBuffer;

global_variable const real32 PI32 = 3.14159265359f;

internal void
Win32LoadXInput()
{
	HMODULE XInputLibrary = LoadLibrary("xinput1_4.dll");
	if(!XInputLibrary)
	{
		XInputLibrary = LoadLibrary("xinput1_3.dll");
	}

	if(XInputLibrary)
	{
		XInputGetState = (_Win32XInputGetState*)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (_Win32XInputSetState*)GetProcAddress(XInputLibrary, "XInputSetState");
	}
	else
	{
		//Can't find xinput_whatever.dll
	}
}

internal void
Win32InitDSound(HWND hwnd, int channels, int samplesPerSecond, int bitsPerSample)
{
	HMODULE DSoundLibrary = LoadLibrary("dsound.dll");

	if (DSoundLibrary)
	{
		_Win32DirectSoundCreate* DirectSoundCreate = 
			(_Win32DirectSoundCreate*)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
		
		LPDIRECTSOUND DirectSound;
		if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			if(SUCCEEDED(DirectSound->SetCooperativeLevel(hwnd, DSSCL_PRIORITY)))
			{
				WAVEFORMATEX waveFormat = {};
				waveFormat.wFormatTag = WAVE_FORMAT_PCM;
				waveFormat.nChannels = channels;
				waveFormat.nSamplesPerSec = samplesPerSecond;
				waveFormat.wBitsPerSample = bitsPerSample;
				waveFormat.nBlockAlign = channels * bitsPerSample / 8;
				waveFormat.nAvgBytesPerSec = samplesPerSecond * waveFormat.nBlockAlign;
				waveFormat.cbSize = 0;

				DSBUFFERDESC bufferDesc = {};
				bufferDesc.dwSize = sizeof(bufferDesc);
				bufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;

				LPDIRECTSOUNDBUFFER soundBufferPrimary;
				if(SUCCEEDED(DirectSound->CreateSoundBuffer(&bufferDesc, &soundBufferPrimary, 0)))
				{
					if(SUCCEEDED(soundBufferPrimary->SetFormat(&waveFormat)))
					{
					}
					else
					{

					}
				}
				else
				{

				}

				DSBUFFERDESC bufferDesc2 = {};
				bufferDesc2.dwSize = sizeof(bufferDesc2);
				//bufferDesc2.dwFlags = 0;
				//One second of sound buffer
				bufferDesc2.dwBufferBytes = waveFormat.nAvgBytesPerSec;
				bufferDesc2.lpwfxFormat = &waveFormat;
				if(SUCCEEDED(DirectSound->CreateSoundBuffer(&bufferDesc2, &GlobalSoundBuffer, 0)))
				{
				}
				else
				{
				}
			}
			else
			{

			}
		}
		else
		{
			//Create Direct Sound Failed
		}
	}
}

internal void
Win32FillSoundBuffer(int channels, int samplesPerSecond, int bitsPerSample, int32 runningSampleIndex)
{
	real32 soundVolume = 500.0f;
	int bytesPerSample = channels * bitsPerSample / 8;
	int soundBufferSize = samplesPerSecond * bytesPerSample;

	DWORD playCursor;
	DWORD writeCursor;
	if(SUCCEEDED(GlobalSoundBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
	{
		DWORD byteToLock = (runningSampleIndex * bytesPerSample) % soundBufferSize;
		DWORD bytesToWrite;
		if (byteToLock == playCursor)
		{
			if (SoundIsPlaying)
			{
				bytesToWrite = 0;
			}
			else
			{
				bytesToWrite = soundBufferSize;
			}
		}
		else if (byteToLock > playCursor)
		{
			bytesToWrite = soundBufferSize - (byteToLock - playCursor);
		}
		else
		{
			bytesToWrite = playCursor - byteToLock;
		}

		VOID* region1;
		DWORD regionSize1;
		VOID* region2;
		DWORD regionSize2;
		if (SUCCEEDED(GlobalSoundBuffer->Lock(byteToLock, bytesToWrite,
			&region1, &regionSize1, &region2, &regionSize2,	0)))
		{
			int16* sampleOut = (int16*)region1;
			DWORD region1SampleCount = regionSize1 / bytesPerSample;
			DWORD region2SampleCount = regionSize2 / bytesPerSample;
			for(DWORD sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex)
			{
				int16 sampleValue = (int16)(sin(runningSampleIndex*PI32*0.05f) * soundVolume);
				for(int ch = 0; ch < channels; ++ch)
				{
					*sampleOut++ = sampleValue;
				}
				++runningSampleIndex;
			}
			for(DWORD sampleIndex = 0; sampleIndex < region2SampleCount; ++sampleIndex)
			{
				int16 sampleValue = (int16)(sin(runningSampleIndex*PI32*0.05f) * soundVolume);
				for(int ch = 0; ch < channels; ++ch)
				{
					*sampleOut++ = sampleValue;
				}
				++runningSampleIndex;
			}
			GlobalSoundBuffer->Unlock(region1, regionSize1, region2, regionSize2);
		}
	}
}

internal void
RenderGradient(const Win32Bitmap& bitmap, int xOffset, int yOffset)
{
	uint8* row = (uint8*)bitmap.Memory;
	for(int y=0; y<bitmap.Height; ++y)
	{
		uint32* pixel = (uint32*)row;
		for(int x=0; x<bitmap.Width; ++x)
		{
			uint8 blue = x + xOffset;
			uint8 green = y + yOffset;
			*pixel++ = (green << 8) | blue;
		}
		row += bitmap.Pitch;
	}
}

internal void
Win32ResizeDIBSection(Win32Bitmap& bitmap, int width, int height)
{
	if(bitmap.Memory)
	{
		VirtualFree(bitmap.Memory, 0, MEM_RELEASE);
	}

	bitmap.Info.bmiHeader.biSize = sizeof(bitmap.Info.bmiHeader);
	bitmap.Info.bmiHeader.biWidth = bitmap.Width = width;
	bitmap.Info.bmiHeader.biHeight = -(bitmap.Height = height);
	bitmap.Info.bmiHeader.biPlanes = 1;
	bitmap.Info.bmiHeader.biBitCount = 32;
	bitmap.Info.bmiHeader.biCompression = BI_RGB;
	bitmap.Pitch = 4*bitmap.Width;

	bitmap.Memory = VirtualAlloc(0, bitmap.Pitch*bitmap.Height, 
		MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

internal void
Win32BlitBuffer(const Win32Bitmap& bitmap, HDC dc, int width, int height)
{
	StretchDIBits(dc,
		0, 0, width, height,
		0, 0, bitmap.Width, bitmap.Height,
		bitmap.Memory, &bitmap.Info,
		DIB_RGB_COLORS, SRCCOPY);
}


LRESULT CALLBACK
Win32MainWindowCallback(HWND   hwnd,
                   UINT   uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
	LRESULT result = 0;

	switch (uMsg)
	{
		case WM_DESTROY:
		{
			GlobalRunning = false;
		}break;

		case WM_CLOSE:
		{
			GlobalRunning = false;
		}break;

		case WM_ACTIVATEAPP:
		{
			OutputDebugString("WM_ACTIVATEAPP\n");
		}break;

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			uint32 VKCode = wParam;
			uint32 altKeyDown = lParam & (1 << 29);
			bool wasDown = (lParam & (1 << 30) != 0);
			bool isDown = (lParam & (1 << 31) == 0);

			if (VKCode == VK_UP)
			{

			}
			if (VKCode == VK_ESCAPE)
			{

			}
			else if (VKCode == 'W')
			{

			}
			else if(VKCode == VK_F4 && altKeyDown)
			{
				GlobalRunning = false;
			}
		}break;

		// Do update window when resizing
		case WM_PAINT:
		{
			PAINTSTRUCT paint;
			HDC dc = BeginPaint(hwnd, &paint);
			RECT rect = paint.rcPaint;
			int width = rect.right - rect.left;
			int height = rect.bottom - rect.top;
			Win32BlitBuffer(GlobalBitmap, dc, width, height);
			EndPaint(hwnd, &paint);
		}break;

		default:
		{
			result = DefWindowProc(hwnd, uMsg, wParam, lParam);
		}break;
	}

	return result;
}

int CALLBACK 
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR     lpCmdLine,
        int       nCmdShow)
{
	WNDCLASS windowClass = {};
	windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	windowClass.lpfnWndProc = Win32MainWindowCallback;
	windowClass.hInstance = hInstance;
//	windowClass.hIcon = ;
	windowClass.lpszClassName = "GamewindowClass";

	if(RegisterClass(&windowClass))
	{
		HWND hwnd = CreateWindowEx(0,
			windowClass.lpszClassName, "Game Project",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			0, 0, hInstance, 0);
		if(hwnd)
		{
			HDC dc = GetDC(hwnd);

			GlobalRunning = true;
			int xOffset = 0;
			int yOffset = 0;

			int samplesPerSecond = 48000;
			int channels = 2;
			int bitsPerSample = 16;
			int32 runningSampleIndex = 0;

			Win32LoadXInput();
			Win32ResizeDIBSection(GlobalBitmap, 1280, 720);
			Win32InitDSound(hwnd, channels, samplesPerSecond, bitsPerSample);
			Win32FillSoundBuffer(channels, samplesPerSecond, bitsPerSample, runningSampleIndex);
			GlobalSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);
			SoundIsPlaying = true;

			while(GlobalRunning)
			{
				MSG message;
				while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
				{
					if (message.message == WM_QUIT)
					{
						GlobalRunning = false;
					}
					TranslateMessage(&message);
					DispatchMessage(&message);
				}

				for (DWORD i=0; i<XUSER_MAX_COUNT; ++i)
				{
					XINPUT_STATE state;
			        if( XInputGetState(i, &state) == ERROR_SUCCESS )
					{ 
					    // Controller is connected 
					    XINPUT_GAMEPAD* pad = &state.Gamepad;
					    bool dpadUp = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
					    bool dpadA = (pad->wButtons & XINPUT_GAMEPAD_A);

					    int16 stickLX = pad->sThumbLX;
					    int16 stickLY = pad->sThumbLY;

					    if (dpadA)
					    {
					    	--yOffset;
					    	XINPUT_VIBRATION vibration;
					    	vibration.wLeftMotorSpeed = 1000;
					    	vibration.wRightMotorSpeed = 1000;
					    	XInputSetState(0, &vibration);
					    }
					}
				    else
					{
				        // Controller is not connected 
					}
				}
				
				Win32FillSoundBuffer(channels, samplesPerSecond, bitsPerSample, runningSampleIndex);
				RenderGradient(GlobalBitmap, xOffset, yOffset);
				
				RECT rect;
				GetClientRect(hwnd, &rect);
				int width = rect.right - rect.left;
				int height = rect.bottom - rect.top;
				Win32BlitBuffer(GlobalBitmap, dc, width, height);

				++xOffset;
				++yOffset;
			}
		}
		else
		{
			//Failed create window
		}
	}
	else
	{
		//Register Fail
	}

	return 0;
}