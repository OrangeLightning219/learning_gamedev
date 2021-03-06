#include "utils.h"
#include "win32_learning_gamedev.h"
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
global_variable bool globalPause;
global_variable Win32_Offscreen_Buffer globalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER globalSecondaryBuffer;

global_variable s64 globalPerformanceCounterFrequency;

DEBUG_PLATFORM_FREE_FILE_MEMORY( DebugPlatformFreeFileMemory )
{
    if ( fileMemory )
    {
        VirtualFree( fileMemory, 0, MEM_RELEASE );
    }
}

DEBUG_PLATFORM_READ_ENTIRE_FILE( DebugPlatformReadEntireFile )
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
                    DebugPlatformFreeFileMemory( thread, result.content );
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

DEBUG_PLATFORM_WRITE_ENTIRE_FILE( DebugPlatformWriteEntireFile )
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

internal void Win32GetExeFilename( Win32_State *state )
{
    GetModuleFileName( 0, state->exePath, sizeof( state->exePath ) );
    state->exeFilename = state->exePath;
    for ( char *scan = state->exePath; *scan; ++scan )
    {
        if ( *scan == '\\' )
        {
            state->exeFilename = scan + 1;
        }
    }
}

internal void Win32BuildExePathFilename( Win32_State *state, char *filename, char *destination, int destinationCount )
{
    ConcatenateStrings( state->exePath, ( int ) ( state->exeFilename - state->exePath ),
                        filename, StringLength( filename ),
                        destination );
}

inline FILETIME Win32GetLastWriteTime( char *filename )
{
    WIN32_FILE_ATTRIBUTE_DATA data;
    if ( GetFileAttributesEx( filename, GetFileExInfoStandard, &data ) )
    {
        return data.ftLastWriteTime;
    }
    return {};
}

internal Win32_Game_Code Win32LoadGameCode( char *sourceDLLName, char *tempDllName )
{
    CopyFile( sourceDLLName, tempDllName, FALSE );
    Win32_Game_Code gameCode = {};
    gameCode.dllLastWriteTime = Win32GetLastWriteTime( sourceDLLName );
    gameCode.gameCodeDLL = LoadLibrary( tempDllName );
    if ( gameCode.gameCodeDLL )
    {
        gameCode.GetSoundSamples = ( game_get_sound_samples * ) GetProcAddress( gameCode.gameCodeDLL, "GameGetSoundSamples" );
        gameCode.UpdateAndRender = ( game_update_and_render * ) GetProcAddress( gameCode.gameCodeDLL, "GameUpdateAndRender" );
        gameCode.isValid = gameCode.GetSoundSamples && gameCode.UpdateAndRender;
    }
    if ( !gameCode.isValid )
    {
        gameCode.GetSoundSamples = 0;
        gameCode.UpdateAndRender = 0;
    }
    return gameCode;
}

internal void Win32UnloadGameCode( Win32_Game_Code gameCode )
{
    if ( gameCode.gameCodeDLL )
    {
        FreeLibrary( gameCode.gameCodeDLL );
        gameCode.gameCodeDLL = 0;
    }
    gameCode.isValid = false;
    gameCode.GetSoundSamples = 0;
    gameCode.UpdateAndRender = 0;
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
    buffer->bytesPerPixel = bytesPerPixel;
}

internal void Win32DisplayBufferInWindow( HDC deviceContext, int windowWidth, int windowHeight,
                                          Win32_Offscreen_Buffer *buffer )
{
    int offsetX = 10;
    int offsetY = 10;
    PatBlt( deviceContext, 0, 0, windowWidth, offsetY, BLACKNESS );
    PatBlt( deviceContext, 0, 0, offsetX, windowHeight, BLACKNESS );
    PatBlt( deviceContext, offsetX + buffer->width, 0, windowWidth - buffer->width - offsetX, windowHeight, BLACKNESS );
    PatBlt( deviceContext, 0, offsetY + buffer->height, windowWidth, windowHeight - buffer->height - offsetY, BLACKNESS );
    StretchDIBits( deviceContext,
                   10, 10, buffer->width, buffer->height,
                   0, 0, buffer->width, buffer->height,
                   buffer->memory, &buffer->info,
                   DIB_RGB_COLORS, SRCCOPY );
}

internal void Win32DebugDrawVertical( Win32_Offscreen_Buffer *backbuffer, int x, int top, int bottom, u32 color )
{
    if ( top < 0 )
    {
        top = 0;
    }

    if ( bottom >= backbuffer->height )
    {
        bottom = backbuffer->height;
    }
    if ( x >= 0 && x < backbuffer->width )
    {
        u8 *pixel = ( u8 * ) backbuffer->memory + x * backbuffer->bytesPerPixel + top * backbuffer->pitch;
        for ( int y = top; y < bottom; ++y )
        {
            *( u32 * ) pixel = color;
            pixel += backbuffer->pitch;
        }
    }
}

inline void Win32DrawSoundBufferMarker( Win32_Offscreen_Buffer *backbuffer, float c, int paddingX,
                                        int top, int bottom, DWORD value, u32 color )
{
    int x = paddingX + ( int ) ( c * ( float32 ) value );
    Win32DebugDrawVertical( backbuffer, x, top, bottom, color );
}
internal void Win32DebugSyncDisplay( Win32_Offscreen_Buffer *backbuffer, int markerCount, Win32_Debug_Sound_Marker *markers,
                                     int currentMarkerIndex, Win32_Sound_Output *soundOutput )
{
    int paddingX = 16;
    int paddingY = 16;

    int lineHeight = 64;

    float32 c = ( float32 ) ( backbuffer->width - 2 * paddingX ) / ( float32 ) soundOutput->secondaryBufferSize;
    for ( int markerIndex = 0; markerIndex < markerCount; ++markerIndex )
    {
        DWORD playColor = 0x000000FF;
        DWORD writeColor = 0x0000FF00;
        int top = paddingY;
        int bottom = paddingY + lineHeight;
        Win32_Debug_Sound_Marker *thisMarker = &markers[ markerIndex ];
        if ( markerIndex == currentMarkerIndex )
        {
            int firstTop = top;
            top += paddingY + lineHeight;
            bottom += paddingY + lineHeight;
            Win32DrawSoundBufferMarker( backbuffer, c, paddingX, top, bottom, thisMarker->outputPlayCursor, playColor );
            Win32DrawSoundBufferMarker( backbuffer, c, paddingX, top, bottom, thisMarker->outputWriteCursor, writeColor );
            top += paddingY + lineHeight;
            bottom += paddingY + lineHeight;
            Win32DrawSoundBufferMarker( backbuffer, c, paddingX, top, bottom, thisMarker->outputLocation, playColor );
            Win32DrawSoundBufferMarker( backbuffer, c, paddingX, top, bottom, thisMarker->outputLocation + thisMarker->outputByteCount, writeColor );
            top += paddingY + lineHeight;
            bottom += paddingY + lineHeight;
            Win32DrawSoundBufferMarker( backbuffer, c, paddingX, firstTop, bottom, thisMarker->expectedFlipPlayCursor, 0x00FFFFFF );
        }
        Win32DrawSoundBufferMarker( backbuffer, c, paddingX, top, bottom, thisMarker->flipPlayCursor, playColor );
        Win32DrawSoundBufferMarker( backbuffer, c, paddingX, top, bottom, thisMarker->flipWriteCursor, writeColor );
    }
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
    if ( newState->endedDown != isDown )
    {
        newState->endedDown = isDown;
        ++newState->halfTransitionCount;
    }
}

internal void Win32ProcessXInputDigitalButton( DWORD XInputButtonState, Game_Button_State *oldState,
                                               DWORD buttonBit, Game_Button_State *newState )
{
    newState->halfTransitionCount = oldState->endedDown != newState->endedDown ? 1 : 0;
    newState->endedDown = ( XInputButtonState & buttonBit ) == buttonBit;
}

internal float32 Win32ProcessXInputStickValue( SHORT stickValue, SHORT deadzone )
{
    if ( stickValue > deadzone )
    {
        return ( float32 ) stickValue / 32767.0f;
    }
    else if ( stickValue < -deadzone )
    {
        return ( float32 ) stickValue / 32768.0f;
    }
    return 0.0f;
}

internal float32 Win32ProcessXInputTriggerValue( BYTE triggerValue, BYTE deadzone )
{
    return triggerValue > deadzone ? ( float32 ) triggerValue / 255.0f : 0.0f;
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

internal void Win32GetInputRecordingFileLocation( Win32_State *state, bool inputStream, int slotIndex, char *destination, int destinationCount )
{
    char temp[ 64 ];
    wsprintf( temp, "recording_%d_%s.input", slotIndex, inputStream ? "input" : "state" );
    Win32BuildExePathFilename( state, temp, destination, destinationCount );
}

internal Win32_Replay_Buffer *Win32GetReplayBuffer( Win32_State *state, int inputRecordingIndex )
{
    Assert( inputRecordingIndex < ArrayCount( state->replayBuffers ) );
    return &state->replayBuffers[ inputRecordingIndex ];
}
internal void Win32BeginRecordingInput( Win32_State *state, int inputRecordingIndex )
{
    Win32_Replay_Buffer *replayBuffer = Win32GetReplayBuffer( state, inputRecordingIndex );
    if ( !replayBuffer->memoryBlock )
    {
        return;
    }
    state->inputRecordingIndex = inputRecordingIndex;
    char filename[ MAX_PATH ];
    Win32GetInputRecordingFileLocation( state, true, inputRecordingIndex, filename, sizeof( filename ) );
    state->recordingHandle = CreateFile( filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0 );
    CopyMemory( replayBuffer->memoryBlock, state->gameMemoryBlock, state->totalSize );
}

internal void Win32EndRecordingInput( Win32_State *state )
{
    CloseHandle( state->recordingHandle );
    state->inputRecordingIndex = -1;
}

internal void Win32BeginInputPlayback( Win32_State *state, int inputPlayingIndex )
{
    Win32_Replay_Buffer *replayBuffer = Win32GetReplayBuffer( state, inputPlayingIndex );
    if ( !replayBuffer->memoryBlock )
    {
        return;
    }
    state->inputPlayingIndex = inputPlayingIndex;
    char filename[ MAX_PATH ];
    Win32GetInputRecordingFileLocation( state, true, inputPlayingIndex, filename, sizeof( filename ) );
    state->playbackHandle = CreateFile( filename, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0 );
    CopyMemory( state->gameMemoryBlock, replayBuffer->memoryBlock, state->totalSize );
}

internal void Win32EndInputPlayback( Win32_State *state )
{
    CloseHandle( state->playbackHandle );
    state->inputPlayingIndex = -1;
}

internal void Win32RecordInput( Win32_State *state, Game_Input *input )
{
    DWORD bytesWritten;
    WriteFile( state->recordingHandle, input, sizeof( *input ), &bytesWritten, 0 );
}

internal void Win32PlaybackInput( Win32_State *state, Game_Input *input )
{
    DWORD bytesRead;
    if ( ReadFile( state->playbackHandle, input, sizeof( *input ), &bytesRead, 0 ) )
    {
        if ( bytesRead == 0 )
        {
            int playingIndex = state->inputPlayingIndex;
            Win32EndInputPlayback( state );
            Win32BeginInputPlayback( state, playingIndex );
            ReadFile( state->playbackHandle, input, sizeof( *input ), &bytesRead, 0 );
        }
    }
}

internal void Win32ProcessPendingMessages( Win32_State *state, Game_Controller_Input *keyboardController )
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
                        Win32ProcessKeyboardMessage( &keyboardController->up, isDown );
                    }
                    else if ( vkCode == 'A' )
                    {
                        Win32ProcessKeyboardMessage( &keyboardController->left, isDown );
                    }
                    else if ( vkCode == 'S' )
                    {
                        Win32ProcessKeyboardMessage( &keyboardController->down, isDown );
                    }
                    else if ( vkCode == 'D' )
                    {
                        Win32ProcessKeyboardMessage( &keyboardController->right, isDown );
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
#ifdef INTERNAL
                    if ( vkCode == 'P' && isDown )
                    {
                        globalPause = !globalPause;
                    }
                    else if ( vkCode == 'L' && isDown )
                    {
                        if ( state->inputPlayingIndex == -1 )
                        {
                            if ( state->inputRecordingIndex == -1 )
                            {
                                Win32BeginRecordingInput( state, 0 );
                            }
                            else
                            {
                                Win32EndRecordingInput( state );
                                Win32BeginInputPlayback( state, 0 );
                            }
                        }
                        else
                        {
                            Win32EndInputPlayback( state );
                        }
                    }
#endif
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

inline LARGE_INTEGER Win32GetWallClock()
{
    LARGE_INTEGER result;
    QueryPerformanceCounter( &result );
    return result;
}

inline float32 Win32GetSecondsElapsed( LARGE_INTEGER start, LARGE_INTEGER end )
{
    return ( float32 ) ( end.QuadPart - start.QuadPart ) / ( float32 ) globalPerformanceCounterFrequency;
}

int WinMain( HINSTANCE instance,
             HINSTANCE prevInstance,
             LPSTR commamdLine,
             int showCode )
{
    Win32_State state = {};
    Win32GetExeFilename( &state );
    char sourceGameCodeDLLFullPath[ MAX_PATH ];
    Win32BuildExePathFilename( &state, "learning_gamedev.dll", sourceGameCodeDLLFullPath, sizeof( sourceGameCodeDLLFullPath ) );
    char tempGameCodeDLLFullPath[ MAX_PATH ];
    Win32BuildExePathFilename( &state, "learning_gamedev_temp.dll", tempGameCodeDLLFullPath, sizeof( tempGameCodeDLLFullPath ) );

    LARGE_INTEGER globalPerformanceCounterFrequencyResult;
    QueryPerformanceFrequency( &globalPerformanceCounterFrequencyResult );
    globalPerformanceCounterFrequency = globalPerformanceCounterFrequencyResult.QuadPart;

    UINT desiredSchedulerMs = 1; // Windows scheduler granularity
    bool sleepIsGranular = timeBeginPeriod( desiredSchedulerMs ) == TIMERR_NOERROR;

    Win32LoadXInput();
    WNDCLASS windowClass = {};

    Win32ResizeDIBSection( &globalBackbuffer, 1280, 720 );

    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = Win32MainWindowCallback;
    windowClass.hInstance = instance;
    // windowClass.hIcon = ;
    windowClass.lpszClassName = "IsometricGameWindowClass";

    RegisterClass( &windowClass );

    //WS_EX_LAYERED - make transparent window
    HWND window = CreateWindowEx( 0, windowClass.lpszClassName, "Isometric Game", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0 );
    // HWND window = CreateWindowEx( 0, windowClass.lpszClassName, "Isometric Game", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
    //                               450, 250, 1350, 800, 0, 0, instance, 0 );
    if ( window )
    {
        // transparent window
        // SetLayeredWindowAttributes( window, RGB( 0, 0, 0 ), 128, LWA_ALPHA );
        globalRunning = true;

        int monitorRehreshHz = 60;
        HDC refreshContext = GetDC( window );
        int refreshRate = GetDeviceCaps( refreshContext, VREFRESH );
        ReleaseDC( window, refreshContext );
        if ( refreshRate > 1 )
        {
            monitorRehreshHz = refreshRate;
        }

        float32 gameRefreshHz = ( float32 ) monitorRehreshHz; // change this in case the software renderer can't hit 60 fps
        float32 targetSecondsPerFrame = 1.0f / ( float32 ) gameRefreshHz;

        Win32_Sound_Output soundOutput = {};
        soundOutput.samplesPerSecond = 48000;
        soundOutput.bytesPerSample = sizeof( s16 ) * 2;
        soundOutput.runningsampleIndex = 0;
        soundOutput.secondaryBufferSize = soundOutput.bytesPerSample * soundOutput.samplesPerSecond;
        int bytesPerSecond = soundOutput.bytesPerSample * soundOutput.samplesPerSecond;
        soundOutput.safetyBytes = ( int ) ( ( ( float32 ) bytesPerSecond / gameRefreshHz ) / 2.0f );

        Win32InitDSound( window, soundOutput.samplesPerSecond, soundOutput.secondaryBufferSize );
        Win32ClearSoundBuffer( &soundOutput );
        globalSecondaryBuffer->Play( 0, 0, DSBPLAY_LOOPING );

        s16 *samples = ( s16 * ) VirtualAlloc( 0, soundOutput.secondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );

        Game_Memory gameMemory = {};
#ifdef INTERNAL
        LPVOID baseAddress = ( LPVOID ) Terabytes( 2 );
#else
        LPVOID baseAddress = 0;
#endif
        gameMemory.permanentStorageSize = Megabytes( 64 );
        gameMemory.transientStorageSize = Megabytes( 256 );
        state.totalSize = gameMemory.permanentStorageSize + gameMemory.transientStorageSize;
        state.gameMemoryBlock = VirtualAlloc( baseAddress, state.totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
        gameMemory.permanentStorage = state.gameMemoryBlock;
        gameMemory.transientStorage = ( u8 * ) gameMemory.permanentStorage + gameMemory.permanentStorageSize;
#ifdef INTERNAL
        gameMemory.DebugPlatformFreeFileMemory = DebugPlatformFreeFileMemory;
        gameMemory.DebugPlatformReadEntireFile = DebugPlatformReadEntireFile;
        gameMemory.DebugPlatformWriteEntireFile = DebugPlatformWriteEntireFile;
#endif

        state.inputPlayingIndex = -1;
        state.inputRecordingIndex = -1;
        for ( int replayIndex = 0; replayIndex < ArrayCount( state.replayBuffers ); ++replayIndex )
        {
            Win32_Replay_Buffer *replayBuffer = &state.replayBuffers[ replayIndex ];
            Win32GetInputRecordingFileLocation( &state, false, replayIndex, replayBuffer->filename, sizeof( replayBuffer->filename ) );
            replayBuffer->fileHandle = CreateFile( replayBuffer->filename, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0 );
            DWORD maxSizeHigh = state.totalSize >> 32;
            DWORD maxSizeLow = state.totalSize & 0xFFFFFFFF;
            replayBuffer->memoryMap = CreateFileMapping( replayBuffer->fileHandle, 0, PAGE_READWRITE,
                                                         maxSizeHigh, maxSizeLow, 0 );
            replayBuffer->memoryBlock = MapViewOfFile( replayBuffer->memoryMap, FILE_MAP_ALL_ACCESS, 0, 0, state.totalSize );
            Assert( replayBuffer->memoryBlock );
        }

        if ( samples && gameMemory.permanentStorage && gameMemory.transientStorage )
        {
            int debugTimeMarkerIndex = 0;
            Win32_Debug_Sound_Marker debugTimeMarkers[ 30 ] = { { 0, 0, 0, 0, 0, 0, 0 } };

            bool soundIsValid = false;

            LARGE_INTEGER lastCounter = Win32GetWallClock();
            LARGE_INTEGER flipWallClock = Win32GetWallClock();
            u64 lastCycleCount = __rdtsc();

            Game_Input input[ 2 ] = {};
            Game_Input *newInput = &input[ 0 ];
            Game_Input *oldInput = &input[ 1 ];

            Win32_Game_Code game = Win32LoadGameCode( sourceGameCodeDLLFullPath, tempGameCodeDLLFullPath );
            while ( globalRunning )
            {
                newInput->dtForUpdate = targetSecondsPerFrame;
#ifdef INTERNAL
                FILETIME newDLLWtiteTime = Win32GetLastWriteTime( sourceGameCodeDLLFullPath );
                if ( CompareFileTime( &newDLLWtiteTime, &game.dllLastWriteTime ) != 0 )
                {
                    Win32UnloadGameCode( game );
                    game = Win32LoadGameCode( sourceGameCodeDLLFullPath, tempGameCodeDLLFullPath );
                }
#endif
                Game_Controller_Input *oldKeyboardController = getController( oldInput, 0 );
                Game_Controller_Input *newKeyboardController = getController( newInput, 0 );
                *newKeyboardController = {};
                newKeyboardController->isConnected = true;

                for ( int buttonIndex = 0; buttonIndex < ArrayCount( newKeyboardController->buttons ); ++buttonIndex )
                {
                    newKeyboardController->buttons[ buttonIndex ].endedDown = oldKeyboardController->buttons[ buttonIndex ].endedDown;
                }

                Win32ProcessPendingMessages( &state, newKeyboardController );

                if ( globalPause )
                {
                    continue;
                }

                POINT mouse;
                GetCursorPos( &mouse );
                ScreenToClient( window, &mouse );
                newInput->mouseX = mouse.x;
                newInput->mouseY = mouse.y;
                newInput->mouseZ = 0;
                Win32ProcessKeyboardMessage( &newInput->mouseButtons[ 0 ], GetKeyState( VK_LBUTTON ) & ( 1 << 15 ) );
                Win32ProcessKeyboardMessage( &newInput->mouseButtons[ 1 ], GetKeyState( VK_RBUTTON ) & ( 1 << 15 ) );
                Win32ProcessKeyboardMessage( &newInput->mouseButtons[ 2 ], GetKeyState( VK_MBUTTON ) & ( 1 << 15 ) );
                Win32ProcessKeyboardMessage( &newInput->mouseButtons[ 3 ], GetKeyState( VK_XBUTTON1 ) & ( 1 << 15 ) );
                Win32ProcessKeyboardMessage( &newInput->mouseButtons[ 4 ], GetKeyState( VK_XBUTTON2 ) & ( 1 << 15 ) );

                DWORD maxControllerCount = XUSER_MAX_COUNT;
                if ( maxControllerCount > ArrayCount( newInput->controllers ) - 1 )
                {
                    maxControllerCount = ArrayCount( newInput->controllers ) - 1;
                }
                for ( DWORD controllerIndex = 0; controllerIndex < maxControllerCount; ++controllerIndex )
                {
                    DWORD ourControllerIndex = controllerIndex + 1;
                    Game_Controller_Input *oldController = getController( oldInput, ourControllerIndex );
                    Game_Controller_Input *newController = getController( newInput, ourControllerIndex );

                    XINPUT_STATE controllerState;
                    if ( XInputGetState( controllerIndex, &controllerState ) == ERROR_SUCCESS )
                    {
                        newController->isConnected = true;
                        newController->isAnalog = oldController->isAnalog;
                        XINPUT_GAMEPAD *pad = &controllerState.Gamepad;

                        newController->leftTriggerAverage = Win32ProcessXInputTriggerValue( pad->bLeftTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD );
                        newController->rightTriggerAverage = Win32ProcessXInputTriggerValue( pad->bRightTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD );

                        newController->leftStickAverageX = Win32ProcessXInputStickValue( pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE );
                        newController->leftStickAverageY = Win32ProcessXInputStickValue( pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE );
                        newController->rightStickAverageX = Win32ProcessXInputStickValue( pad->sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE );
                        newController->rightStickAverageY = Win32ProcessXInputStickValue( pad->sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE );

                        if ( newController->leftStickAverageY != 0.0f || newController->leftStickAverageX != 0.0f ||
                             newController->rightStickAverageY != 0.0f || newController->rightStickAverageX != 0.0f ||
                             newController->leftTriggerAverage != 0.0f || newController->rightTriggerAverage != 0.0f )
                        {
                            newController->isAnalog = true;
                        }
                        if ( pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT )
                        {
                            newController->leftStickAverageX = -1;
                            newController->isAnalog = false;
                        }
                        if ( pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT )
                        {
                            newController->leftStickAverageX = 1;
                            newController->isAnalog = false;
                        }
                        if ( pad->wButtons & XINPUT_GAMEPAD_DPAD_UP )
                        {
                            newController->leftStickAverageY = 1;
                            newController->isAnalog = false;
                        }
                        if ( pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN )
                        {
                            newController->leftStickAverageY = -1;
                            newController->isAnalog = false;
                        }

                        float32 threshold = 0.5f;
                        Win32ProcessXInputDigitalButton( newController->leftStickAverageX < -threshold ? 1 : 0,
                                                         &oldController->left, 1, &newController->left );
                        Win32ProcessXInputDigitalButton( newController->leftStickAverageX > threshold ? 1 : 0,
                                                         &oldController->right, 1, &newController->right );
                        Win32ProcessXInputDigitalButton( newController->leftStickAverageY < -threshold ? 1 : 0,
                                                         &oldController->down, 1, &newController->down );
                        Win32ProcessXInputDigitalButton( newController->leftStickAverageY > threshold ? 1 : 0,
                                                         &oldController->up, 1, &newController->up );

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
                        newController->isConnected = false;
                    }
                }

                Thread_Context thread;
                Game_Offscreen_Buffer buffer = {};
                buffer.memory = globalBackbuffer.memory;
                buffer.width = globalBackbuffer.width;
                buffer.height = globalBackbuffer.height;
                buffer.pitch = globalBackbuffer.pitch;
                buffer.bytesPerPixel = globalBackbuffer.bytesPerPixel;

                if ( state.inputRecordingIndex > -1 )
                {
                    Win32RecordInput( &state, newInput );
                }

                if ( state.inputPlayingIndex > -1 )
                {
                    Win32PlaybackInput( &state, newInput );
                }

                if ( game.UpdateAndRender )
                {
                    game.UpdateAndRender( &thread, &gameMemory, newInput, &buffer );
                }

                LARGE_INTEGER audioWallClock = Win32GetWallClock();
                float32 fromBeginToAudioSeconds = Win32GetSecondsElapsed( flipWallClock, audioWallClock );

                DWORD playCursor;
                DWORD writeCursor;
                if ( SUCCEEDED( globalSecondaryBuffer->GetCurrentPosition( &playCursor, &writeCursor ) ) )
                {
                    if ( !soundIsValid )
                    {
                        soundOutput.runningsampleIndex = writeCursor / soundOutput.bytesPerSample;
                        soundIsValid = true;
                    }
                    DWORD byteToLock = ( soundOutput.runningsampleIndex * soundOutput.bytesPerSample ) % soundOutput.secondaryBufferSize;

                    DWORD expectedSoundBytesPerFrame = ( int ) ( ( float32 ) ( soundOutput.samplesPerSecond * soundOutput.bytesPerSample ) / gameRefreshHz );
                    float32 secondsLeftUntilFlip = targetSecondsPerFrame - fromBeginToAudioSeconds;
                    DWORD expectedBytesUnitlFlip = ( DWORD ) ( ( float32 ) expectedSoundBytesPerFrame *
                                                               ( secondsLeftUntilFlip / targetSecondsPerFrame ) );
                    DWORD expectedFrameBoundryByte = playCursor + expectedBytesUnitlFlip;
                    DWORD safeWriteCursor = writeCursor;
                    if ( safeWriteCursor < playCursor )
                    {
                        safeWriteCursor += soundOutput.secondaryBufferSize;
                    }
                    Assert( safeWriteCursor >= playCursor );
                    safeWriteCursor += soundOutput.safetyBytes;
                    bool audioCardIsLowLatency = safeWriteCursor < expectedFrameBoundryByte;

                    DWORD targetCursor = 0;
                    if ( audioCardIsLowLatency )
                    {
                        targetCursor = ( expectedFrameBoundryByte + expectedSoundBytesPerFrame );
                    }
                    else
                    {
                        targetCursor = ( writeCursor + expectedSoundBytesPerFrame + soundOutput.safetyBytes );
                    }
                    targetCursor = targetCursor % soundOutput.secondaryBufferSize;

                    DWORD bytesToWrite = 0;
                    if ( byteToLock > targetCursor )
                    {
                        bytesToWrite = soundOutput.secondaryBufferSize - byteToLock;
                        bytesToWrite += targetCursor;
                    }
                    else
                    {
                        bytesToWrite = targetCursor - byteToLock;
                    }

                    Game_Sound_Output_Buffer soundBuffer = {};
                    soundBuffer.samplesPerSecond = soundOutput.samplesPerSecond;
                    soundBuffer.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
                    soundBuffer.samples = samples;
                    if ( game.GetSoundSamples )
                    {
                        game.GetSoundSamples( &thread, &gameMemory, &soundBuffer );
                    }
#ifdef INTERNAL
                    Win32_Debug_Sound_Marker *marker = &debugTimeMarkers[ debugTimeMarkerIndex ];
                    marker->outputPlayCursor = playCursor;
                    marker->outputWriteCursor = writeCursor;
                    marker->outputLocation = byteToLock;
                    marker->outputByteCount = bytesToWrite;
                    marker->expectedFlipPlayCursor = expectedFrameBoundryByte;
#endif
                    Win32FillSoundBuffer( &soundOutput, byteToLock, bytesToWrite, &soundBuffer );
                }
                else
                {
                    soundIsValid = false;
                }

                LARGE_INTEGER workCounter = Win32GetWallClock();
                float32 workSecondsElapsed = Win32GetSecondsElapsed( lastCounter, workCounter );

                float32 secondsElapsedForFrame = workSecondsElapsed;
                if ( secondsElapsedForFrame < targetSecondsPerFrame )
                {
                    if ( sleepIsGranular )
                    {
                        DWORD sleepMs = ( DWORD ) ( 1000.0f * ( targetSecondsPerFrame - secondsElapsedForFrame ) ) - 1;
                        if ( sleepMs > 0 )
                        {
                            Sleep( sleepMs );
                        }
                    }
                    float32 testSecondsElapsed = Win32GetSecondsElapsed( lastCounter, Win32GetWallClock() );
                    if ( testSecondsElapsed < targetSecondsPerFrame )
                    {
                        //@TODO: Log missed sleep
                    }
                    while ( secondsElapsedForFrame < targetSecondsPerFrame )
                    {
                        secondsElapsedForFrame = Win32GetSecondsElapsed( lastCounter, Win32GetWallClock() );
                    }
                }
                else
                {
                    //@TODO: Missed frame rate
                }

                LARGE_INTEGER endCounter = Win32GetWallClock();
                lastCounter = endCounter;

#ifdef INTERNAL
                // Win32DebugSyncDisplay( &globalBackbuffer, ArrayCount( debugTimeMarkers ),
                // debugTimeMarkers, debugTimeMarkerIndex - 1, &soundOutput );
#endif
                Win32_Window_Dimensions dimensions = Win32GetWindowDimensions( window );
                HDC deviceContext = GetDC( window );
                Win32DisplayBufferInWindow( deviceContext, dimensions.width, dimensions.height,
                                            &globalBackbuffer );
                ReleaseDC( window, deviceContext );

                flipWallClock = Win32GetWallClock();
#ifdef INTERNAL
                {
                    DWORD debugPlayCursor;
                    DWORD debugWriteCursor;
                    if ( SUCCEEDED( globalSecondaryBuffer->GetCurrentPosition( &debugPlayCursor, &debugWriteCursor ) ) )
                    {
                        Assert( debugTimeMarkerIndex < ArrayCount( debugTimeMarkers ) );
                        Win32_Debug_Sound_Marker *marker = &debugTimeMarkers[ debugTimeMarkerIndex ];
                        marker->flipPlayCursor = debugPlayCursor;
                        marker->flipWriteCursor = debugWriteCursor;
                        // globalSecondaryBuffer->GetCurrentPosition( &marker->playCursor, &marker->writeCursor );
                    }
                    ++debugTimeMarkerIndex;
                    if ( debugTimeMarkerIndex >= ArrayCount( debugTimeMarkers ) )
                    {
                        debugTimeMarkerIndex = 0;
                    }
                }
#endif
                // float32 fps = 1.0f / secondsElapsedForFrame;
                // char tempBuffer[ 256 ];
                // sprintf( tempBuffer, "FPS: %f\n", fps );
                // OutputDebugString( tempBuffer );
                // float32 megaCyclesPerFrame = ( float32 ) ( ( float32 ) cyclesElapsed / 1000000.0f );

                Game_Input *temp = newInput;
                newInput = oldInput;
                oldInput = temp;

                u64 endCycleCount = __rdtsc();
                // u64 cyclesElapsed = endCycleCount - lastCycleCount;
                lastCycleCount = endCycleCount;
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
