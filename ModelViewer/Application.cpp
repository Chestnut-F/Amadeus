#include "stdafx.h"
#include "Application.h"

HWND Application::mHwnd = nullptr;

Application::Application(UINT width, UINT height, std::wstring name) :
    mWidth(width),
    mHeight(height),
    mTitle(name)
{
    mAspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

Application::~Application()
{
}

int Application::Run(HINSTANCE hInstance, int nCmdShow)
{
    // Parse the command line parameters
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    LocalFree(argv);

    // Initialize the window class.
    WNDCLASSEX windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = L"Application";
    RegisterClassEx(&windowClass);

    RECT windowRect = { 0, 0, static_cast<LONG>(mWidth), static_cast<LONG>(mHeight) };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // Create the window and store a handle to it.
    mHwnd = CreateWindow(
        windowClass.lpszClassName,
        this->GetTitle(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,        // We have no parent window.
        nullptr,        // We aren't using menus.
        hInstance,
        this);

    mRoot.reset(new Amadeus::Root(mTitle.c_str(), mWidth, mHeight, mHwnd));

    mRoot->Init();

    ShowWindow(mHwnd, nCmdShow);

    // Main sample loop.
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        // Process any messages in the queue.
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    mRoot->Destroy();

    // Return this part of the WM_QUIT message to Windows.
    return static_cast<char>(msg.wParam);
}

// Main message handler for the sample.
LRESULT CALLBACK Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Application* app = reinterpret_cast<Application*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_CREATE:
        {
            // Save the Application* passed in to CreateWindow.
            LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
        }
        return 0;

    case WM_MOUSEWHEEL:
        if (app)
        {
            app->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        }
        return 0;

    case WM_MOUSEMOVE:
        if (app)
        {
            app->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        return 0;

    case WM_LBUTTONDOWN:
        if (app)
        {
            app->OnButtonDown(0x01, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        return 0;

    case WM_RBUTTONDOWN:
        if (app)
        {
            app->OnButtonDown(0x02, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }
        return 0;

    case WM_LBUTTONUP:
        if (app)
        {
            app->OnButtonUp();
        }
        return 0;

    case WM_RBUTTONUP:
        if (app)
        {
            app->OnButtonUp();
        }
        return 0;

    case WM_KEYDOWN:
        if (app)
        {
            app->OnKeyDown(static_cast<UINT8>(wParam));
        }
        return 0;

    case WM_KEYUP:
        if (app)
        {
            app->OnKeyUp(static_cast<UINT8>(wParam));
        }
        return 0;

    case WM_PAINT:
        if (app)
        {
            app->Render();
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    // Handle any messages the switch statement didn't.
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void Application::OnKeyDown(UINT8 key)
{
}

void Application::OnKeyUp(UINT8 key)
{
}

void Application::OnMouseMove(INT x, INT y)
{
    mRoot->OnMouseMove(x, y);
}

void Application::OnMouseWheel(INT16 zDelta)
{
    mRoot->OnMouseWheel(zDelta);
}

void Application::OnButtonDown(INT button, INT x, INT y)
{
    mRoot->OnButtonDown(button, x, y);
}

void Application::OnButtonUp()
{
    mRoot->OnButtonUp();
}

void Application::Render()
{
    mRoot->PreRender();
    mRoot->Render();
    mRoot->PostRender();
}
