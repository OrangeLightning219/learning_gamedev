#include "isometric_game.h"
#include "utils.h"
#include <math.h>
#include "isometric_game.cpp"

#include "win32_isometric_game.h"
#include <malloc.h>
#include <stdio.h>
#include <xinput.h>
#include <dsound.h>

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

internal Debug_Read_File_Result DebugPlatformReadEntireFile( char *filename )
{
    Debug_Read_File_Result result = {};
    HANDLE fileHandle = CreateFile( filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0 );
    if ( fileHandle != INVALID_HANDLE_VALUE )
    {
        LARGE_INTEGER fileSize;
        if ( GetFileSizeEx( fileHandle, &fileSize ) )
        {
            u32 fileSize32 = SafeTruncateU64( fileSize.QuadPart );
            result.content = VirtualAlloc( 0, fileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
            if ( result.content )
            {
                DWORD bytesRead;
                if ( ReadFile( fileHandle, result.content, fileSize32, &bytesRead, 0 ) && bytesRead == fileSize32 )
                {
                    result.contentSize = fileSize32;
                }
                else
                {
                    //@TODO: Logging
                    DebugPlatformFreeFileMemory( result.content );
                    result.content = 0;
                }
            }
            else
            {
                //@TODO: Logging
            }
        }
        else
        {
            //@TODO: Logging
        }
        CloseHandle( fileHandle );
    }
    else
    {
        //@TODO: Logging
    }
    return result;
}

internal void DebugPlatformFreeFileMemory( void *fileMemory )
{
    if ( fileMemory )
    {
        VirtualFree( fileMemory, 0, MEM_RELEASE );
    }
}

internal bool DebugPlatformWriteEntireFile( char *filename, u32 memorySize, void *memory )
{
    bool result = false;
    HANDLE fileHandle = CreateFile( filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0 );
    if ( fileHandle != INVALID_HANDLE_VALUE )
    {
        DWORD bytesWritten;
        if ( WriteFile( fileHandle, memory, memorySize, &bytesWritten, 0 ) )
        {
            result = bytesWritten == memorySize;
        }
        else
        {
            //@TODO: Logging
        }

        CloseHandle( fileHandle );
    }
    else
    {
        //@TODO: Logging
    }
    return result;
}

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
    buffer->memory = VirtualAlloc( 0, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
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

internal void Win32ClearSoundBuffer( Win32_Sound_Output *soundOutput )
{
    VOID *region1;
    DWORD region1Size;
    VOID *region2;
    DWORD region2Size;
    if ( SUCCEEDED( globalSecondaryBuffer->Lock( 0, soundOutput->secondaryBufferSize, &region1, &region1Size, &region2, &region2Size, 0 ) ) )
    {
        u8 *destByte = ( u8 * ) region1;
        for ( DWORD byteIndex = 0; byteIndex < region1Size; ++byteIndex )
        {
            *destByte++ = 0;
        }

        destByte = ( u8 * ) region2;
        for ( DWORD byteIndex = 0; byteIndex < region2Size; ++byteIndex )
        {
            *destByte++ = 0;
        }
        globalSecondaryBuffer->Unlock( region1, region1Size, region2, region2Size );
    }
}

internal void Win32FillSoundBuffer( Win32_Sound_Output *soundOutput, DWORD byteToLock, DWORD bytesToWrite,
                                    Game_Sound_Output_Buffer *sourceBuffer )
{
    VOID *region1;
    DWORD region1Size;
    VOID *region2;
    DWORD region2Size;

    if ( SUCCEEDED( globalSecondaryBuffer->Lock( byteToLock, bytesToWrite, &region1, &region1Size, &region2, &region2Size, 0 ) ) )
    {
        s16 *destSample = ( s16 * ) region1;
        s16 *sourceSample = sourceBuffer->samples;
        DWORD region1SampleCount = region1Size / soundOutput->bytesPerSample;
        for ( DWORD sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex )
        {
            *destSample++ = *sourceSample++;
            *destSample++ = *sourceSample++;
            ++soundOutput->runningsampleIndex;
        }

        destSample = ( s16 * ) region2;
        DWORD region2SampleCount = region2Size / soundOutput->bytesPerSample;
        for ( DWORD sampleIndex = 0; sampleIndex < region2SampleCount; ++sampleIndex )
        {
            *destSample++ = *sourceSample++;
            *destSample++ = *sourceSample++;
            ++soundOutput->runningsampleIndex;
        }
        globalSecondaryBuffer->Unlock( region1, region1Size, region2, region2Size );
    }
}

internal void Win32ProcessKeyboardMessage( Game_Button_State *newState, bool isDown )
{
    newState->endedDown = isDown;
    ++newState->halfTransitionCount;
}

internal void Win32ProcessXInputDigitalButton( DWORD XInputButtonState, Game_Button_State *oldState,
                                               DWORD buttonBit, Game_Button_State *newState )
{
    newState->halfTransitionCount = oldState->endedDown != newState->endedDown ? 1 : 0;
    newState->endedDown = ( XInputButtonState & buttonBit ) == buttonBit;
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
            Assert( !"Keyboard input came in throught a non dispatch message" );
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint( window, &paint );
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

internal void Win32ProcessPendingMessages( Game_Controller_Input *keyboardController )
{
    MSG message;
    while ( PeekMessage( &message, 0, 0, 0, PM_REMOVE ) )
    {
        switch ( message.message )
        {
            case WM_QUIT:
            {
                globalRunning = false;
            }
            break;
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                u32 vkCode = ( u32 ) message.wParam;
                bool wasDown = ( message.lParam & ( 1 << 30 ) ) != 0;
                bool isDown = ( message.lParam & ( 1 << 31 ) ) == 0;
                bool altWasDown = ( message.lParam & ( 1 << 29 ) ) != 0;
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
                        Win32ProcessKeyboardMessage( &keyboardController->a, isDown );
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
                        globalRunning = false;
                    }

                    if ( vkCode == VK_F4 && altWasDown )
                    {
                        globalRunning = false;
                    }
                }
            }
            break;
            default:
            {
                TranslateMessage( &message );
                DispatchMessage( &message );
            }
            break;
        }
    }
}

int WinMain( HINSTANCE instance,
             HINSTANCE prevInstance,
             LPSTR commamdLine,
             int showCode )
{
    LARGE_INTEGER performanceCounterFrequencyResult;
    QueryPerformanceFrequency( &performanceCounterFrequencyResult );
    s64 performanceCounterFrequency = performanceCounterFrequencyResult.QuadPart;

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
        Win32_Sound_Output soundOutput = {};
        soundOutput.samplesPerSecond = 48000;
        soundOutput.bytesPerSample = sizeof( s16 ) * 2;
        soundOutput.runningsampleIndex = 0;
        soundOutput.secondaryBufferSize = soundOutput.bytesPerSample * soundOutput.samplesPerSecond;
        soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;

        Win32InitDSound( window, soundOutput.samplesPerSecond, soundOutput.secondaryBufferSize );
        Win32ClearSoundBuffer( &soundOutput );
        globalSecondaryBuffer->Play( 0, 0, DSBPLAY_LOOPING );

        s16 *samples = ( s16 * ) VirtualAlloc( 0, soundOutput.secondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );

        Game_Memory gameMemory = {};
#if INTERNAL
        LPVOID baseAddress = ( LPVOID ) Terabytes( 2 );
#else
        LPVOID baseAddress = 0;
#endif
        gameMemory.permanentStorageSize = Megabytes( 64 );
        gameMemory.transientStorageSize = Gigabytes( 1 );
        u64 totalSize = gameMemory.permanentStorageSize + gameMemory.transientStorageSize;
        gameMemory.permanentStorage = VirtualAlloc( baseAddress, totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );

        gameMemory.transientStorage = ( u8 * ) gameMemory.permanentStorage + gameMemory.permanentStorageSize;
        if ( samples && gameMemory.permanentStorage && gameMemory.transientStorage )
        {
            LARGE_INTEGER lastCounter;
            QueryPerformanceCounter( &lastCounter );

            u64 lastCycleCount = __rdtsc();

            Game_Input input[ 2 ] = {};
            Game_Input *newInput = &input[ 0 ];
            Game_Input *oldInput = &input[ 1 ];
            while ( globalRunning )
            {
                Game_Controller_Input *keyboardController = &newInput->controllers[ 0 ];
                Game_Controller_Input zeroController = {};
                *keyboardController = zeroController;

                Win32ProcessPendingMessages( keyboardController );

                DWORD maxControllerCount = XUSER_MAX_COUNT;
                if ( maxControllerCount > ArrayCount( newInput->controllers ) )
                {
                    maxControllerCount = ArrayCount( newInput->controllers );
                }
                for ( DWORD controllerIndex = 0; controllerIndex < maxControllerCount; ++controllerIndex )
                {
                    Game_Controller_Input *oldController = &oldInput->controllers[ controllerIndex ];
                    Game_Controller_Input *newController = &newInput->controllers[ controllerIndex ];

                    XINPUT_STATE controllerState;
                    if ( XInputGetState( controllerIndex, &controllerState ) == ERROR_SUCCESS )
                    {
                        // controller available;
                        XINPUT_GAMEPAD *pad = &controllerState.Gamepad;
                        s16 rightStickX = pad->sThumbRX;
                        s16 rightStickY = pad->sThumbRY;

                        u8 leftTrigger = pad->bLeftTrigger;
                        u8 rightTrigger = pad->bRightTrigger;

                        newController->isAnalog = true;
                        float32 leftStickX = pad->sThumbLX < 0 ? ( float32 ) pad->sThumbLX / 32768.0f : ( float32 ) pad->sThumbLX / 32767.0f;
                        newController->leftStickStartX = oldController->leftStickEndX;
                        newController->minStickX = newController->maxStickX = newController->leftStickEndX = leftStickX;

                        float32 leftStickY = pad->sThumbLY < 0 ? ( float32 ) pad->sThumbLY / 32768.0f : ( float32 ) pad->sThumbLY / 32767.0f;
                        newController->leftStickStartY = oldController->leftStickEndY;
                        newController->minStickY = newController->maxStickY = newController->leftStickEndY = leftStickY;

                        Win32ProcessXInputDigitalButton( pad->wButtons, &oldController->down, XINPUT_GAMEPAD_DPAD_DOWN, &newController->down );
                        Win32ProcessXInputDigitalButton( pad->wButtons, &oldController->up, XINPUT_GAMEPAD_DPAD_UP, &newController->up );
                        Win32ProcessXInputDigitalButton( pad->wButtons, &oldController->left, XINPUT_GAMEPAD_DPAD_LEFT, &newController->left );
                        Win32ProcessXInputDigitalButton( pad->wButtons, &oldController->right, XINPUT_GAMEPAD_DPAD_RIGHT, &newController->right );
                        Win32ProcessXInputDigitalButton( pad->wButtons, &oldController->start, XINPUT_GAMEPAD_START, &newController->start );
                        Win32ProcessXInputDigitalButton( pad->wButtons, &oldController->back, XINPUT_GAMEPAD_BACK, &newController->back );
                        Win32ProcessXInputDigitalButton( pad->wButtons, &oldController->l3, XINPUT_GAMEPAD_LEFT_THUMB, &newController->l3 );
                        Win32ProcessXInputDigitalButton( pad->wButtons, &oldController->r3, XINPUT_GAMEPAD_RIGHT_THUMB, &newController->r3 );
                        Win32ProcessXInputDigitalButton( pad->wButtons, &oldController->l1, XINPUT_GAMEPAD_LEFT_SHOULDER, &newController->l1 );
                        Win32ProcessXInputDigitalButton( pad->wButtons, &oldController->r1, XINPUT_GAMEPAD_RIGHT_SHOULDER, &newController->r1 );
                        Win32ProcessXInputDigitalButton( pad->wButtons, &oldController->a, XINPUT_GAMEPAD_A, &newController->a );
                        Win32ProcessXInputDigitalButton( pad->wButtons, &oldController->b, XINPUT_GAMEPAD_B, &newController->b );
                        Win32ProcessXInputDigitalButton( pad->wButtons, &oldController->x, XINPUT_GAMEPAD_X, &newController->x );
                        Win32ProcessXInputDigitalButton( pad->wButtons, &oldController->y, XINPUT_GAMEPAD_Y, &newController->y );

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

                DWORD byteToLock = 0;
                DWORD bytesToWrite = 0;
                DWORD writeCursor = 0;
                DWORD playCursor = 0;
                bool soundIsValid = false;
                if ( SUCCEEDED( globalSecondaryBuffer->GetCurrentPosition( &playCursor, &writeCursor ) ) )
                {
                    byteToLock = ( soundOutput.runningsampleIndex * soundOutput.bytesPerSample ) % soundOutput.secondaryBufferSize;
                    DWORD targetCursor = ( playCursor + soundOutput.latencySampleCount * soundOutput.bytesPerSample ) % soundOutput.secondaryBufferSize;

                    if ( byteToLock > targetCursor )
                    {
                        bytesToWrite = soundOutput.secondaryBufferSize - byteToLock;
                        bytesToWrite += targetCursor;
                    }
                    else
                    {
                        bytesToWrite = targetCursor - byteToLock;
                    }
                    soundIsValid = true;
                }
                Game_Offscreen_Buffer buffer = {};
                buffer.memory = globalBackbuffer.memory;
                buffer.width = globalBackbuffer.width;
                buffer.height = globalBackbuffer.height;
                buffer.pitch = globalBackbuffer.pitch;

                Game_Sound_Output_Buffer soundBuffer = {};
                soundBuffer.samplesPerSecond = soundOutput.samplesPerSecond;
                soundBuffer.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
                soundBuffer.samples = samples;
                GameUpdateAndRender( &gameMemory, newInput, &buffer, &soundBuffer );

                if ( soundIsValid )
                {
                    Win32FillSoundBuffer( &soundOutput, byteToLock, bytesToWrite, &soundBuffer );
                }
                Win32_Window_Dimensions dimensions = Win32GetWindowDimensions( window );
                Win32DisplayBufferInWindow( deviceContext, dimensions.width, dimensions.height,
                                            &globalBackbuffer );

                LARGE_INTEGER endCounter;
                QueryPerformanceCounter( &endCounter );

                u64 endCycleCount = __rdtsc();

                // u64 cyclesElapsed = endCycleCount - lastCycleCount;
                // s64 counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;
                // float32 msPerFrame = ( float32 ) ( ( 1000.0f * ( float32 ) counterElapsed ) / ( float32 ) performanceCounterFrequency );
                // float32 fps = 1000.0f / msPerFrame;
                // float32 megaCyclesPerFrame = ( float32 ) ( ( float32 ) cyclesElapsed / 1000000.0f );

                lastCounter = endCounter;
                lastCycleCount = endCycleCount;

                Game_Input *temp = newInput;
                newInput = oldInput;
                oldInput = temp;
            }
        }
        else
        {
            //@TODO: Logging
        }
    }
    else
    {
        //@TODO: Logging
    }
    return 0;
}
