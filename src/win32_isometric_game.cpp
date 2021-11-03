#include <dsound.h>
#include <math.h>
#include <stdint.h>
#include <windows.h>
#include <xinput.h>

#define global_variable static
#define local_persist   static
#define internal        static
#define pi32            3.14159265359f

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef int64_t s64;
typedef uint64_t u64;

typedef float float32;
typedef double float64;

struct Win32_Offscreen_Buffer
{
    BITMAPINFO info;
    void *memory;
    int width;
    int height;
    int pitch;
};

struct Win32_Window_Dimensions
{
    int width;
    int height;
};

struct Win32_Sound_Output
{
    int samplesPerSecond;
    int bytesPerSample;
    int toneHz;
    u32 runningsampleIndex;
    int wavePeriod;
    int secondaryBufferSize;
    s16 toneVolume;
    float32 tSine;
    int latencySampleCount;
};

#define DIRECT_SOUND_CREATE( name ) HRESULT WINAPI name( LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter )
typedef DIRECT_SOUND_CREATE( direct_sound_create );

#define X_INPUT_GET_STATE( name ) DWORD WINAPI name( DWORD dwUserIndex, XINPUT_STATE *pState )
#define X_INPUT_SET_STATE( name ) DWORD WINAPI name( DWORD dwUserIndex, XINPUT_VIBRATION *pVibration )
typedef X_INPUT_GET_STATE( x_input_get_state );
typedef X_INPUT_SET_STATE( x_input_set_state );

X_INPUT_GET_STATE( XInputGetStateStub )
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

X_INPUT_SET_STATE( XInputSetStateStub )
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

global_variable bool globalRunning;
global_variable Win32_Offscreen_Buffer globalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER globalSecondaryBuffer;

internal void Win32LoadXInput()
{
    HMODULE xInputLibrary = LoadLibrary( "xinput1_4.dll" );

    if ( !xInputLibrary )
    {
        xInputLibrary = LoadLibrary( "xinput9_1_0.dll" );
    }

    if ( !xInputLibrary )
    {
        xInputLibrary = LoadLibrary( "xinput1_3.dll" );
    }

    if ( xInputLibrary )
    {
        XInputGetState = ( x_input_get_state * ) GetProcAddress( xInputLibrary, "XInputGetState" );
        XInputSetState = ( x_input_set_state * ) GetProcAddress( xInputLibrary, "XInputSetState" );
    }
}

internal Win32_Window_Dimensions Win32GetWindowDimensions( HWND window )
{
    Win32_Window_Dimensions result;
    RECT clientRect;
    GetClientRect( window, &clientRect );
    result.width = clientRect.right - clientRect.left;
    result.height = clientRect.bottom - clientRect.top;
    return result;
}

internal void RenderWeirdGradient( Win32_Offscreen_Buffer *buffer, int xOffset, int yOffset )
{
    u8 *row = ( u8 * ) buffer->memory;
    for ( int y = 0; y < buffer->height; ++y )
    {
        u32 *pixel = ( u32 * ) row;
        for ( int x = 0; x < buffer->width; ++x )
        {
            u8 red = x + y + xOffset + yOffset;
            u8 green = y + yOffset;
            u8 blue = x + xOffset;
            *pixel++ = 0x00000000 | red << 16 | green << 8 | blue;
        }
        row += buffer->pitch;
    }
}

internal void Win32ResizeDIBSection( Win32_Offscreen_Buffer *buffer, int width, int height )
{
    if ( buffer->memory )
    {
        VirtualFree( buffer->memory, 0, MEM_RELEASE );
    }

    buffer->width = width;
    buffer->height = height;
    int bytesPerPixel = 4;

    buffer->info.bmiHeader.biSize = sizeof( buffer->info.bmiHeader );
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biClrImportant = BI_RGB;

    int bitmapMemorySize = bytesPerPixel * buffer->width * buffer->height;
    buffer->memory = VirtualAlloc( 0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE );
    buffer->pitch = width * bytesPerPixel;
}

internal void Win32DisplayBufferInWindow( HDC deviceContext, int windowWidth, int windowHeight,
                                          Win32_Offscreen_Buffer *buffer )
{
    StretchDIBits( deviceContext,
                   0, 0, windowWidth, windowHeight,
                   0, 0, buffer->width, buffer->height,
                   buffer->memory, &buffer->info,
                   DIB_RGB_COLORS, SRCCOPY );
}

internal void Win32InitDSound( HWND window, s32 samplesPerSec, s32 bufferSize )
{
    HMODULE dSoundLibrary = LoadLibrary( "dsound.dll" );
    if ( dSoundLibrary )
    {
        direct_sound_create *DirectSoundCreate = ( direct_sound_create * ) GetProcAddress( dSoundLibrary, "DirectSoundCreate" );
        LPDIRECTSOUND directSound;
        if ( DirectSoundCreate && SUCCEEDED( DirectSoundCreate( 0, &directSound, 0 ) ) )
        {
            WAVEFORMATEX waveFormat = {};
            waveFormat.wFormatTag = WAVE_FORMAT_PCM;
            waveFormat.nChannels = 2;
            waveFormat.nSamplesPerSec = samplesPerSec;
            waveFormat.wBitsPerSample = 16;
            waveFormat.nBlockAlign = ( waveFormat.nChannels * waveFormat.wBitsPerSample ) / 8;
            waveFormat.nAvgBytesPerSec = waveFormat.nBlockAlign * waveFormat.nSamplesPerSec;

            if ( SUCCEEDED( directSound->SetCooperativeLevel( window, DSSCL_PRIORITY ) ) )
            {
                DSBUFFERDESC bufferDescription = {};
                bufferDescription.dwSize = sizeof( bufferDescription );
                bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
                LPDIRECTSOUNDBUFFER primaryBuffer;
                if ( SUCCEEDED( directSound->CreateSoundBuffer( &bufferDescription, &primaryBuffer, 0 ) ) )
                {
                    if ( SUCCEEDED( primaryBuffer->SetFormat( &waveFormat ) ) )
                    {
                        OutputDebugString( "Primary buffer format set\n" );
                    }
                }
                DSBUFFERDESC secondaryBufferDescription = {};
                secondaryBufferDescription.dwSize = sizeof( secondaryBufferDescription );
                secondaryBufferDescription.dwFlags = 0;
                secondaryBufferDescription.dwBufferBytes = bufferSize;
                secondaryBufferDescription.lpwfxFormat = &waveFormat;
                if ( SUCCEEDED( directSound->CreateSoundBuffer( &secondaryBufferDescription, &globalSecondaryBuffer, 0 ) ) )
                {
                    OutputDebugString( "Secondary buffer format set\n" );
                }
            }
        }
    }
}

internal void Win32FillSoundBuffer( Win32_Sound_Output *soundOutput, DWORD byteToLock, DWORD bytesToWrite )
{
    VOID *region1;
    DWORD region1Size;
    VOID *region2;
    DWORD region2Size;

    if ( SUCCEEDED( globalSecondaryBuffer->Lock( byteToLock, bytesToWrite, &region1, &region1Size, &region2, &region2Size, 0 ) ) )
    {
        s16 *sampleOut = ( s16 * ) region1;
        DWORD region1SampleCount = region1Size / soundOutput->bytesPerSample;
        for ( DWORD sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex )
        {
            soundOutput->tSine += 2 * pi32 / ( float32 ) soundOutput->wavePeriod;
            float32 sineValue = sinf( soundOutput->tSine );
            s16 sampleValue = ( s16 ) ( sineValue * soundOutput->toneVolume );
            *sampleOut++ = sampleValue;
            *sampleOut++ = sampleValue;
            ++soundOutput->runningsampleIndex;
        }

        sampleOut = ( s16 * ) region2;
        DWORD region2SampleCount = region2Size / soundOutput->bytesPerSample;
        for ( DWORD sampleIndex = 0; sampleIndex < region2SampleCount; ++sampleIndex )
        {
            soundOutput->tSine += 2 * pi32 / ( float32 ) soundOutput->wavePeriod;
            float32 sineValue = sinf( soundOutput->tSine );
            s16 sampleValue = ( s16 ) ( sineValue * soundOutput->toneVolume );
            *sampleOut++ = sampleValue;
            *sampleOut++ = sampleValue;
            ++soundOutput->runningsampleIndex;
        }
        globalSecondaryBuffer->Unlock( region1, region1Size, region2, region2Size );
    }
}

LRESULT CALLBACK Win32MainWindowCallback( HWND window, UINT message, WPARAM wParam, LPARAM lParam )
{
    LRESULT result = 0;
    switch ( message )
    {
        case WM_SIZE:
        {
        }
        break;

        case WM_DESTROY:
        {
            globalRunning = false;
        }
        break;

        case WM_CLOSE:
        {
            globalRunning = false;
        }
        break;

        case WM_ACTIVATEAPP:
        {
        }
        break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            u32 vkCode = wParam;
            bool wasDown = ( lParam & ( 1 << 30 ) ) != 0;
            bool isDown = ( lParam & ( 1 << 31 ) ) == 0;
            bool altWasDown = ( lParam & ( 1 << 29 ) ) != 0;
            if ( wasDown != isDown )
            {
                if ( vkCode == 'W' )
                {
                }
                else if ( vkCode == 'A' )
                {
                }
                else if ( vkCode == 'S' )
                {
                }
                else if ( vkCode == 'D' )
                {
                }
                else if ( vkCode == 'Q' )
                {
                }
                else if ( vkCode == 'E' )
                {
                }
                else if ( vkCode == VK_UP )
                {
                }
                else if ( vkCode == VK_LEFT )
                {
                }
                else if ( vkCode == VK_DOWN )
                {
                }
                else if ( vkCode == VK_RIGHT )
                {
                }
                else if ( vkCode == VK_SPACE )
                {
                }
                else if ( vkCode == VK_ESCAPE )
                {
                }

                if ( vkCode == VK_F4 && altWasDown )
                {
                    globalRunning = false;
                }
            }
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint( window, &paint );
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;

            Win32_Window_Dimensions dimensions = Win32GetWindowDimensions( window );
            Win32DisplayBufferInWindow( deviceContext, dimensions.width, dimensions.height, &globalBackbuffer );
            EndPaint( window, &paint );
        }
        break;

        default:
        {
            result = DefWindowProc( window, message, wParam, lParam );
        }
        break;
    }
    return result;
}

int WinMain( HINSTANCE instance,
             HINSTANCE prevInstance,
             LPSTR commamdLine,
             int showCode )
{
    Win32LoadXInput();
    WNDCLASS windowClass = {};

    Win32ResizeDIBSection( &globalBackbuffer, 1280, 720 );

    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = Win32MainWindowCallback;
    windowClass.hInstance = instance;
    // windowClass.hIcon = ;
    windowClass.lpszClassName = "IsometricGameWindowClass";

    RegisterClass( &windowClass );

    HWND window = CreateWindowEx( 0, windowClass.lpszClassName, "Isometric Game", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0 );
    HDC deviceContext = GetDC( window );
    if ( window )
    {
        globalRunning = true;
        int xOffset = 0;
        int yOffset = 0;

        Win32_Sound_Output soundOutput = {};
        soundOutput.samplesPerSecond = 48000;
        soundOutput.bytesPerSample = sizeof( s16 ) * 2;
        soundOutput.toneHz = 256;
        soundOutput.runningsampleIndex = 0;
        soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.toneHz;
        soundOutput.secondaryBufferSize = soundOutput.bytesPerSample * soundOutput.samplesPerSecond;
        soundOutput.toneVolume = 1000;
        soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;

        Win32InitDSound( window, soundOutput.samplesPerSecond, soundOutput.secondaryBufferSize );
        Win32FillSoundBuffer( &soundOutput, 0, soundOutput.latencySampleCount * soundOutput.bytesPerSample );
        globalSecondaryBuffer->Play( 0, 0, DSBPLAY_LOOPING );

        while ( globalRunning )
        {
            MSG message;
            while ( PeekMessage( &message, 0, 0, 0, PM_REMOVE ) )
            {
                if ( message.message == WM_QUIT )
                {
                    globalRunning = false;
                }
                TranslateMessage( &message );
                DispatchMessage( &message );
            }

            for ( DWORD controllerIndex = 0; controllerIndex < XUSER_MAX_COUNT; ++controllerIndex )
            {
                XINPUT_STATE controllerState;
                if ( XInputGetState( controllerIndex, &controllerState ) == ERROR_SUCCESS )
                {
                    // controller available;
                    XINPUT_GAMEPAD *pad = &controllerState.Gamepad;
                    bool dpadDown = pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                    bool dpadUp = pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                    bool dpadRight = pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                    bool dpadLeft = pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                    bool start = pad->wButtons & XINPUT_GAMEPAD_START;
                    bool back = pad->wButtons & XINPUT_GAMEPAD_BACK;
                    bool leftThumb = pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
                    bool rightThumb = pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
                    bool leftShoulder = pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                    bool rightSoulder = pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
                    bool aButton = pad->wButtons & XINPUT_GAMEPAD_A;
                    bool bButton = pad->wButtons & XINPUT_GAMEPAD_B;
                    bool xButton = pad->wButtons & XINPUT_GAMEPAD_X;
                    bool yButton = pad->wButtons & XINPUT_GAMEPAD_Y;

                    s16 leftStickX = pad->sThumbLX;
                    s16 leftStickY = pad->sThumbLY;
                    s16 rightStickX = pad->sThumbRX;
                    s16 rightStickY = pad->sThumbRY;

                    u8 leftTrigger = pad->bLeftTrigger;
                    u8 rightTrigger = pad->bRightTrigger;

                    soundOutput.toneHz = 512 + ( int ) ( 256.0f * ( float32 ) leftStickY / 30000.0f );
                    soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.toneHz;

                    xOffset -= leftStickX / 4096;
                    yOffset += leftStickY / 4096;
                    // ++yOffset;
                    // XINPUT_VIBRATION vibration;
                    // vibration.wLeftMotorSpeed = 60000;
                    // vibration.wRightMotorSpeed = 60000;
                    // XInputSetState( 0, &vibration );
                }
                else
                {
                    // controller not available
                }
            }

            RenderWeirdGradient( &globalBackbuffer, xOffset, yOffset );

            DWORD writeCursor;
            DWORD playCursor;
            if ( SUCCEEDED( globalSecondaryBuffer->GetCurrentPosition( &playCursor, &writeCursor ) ) )
            {
                DWORD byteToLock = ( soundOutput.runningsampleIndex * soundOutput.bytesPerSample ) % soundOutput.secondaryBufferSize;
                DWORD targetCursor = ( playCursor + soundOutput.latencySampleCount * soundOutput.bytesPerSample ) % soundOutput.secondaryBufferSize;
                DWORD bytesToWrite;

                if ( byteToLock > targetCursor )
                {
                    bytesToWrite = soundOutput.secondaryBufferSize - byteToLock;
                    bytesToWrite += targetCursor;
                }
                else
                {
                    bytesToWrite = targetCursor - byteToLock;
                }
                Win32FillSoundBuffer( &soundOutput, byteToLock, bytesToWrite );
            }
            Win32_Window_Dimensions dimensions = Win32GetWindowDimensions( window );
            Win32DisplayBufferInWindow( deviceContext, dimensions.width, dimensions.height,
                                        &globalBackbuffer );
        }
    }
    return 0;
}
