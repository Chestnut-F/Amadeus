#pragma once
#include "Root.h"

class Application
{
public:
    Application(UINT width, UINT height, std::wstring name);
    ~Application();

    int Run(HINSTANCE hInstance, int nCmdShow);
    static HWND GetHwnd() { return mHwnd; }

    // Accessors.
    UINT GetWidth() const { return mWidth; }
    UINT GetHeight() const { return mHeight; }
    const WCHAR* GetTitle() const { return mTitle.c_str(); }

protected:
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    // The event handlers to handle specific messages.
    void OnKeyDown(UINT8 key);
    void OnKeyUp(UINT8 key);
    void OnMouseMove(INT x, INT y);
    void OnMouseWheel(INT16 zDelta);
    void OnButtonDown(INT button, INT x, INT y);
    void OnButtonUp();

    void Render();

private:
    static HWND mHwnd;

    // Viewport dimensions.
    UINT mWidth;
    UINT mHeight;
    float mAspectRatio;

    // Window title.
    std::wstring mTitle;
    std::wstring mAssetsPath;

    // Root
    typedef std::unique_ptr<Amadeus::Root> RootPtr;
    RootPtr mRoot;
};