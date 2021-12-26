#include "stdafx.h"
#include "Application.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    Application app(1280, 720, L"Model Viewer");

    return app.Run(hInstance, nCmdShow);
}