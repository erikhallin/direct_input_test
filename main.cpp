#include <iostream>
#include <windows.h>
#include <gl/gl.h>
#include <D3D/dinput.h>
#include <fcntl.h> //for console
#include <stdio.h>
#include <vector>

//Libraries required: dinput8.lib dxguid.lib (dinput.lib)

using namespace std;

LPDIRECTINPUT8 di;//direct input device
LPDIRECTINPUTDEVICE8 joystick;//joystick device
vector<LPDIRECTINPUTDEVICE8> vec_joys;
vector<long> vec_last_x;
vector<long> vec_last_y;
int joy_counter=0;
//long last_x=0;
//long last_y=0;

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void EnableOpenGL(HWND hwnd, HDC*, HGLRC*);
void DisableOpenGL(HWND, HDC, HGLRC);

//Direct Input Callbacks
BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* instance, VOID* context);
BOOL CALLBACK enumAxesCallback(const DIDEVICEOBJECTINSTANCE* instance, VOID* context);
HRESULT poll(DIJOYSTATE2 *js,int joystick_index);


int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    WNDCLASSEX wcex;
    HWND hwnd;
    HDC hDC;
    HGLRC hRC;
    MSG msg;
    BOOL bQuit = FALSE;
    float theta = 0.0f;

    /* register window class */
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_OWNDC;
    wcex.lpfnWndProc = WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "GLSample";
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);;


    if (!RegisterClassEx(&wcex))
        return 0;

    /* create main window */
    hwnd = CreateWindowEx(0,
                          "GLSample",
                          "OpenGL Sample",
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          256,
                          256,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);

    ShowWindow(hwnd, nCmdShow);


    //Open a console window
    AllocConsole();
    //Connect console output
    HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
    int hCrt          = _open_osfhandle((long) handle_out, _O_TEXT);
    FILE* hf_out      = _fdopen(hCrt, "w");
    setvbuf(hf_out, NULL, _IONBF, 1);
    *stdout = *hf_out;
    //Connect console input
    HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE);
    hCrt             = _open_osfhandle((long) handle_in, _O_TEXT);
    FILE* hf_in      = _fdopen(hCrt, "r");
    setvbuf(hf_in, NULL, _IONBF, 128);
    *stdin = *hf_in;
    //Set console title
    SetConsoleTitle("Debug Console");
    //HWND hwnd_console=GetConsoleWindow();
    //MoveWindow(hwnd_console,g_window_width,0,680,510,TRUE);
    cout<<"Software Started\n";


    /* enable OpenGL for the window */
    EnableOpenGL(hwnd, &hDC, &hRC);

//--------------------------------------------

    //init DI
    //LPDIRECTINPUT8 di;
    HRESULT hr;

    // Create a DirectInput device
    hr=DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION,
                          IID_IDirectInput8, (VOID**)&di, NULL);
    if(FAILED(hr))
    {
        cout<<"ERROR: DirectInput8Create\n";
        system("PAUSE");
        return hr;
    }
    cout<<"Direct Input Device created\n";


    //iterate through all of the available input devices on the system
    //LPDIRECTINPUTDEVICE8 joystick;
    // Look for the first simple joystick we can find.
    cout<<"Joystick detection\n";
    hr = di->EnumDevices(DI8DEVCLASS_GAMECTRL, enumCallback,
                         NULL, DIEDFL_ATTACHEDONLY);
    if(FAILED(hr))
    {
        cout<<"ERROR: EnumDevices\n";
        system("PAUSE");
        return hr;
    }
    // Make sure we got a joystick
    for(int i=0;i<(int)vec_joys.size();i++)
    {
        if(vec_joys[i] == NULL)
        {
            //joystick not found
            cout<<"ERROR: Joystick not found: "<<i<<endl;
            system("PAUSE");
            return E_FAIL;
        }
    }
    cout<<"Numof inputs: "<<(int)vec_joys.size()<<endl;



    //joystick settings
    cout<<"Joystick settings\n";
    DIDEVCAPS capabilities;

    // Set the data format to "simple joystick" - a predefined data format
    //
    // A data format specifies which controls on a device we are interested in,
    // and how they should be reported. This tells DInput that we will be
    // passing a DIJOYSTATE2 structure to IDirectInputDevice::GetDeviceState().
    for(int i=0;i<(int)vec_joys.size();i++)
    {
        hr = vec_joys[i]->SetDataFormat(&c_dfDIJoystick2);
        if (FAILED(hr))
        {
            cout<<"ERROR: SetDataFormat: "<<i<<endl;
            system("PAUSE");
            return hr;
        }
    }

    // Set the cooperative level to let DInput know how this device should
    // interact with the system and with other DInput applications.
/*FLAGS:
DISCL_BACKGROUND
    The application requires background access.
    If background access is granted, the device can be acquired at any time,
    even when the associated window is not the active window.
DISCL_EXCLUSIVE
    The application requires exclusive access.
    If exclusive access is granted, no other instance of the device can obtain
    exclusive access to the device while it is acquired. However,
    nonexclusive access to the device is always permitted, even if another
    application has obtained exclusive access. An application that acquires
    the mouse or keyboard device in exclusive mode should always unacquire
    the devices when it receives WM_ENTERSIZEMOVE and WM_ENTERMENULOOP
    messages. Otherwise, the user cannot manipulate the menu or move and
    resize the window.
DISCL_FOREGROUND (ERROR if hwnd window is not at the top)
    The application requires foreground access. If foreground access is
    granted, the device is automatically unacquired when the associated
    window moves to the background.
DISCL_NONEXCLUSIVE
    The application requires nonexclusive access. Access to the device does
    not interfere with other applications that are accessing the same device.
DISCL_NOWINKEY
    Disable the Windows logo key. Setting this flag ensures that the user
    cannot inadvertently break out of the application. Note, however, that
    DISCL_NOWINKEY has no effect when the default action mapping user
    interface (UI) is displayed, and the Windows logo key will operate
    normally as long as that UI is present.
*/
    cout<<"Joystick cooperative level\n";
    for(int i=0;i<(int)vec_joys.size();i++)
    {
        hr = vec_joys[i]->SetCooperativeLevel(hwnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
        if (FAILED(hr))
        {
            //DIERR_INVALIDPARAM, DIERR_NOTINITIALIZED, E_HANDLE.
            cout<<"ERROR: SetCooperativeLevel: "<<i<<endl;
            if(hr==DIERR_INVALIDPARAM) cout<<"DIERR_INVALIDPARAM\n";
            if(hr==DIERR_NOTINITIALIZED) cout<<"DIERR_NOTINITIALIZED\n";
            if(hr==E_HANDLE) cout<<"E_HANDLE\n";
            system("PAUSE");
            return hr;
        }
    }

    // Determine how many axis the joystick has (so we don't error out setting
    // properties for unavailable axis)
    cout<<"Joystick axis enumeration\n";
    for(int i=0;i<(int)vec_joys.size();i++)
    {
        capabilities.dwSize = sizeof(DIDEVCAPS);
        hr = vec_joys[i]->GetCapabilities(&capabilities);
        if (FAILED(hr))
        {
            cout<<"ERROR: GetCapabilities: "<<i<<endl;
            system("PAUSE");
            return hr;
        }
    }


    // Enumerate the axes of the joyctick and set the range of each axis. Note:
    // we could just use the defaults, but we're just trying to show an example
    // of enumerating device objects (axes, buttons, etc.).
    /*cout<<"Joystick axis calibration\n";
    for(int i=0;i<(int)vec_joys.size();i++)
    {
        hr = vec_joys[i]->EnumObjects(enumAxesCallback, NULL, DIDFT_AXIS);
        if (FAILED(hr))
        {
            cout<<"ERROR: EnumObjects\n";
            system("PAUSE");
            return hr;
        }
    }*/

    cout<<"Joystick Initialization Complete\n";

    //The joystick data can be accessed with poll();

//---------------------------------------------------

    /* program main loop */
    while (!bQuit)
    {
        /* check for messages */
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            /* handle or dispatch messages */
            if (msg.message == WM_QUIT)
            {
                bQuit = TRUE;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            //joystick poll
            for(int i=0;i<(int)vec_joys.size();i++)
            {
                DIJOYSTATE2 js;
                poll(&js,i);
                //report joystick data
                for(int but=0;but<128;but++)
                {
                    //buttons
                    if(js.rgbButtons[but]) cout<<"Joy "<<i<<" Pressed Button "<<but<<endl;
                }
                //axes
                if( vec_last_x[i]!=js.lX || vec_last_y[i]!=js.lY)
                {
                    cout<<"Joy "<<i<<" Axis: "<<js.lX<<", "<<js.lY<<endl;
                    vec_last_x[i]=js.lX;
                    vec_last_y[i]=js.lY;
                }

                //Other data in dinput.h
            }


            /* OpenGL animation code goes here */
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glPushMatrix();
            glRotatef(theta, 0.0f, 0.0f, 1.0f);

            glBegin(GL_TRIANGLES);

                glColor3f(1.0f, 0.0f, 0.0f);   glVertex2f(0.0f,   1.0f);
                glColor3f(0.0f, 1.0f, 0.0f);   glVertex2f(0.87f,  -0.5f);
                glColor3f(0.0f, 0.0f, 1.0f);   glVertex2f(-0.87f, -0.5f);

            glEnd();

            glPopMatrix();

            SwapBuffers(hDC);

            theta += 1.0f;
            Sleep (1);
        }
    }

    /* shutdown OpenGL */
    DisableOpenGL(hwnd, hDC, hRC);

    /* destroy the window explicitly */
    DestroyWindow(hwnd);

    //remove the joystick
    for(int i=0;i<(int)vec_joys.size();i++)
    {
        if (vec_joys[i])
        {
            vec_joys[i]->Unacquire();
        }
    }

    return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CLOSE:
            PostQuitMessage(0);
        break;

        case WM_DESTROY:
            return 0;

        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_ESCAPE:
                    PostQuitMessage(0);
                break;
            }
        }
        break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

void EnableOpenGL(HWND hwnd, HDC* hDC, HGLRC* hRC)
{
    PIXELFORMATDESCRIPTOR pfd;

    int iFormat;

    /* get the device context (DC) */
    *hDC = GetDC(hwnd);

    /* set the pixel format for the DC */
    ZeroMemory(&pfd, sizeof(pfd));

    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW |
                  PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    iFormat = ChoosePixelFormat(*hDC, &pfd);

    SetPixelFormat(*hDC, iFormat, &pfd);

    /* create and enable the render context (RC) */
    *hRC = wglCreateContext(*hDC);

    wglMakeCurrent(*hDC, *hRC);
}

void DisableOpenGL (HWND hwnd, HDC hDC, HGLRC hRC)
{
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hwnd, hDC);
}

BOOL CALLBACK
enumCallback(const DIDEVICEINSTANCE* instance, VOID* context)
{
    HRESULT hr;

    //store all joysticks
    vec_joys.push_back( LPDIRECTINPUTDEVICE8() );
    vec_last_x.push_back(0);
    vec_last_y.push_back(0);

    // Obtain an interface to the enumerated joystick.
    hr = di->CreateDevice(instance->guidInstance, &vec_joys[joy_counter], NULL);

    // If it failed, then we can't use this joystick. (Maybe the user unplugged
    // it while we were in the middle of enumerating it.)
    if (FAILED(hr))
    {
        //remove that joystick
        vec_joys.pop_back();
        vec_last_x.pop_back();
        vec_last_y.pop_back();

        return DIENUM_CONTINUE;
    }

    joy_counter++;
    cout<<"Joystick added: "<<joy_counter<<endl;


    return DIENUM_CONTINUE;
    // Stop enumeration. Note: we're just taking the first joystick we get. You
    // could store all the enumerated joysticks and let the user pick.
    return DIENUM_STOP;
}

BOOL CALLBACK
enumAxesCallback(const DIDEVICEOBJECTINSTANCE* instance, VOID* context)
{
    HWND hDlg = (HWND)context;

    DIPROPRANGE propRange;
    propRange.diph.dwSize       = sizeof(DIPROPRANGE);
    propRange.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    propRange.diph.dwHow        = DIPH_BYID;
    propRange.diph.dwObj        = instance->dwType;
    propRange.lMin              = -1000;
    propRange.lMax              = +1000;

    // Set the range for the axis
    HRESULT hr;
    hr = joystick->SetProperty(DIPROP_RANGE, &propRange.diph);
    if (FAILED(hr))
    {
        return DIENUM_STOP;
    }

    return DIENUM_CONTINUE;
}

HRESULT
poll(DIJOYSTATE2 *js,int joystick_index)
{
    HRESULT hr;

    if (vec_joys[joystick_index] == NULL)
    {
        return S_OK;
    }


    // Poll the device to read the current state
    hr = vec_joys[joystick_index]->Poll();
    if (FAILED(hr))
    {
        // DInput is telling us that the input stream has been
        // interrupted. We aren't tracking any state between polls, so
        // we don't have any special reset that needs to be done. We
        // just re-acquire and try again.
        hr = vec_joys[joystick_index]->Acquire();
        while (hr == DIERR_INPUTLOST)
        {
            hr = vec_joys[joystick_index]->Acquire();
        }

        // If we encounter a fatal error, return failure.
        if ((hr == DIERR_INVALIDPARAM) || (hr == DIERR_NOTINITIALIZED))
        {
            return E_FAIL;
        }

        // If another application has control of this device, return successfully.
        // We'll just have to wait our turn to use the joystick.
        if (hr == DIERR_OTHERAPPHASPRIO)
        {
            return S_OK;
        }
    }

    // Get the input's device state
    hr = vec_joys[joystick_index]->GetDeviceState(sizeof(DIJOYSTATE2), js);
    if (FAILED(hr))
    {
        return hr; // The device should have been acquired during the Poll()
    }

    return S_OK;
}
